#include <stdarg.h>
#include <string.h>
#include "libretro.h"
#include "gb_emu.h"

#define MAX_MSG_LEN  100
#define MSG_DURATION 360

#define GBSTATUS_RETRO_ERR(msg) err_print("%s ([%s] %s)\n", msg, __gbstatus_str_repr[status], __gbstatus_str);

typedef struct
{
   char r;
   char g;
   char b;
   char x;
} fb_color_t;

fb_color_t gb_screen_colors[] =
{
   { 0xFF, 0xFF, 0xFF, 0x00 },
   { 0xAA, 0xAA, 0xAA, 0x00 },
   { 0x55, 0x55, 0x55, 0x00 },
   { 0x00, 0x00, 0x00, 0x00 }
};

/**
 * Libretro relies on global state
 */

static retro_environment_t        env_cb         = NULL;
static retro_log_printf_t         log_cb         = NULL;
static retro_audio_sample_t       audio_cb       = NULL;
static retro_audio_sample_batch_t audio_batch_cb = NULL;
static retro_video_refresh_t      video_cb       = NULL;
static retro_input_poll_t         input_poll_cb  = NULL;
static retro_input_state_t        input_state_cb = NULL;

static bool core_initialized = false;

static gb_emu_t gb_emu = {0};

static       char *out_framebuffer    = NULL;
static const char *gb_framebuffer     = NULL;
static const bool *gb_frame_ready_ptr = NULL;

static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
   va_list va;
   va_start(va, fmt);
   vfprintf(stderr, fmt, va);
   va_end(va);
}

static void err_print(const char *fmt, ...)
{
   char msg[MAX_MSG_LEN + 1] = {0};

   va_list va;
   va_start(va, fmt);
   vsnprintf(msg, MAX_MSG_LEN, fmt, va);
   va_end(va);

   struct retro_message screen_msg = {0};
   screen_msg.msg = msg;
   screen_msg.frames = MSG_DURATION;

   log_cb(RETRO_LOG_ERROR, msg);
   env_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &screen_msg);
}

unsigned retro_api_version()
{
   return RETRO_API_VERSION;
}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(struct retro_system_info));
   info->library_name     = "gb";
   info->library_version  = "0.9";
   info->need_fullpath    = true;
   info->block_extract    = false;
   info->valid_extensions = "gb";
}

void retro_set_environment(retro_environment_t cb)
{
   env_cb = cb;

   struct retro_log_callback logging = {0};
   if(env_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
      log_cb = logging.log;
   else
      log_cb = fallback_log;

   bool no_rom = true;
   env_cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_rom);
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

void retro_init()
{
   gbstatus_e status = GBSTATUS_OK;

   status = gb_emu_init(&gb_emu);
   if (status != GBSTATUS_OK)
      goto error_handler0;

   gb_emu_framebuffer_ptr(&gb_emu, &gb_framebuffer);

   gb_emu_frame_ready_ptr(&gb_emu, &gb_frame_ready_ptr);

   // XRGB8888
   out_framebuffer = calloc(GB_SCREEN_HEIGHT * GB_SCREEN_WIDTH * 4, sizeof(char));
   if (out_framebuffer == NULL)
   {
      GBSTATUS(GBSTATUS_BAD_ALLOC, "unable to allocate memory");
      goto error_handler1;
   }

   core_initialized = true;
   return;

error_handler1:
   gb_emu_deinit(&gb_emu);

error_handler0:
   GBSTATUS_RETRO_ERR("Failed to initialize Gameboy core!");
}

bool retro_load_game(const struct retro_game_info *info)
{
   if (!core_initialized)
   {
      err_print("Gameboy core is not initialized! Try to reload core");
      return false;
   }

   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
   if (!env_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   {
      err_print("XRGB8888 is not supported!");
      return false;
   }

   struct retro_input_descriptor desc[] = {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT  , "Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP    , "Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN  , "Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT , "Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A     , "A" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B     , "B" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START , "Start" },
      { 0 },
   };

   env_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

   // Allow running without ROM
   if (info == NULL)
      return true;

   gbstatus_e status = GBSTATUS_OK;

   status = gb_emu_change_rom(&gb_emu, info->path);
   if (status != GBSTATUS_OK)
   {
      GBSTATUS_RETRO_ERR("Failed to load game");
      return false;
   }
      
   return true;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
   return false;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   info->geometry.base_width   = GB_SCREEN_WIDTH;
   info->geometry.base_height  = GB_SCREEN_HEIGHT;
   info->geometry.max_width    = GB_SCREEN_WIDTH;
   info->geometry.max_height   = GB_SCREEN_HEIGHT;
   info->geometry.aspect_ratio = (float)GB_SCREEN_WIDTH / (float)GB_SCREEN_HEIGHT;

   info->timing.fps = 60.0f;
}

void retro_run()
{
   gbstatus_e status = GBSTATUS_OK;

   int joypad_state = 0;

   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A))
      joypad_state |= BUTTON_A;

   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B))
      joypad_state |= BUTTON_B;

   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT))
      joypad_state |= BUTTON_SELECT;

   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START))
      joypad_state |= BUTTON_START;

   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP))
      joypad_state |= BUTTON_UP;

   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN))
      joypad_state |= BUTTON_DOWN;

   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT))
      joypad_state |= BUTTON_LEFT;

   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT))
      joypad_state |= BUTTON_RIGHT;

   gb_emu_update_input(&gb_emu, joypad_state);

   while (!(*gb_frame_ready_ptr))
   {
      status = gb_emu_step(&gb_emu);
      if (status != GBSTATUS_OK)
      {
         GBSTATUS_RETRO_ERR("Emulation error!");
         return;
      }
   }

   gb_emu_grab_frame(&gb_emu);

   for (int y = 0; y < GB_SCREEN_HEIGHT; y++)
   {
      for (int x = 0; x < GB_SCREEN_WIDTH; x++)
      {
         char gb_color = gb_framebuffer[y * GB_SCREEN_WIDTH + x];
         memcpy(out_framebuffer + (y * GB_SCREEN_WIDTH + x) * 4, &gb_screen_colors[gb_color], sizeof(fb_color_t));
      }
   }

   video_cb(out_framebuffer, GB_SCREEN_WIDTH, GB_SCREEN_HEIGHT, GB_SCREEN_WIDTH * 4);
   audio_cb(0, 0);
}

unsigned int retro_get_region()
{
   return RETRO_REGION_NTSC;
}

void retro_reset()
{
   gb_emu_reset(&gb_emu);
}

void retro_unload_game()
{
   gb_emu_unload_rom(&gb_emu);
}

void retro_deinit()
{
   if (!core_initialized)
      return;

   free(out_framebuffer);
   gb_emu_deinit(&gb_emu);

   core_initialized = false;
   return;
}

void retro_set_controller_port_device(unsigned int port, unsigned int device)
{

}

void *retro_get_memory_data(unsigned int id)
{
   return NULL;
}

size_t retro_get_memory_size(unsigned int id)
{
   return 0;
}

size_t retro_serialize_size()
{
   return 0;
}

bool retro_serialize(void *data, size_t size)
{
   return false;
}

bool retro_unserialize(const void *data, size_t size)
{
   return false;
}

void retro_cheat_reset()
{
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
}