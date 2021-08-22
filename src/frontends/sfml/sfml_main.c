#include <SFML/Graphics.h>
#include <stdio.h>
#include <string.h>
#include "gb_emu.h"

sfColor gb_screen_colors[] =
{
    { 0xFF, 0xFF, 0xFF, 0xFF},
    { 0xAA, 0xAA, 0xAA, 0xFF},
    { 0x55, 0x55, 0x55, 0xFF },
    { 0x00, 0x00, 0x00, 0xFF }
};

typedef struct
{
    sfRenderWindow *sf_window;

    sfImage   *sf_image;
    sfTexture *sf_texture;
    sfSprite  *sf_sprite;
} sfml_frontend_t;

#define SCREEN_SCALE 4

gbstatus_e init_sfml(sfml_frontend_t *frontend)
{
    gbstatus_e status = GBSTATUS_OK;

    sfVideoMode video_mode = { GB_SCREEN_WIDTH * SCREEN_SCALE, GB_SCREEN_HEIGHT * SCREEN_SCALE, 32 };

    frontend->sf_window = sfRenderWindow_create(video_mode, "gb", sfClose, NULL);
    if (frontend->sf_window == NULL)
    {
        GBSTATUS(GBSTATUS_SFML_FAIL, "unable to initialize SFML");
        goto cleanup0;
    }

    frontend->sf_image = sfImage_create(GB_SCREEN_WIDTH, GB_SCREEN_HEIGHT);
    if (frontend->sf_image == NULL)
    {
        GBSTATUS(GBSTATUS_SFML_FAIL, "unable to initialize SFML");
        goto cleanup1;
    }

    frontend->sf_texture = sfTexture_create(GB_SCREEN_WIDTH, GB_SCREEN_HEIGHT);
    if (frontend->sf_texture == NULL)
    {
        GBSTATUS(GBSTATUS_SFML_FAIL, "unable to initialize SFML");
        goto cleanup2;
    }

    frontend->sf_sprite = sfSprite_create();
    if (frontend->sf_sprite == NULL)
    {
        GBSTATUS(GBSTATUS_SFML_FAIL, "unable to initialize SFML");
        goto cleanup3;
    }

    sfSprite_setPosition(frontend->sf_sprite, (sfVector2f){ 0, 0 });
    sfSprite_setScale   (frontend->sf_sprite, (sfVector2f){ SCREEN_SCALE, SCREEN_SCALE });
    sfSprite_setTexture (frontend->sf_sprite, frontend->sf_texture, false);
    return GBSTATUS_OK;

cleanup3:
    sfTexture_destroy(frontend->sf_texture);

cleanup2:
    sfImage_destroy(frontend->sf_image);

cleanup1:
    sfRenderWindow_destroy(frontend->sf_window);

cleanup0:
    return status;
}

void deinit_sfml(sfml_frontend_t *frontend)
{
    sfSprite_destroy(frontend->sf_sprite);
    sfTexture_destroy(frontend->sf_texture);
    sfImage_destroy(frontend->sf_image);
    sfRenderWindow_destroy(frontend->sf_window);
}

gbstatus_e run(const char *rom_path)
{
    gbstatus_e status = GBSTATUS_OK;

    sfml_frontend_t frontend = {0};
    status = init_sfml(&frontend);
    if (status != GBSTATUS_OK)
        goto cleanup0;

    gb_emu_t gb_emu = {0};

    status = gb_emu_init(&gb_emu);
    if (status != GBSTATUS_OK)
        goto cleanup1;

    status = gb_emu_change_rom(&gb_emu, rom_path);
    if (status != GBSTATUS_OK)
        goto cleanup2;

    const char *game_title = NULL;
    gb_emu_game_title_ptr(&gb_emu, &game_title);

    const char *framebuffer = NULL;
    gb_emu_framebuffer_ptr(&gb_emu, &framebuffer);

    const bool *frame_ready_ptr = NULL;
    gb_emu_frame_ready_ptr(&gb_emu, &frame_ready_ptr);

    char window_title[GAME_TITLE_LEN + 20];
    strncpy(window_title, game_title, GAME_TITLE_LEN);
    strcat(window_title, " - gb");
    sfRenderWindow_setTitle(frontend.sf_window, window_title);

    sfRenderWindow_setFramerateLimit(frontend.sf_window, 60);

    while (sfRenderWindow_isOpen(frontend.sf_window))
    {
        sfEvent event;
        while (sfRenderWindow_pollEvent(frontend.sf_window, &event))
        {
            if (event.type == sfEvtClosed)
                sfRenderWindow_close(frontend.sf_window);
        }

        int joypad_state = 0;

        if (sfKeyboard_isKeyPressed(sfKeyX))
            joypad_state |= BUTTON_A;

        if (sfKeyboard_isKeyPressed(sfKeyZ))
            joypad_state |= BUTTON_B;

        if (sfKeyboard_isKeyPressed(sfKeySpace))
            joypad_state |= BUTTON_SELECT;

        if (sfKeyboard_isKeyPressed(sfKeyEnter))
            joypad_state |= BUTTON_START;

        if (sfKeyboard_isKeyPressed(sfKeyUp))
            joypad_state |= BUTTON_UP;

        if (sfKeyboard_isKeyPressed(sfKeyDown))
            joypad_state |= BUTTON_DOWN;

        if (sfKeyboard_isKeyPressed(sfKeyLeft))
            joypad_state |= BUTTON_LEFT;

        if (sfKeyboard_isKeyPressed(sfKeyRight))
            joypad_state |= BUTTON_RIGHT;

        gb_emu_update_input(&gb_emu, joypad_state);

        while (!(*frame_ready_ptr))
        {
            status = gb_emu_step(&gb_emu);
            if (status != GBSTATUS_OK)
                goto cleanup2;
        }

        gb_emu_grab_frame(&gb_emu);

        for (int y = 0; y < GB_SCREEN_HEIGHT; y++)
        {
            for (int x = 0; x < GB_SCREEN_WIDTH; x++)
            {
                int color = framebuffer[y * GB_SCREEN_WIDTH + x];
                sfImage_setPixel(frontend.sf_image, x, y, gb_screen_colors[color]);
            }
        }

        sfTexture_updateFromImage(frontend.sf_texture, frontend.sf_image, 0, 0);
        
        sfRenderWindow_clear(frontend.sf_window, sfBlack);
        sfRenderWindow_drawSprite(frontend.sf_window, frontend.sf_sprite, NULL);
        sfRenderWindow_display(frontend.sf_window);
    }

cleanup2:
    gb_emu_deinit(&gb_emu);

cleanup1:
    deinit_sfml(&frontend);

cleanup0:
    return status;
}

int main(int argc, const char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: ./gb <ROM file path>\n");
        return 0;
    }

    gbstatus_e status = run(argv[1]);

    if (status != GBSTATUS_OK)
    {
        GBSTATUS_ERR_PRINT("Something went wrong");
        return -1;
    }

    return 0;
}