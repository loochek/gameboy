#include <SFML/Graphics.h>
#include <stdio.h>
#include "gb.h"
#include "cart.h"

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

    gb_t        gb     = {0};
    gb_cpu_t    cpu    = {0};
    gb_mmu_t    mmu    = {0};
    gb_timer_t  timer  = {0};
    gb_ppu_t    ppu    = {0};
    gb_joypad_t joypad = {0};
    gb_int_controller_t intr_ctl = {0};
    
    gb.cpu       = &cpu;
    gb.mmu       = &mmu;
    gb.intr_ctrl = &intr_ctl;
    gb.timer     = &timer;
    gb.ppu       = &ppu;
    gb.joypad    = &joypad;

    status = cpu_init(gb.cpu, &gb);
    if (status != GBSTATUS_OK)
        goto cleanup1;

    status = mmu_init(gb.mmu, &gb);
    if (status != GBSTATUS_OK)
        goto cleanup1;

    status = int_init(gb.intr_ctrl, &gb);
    if (status != GBSTATUS_OK)
        goto cleanup2;

    status = timer_init(gb.timer, &gb);
    if (status != GBSTATUS_OK)
        goto cleanup2;

    status = joypad_init(gb.joypad, &gb);
    if (status != GBSTATUS_OK)
        goto cleanup2;

    gb_cart_t cart = {0};
    status = cart_init(&cart, rom_path);
    if (status != GBSTATUS_OK)
        goto cleanup2;

    status = mmu_switch_cart(gb.mmu, &cart);
    if (status != GBSTATUS_OK)
        goto cleanup3;

    status = ppu_init(gb.ppu, &gb);
    if (status != GBSTATUS_OK)
        goto cleanup3;

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

        GBCHK(joypad_update(gb.joypad, joypad_state));

        while (!gb.ppu->new_frame_ready)
        {
            status = cpu_step(gb.cpu);
            if (status != GBSTATUS_OK)
                goto cleanup4;
        }

        ppu.new_frame_ready = false;

        for (int y = 0; y < GB_SCREEN_HEIGHT; y++)
        {
            for (int x = 0; x < GB_SCREEN_WIDTH; x++)
            {
                int color = gb.ppu->framebuffer[y * GB_SCREEN_WIDTH + x];
                sfImage_setPixel(frontend.sf_image, x, y, gb_screen_colors[color]);
            }
        }

        sfTexture_updateFromImage(frontend.sf_texture, frontend.sf_image, 0, 0);
        
        sfRenderWindow_clear(frontend.sf_window, sfBlack);
        sfRenderWindow_drawSprite(frontend.sf_window, frontend.sf_sprite, NULL);
        sfRenderWindow_display(frontend.sf_window);
    }

cleanup4:
    GBCHK(ppu_deinit(gb.ppu));

cleanup3:
    GBCHK(cart_deinit(&cart));

cleanup2:
    GBCHK(mmu_deinit(gb.mmu));

cleanup1:
    deinit_sfml(&frontend);

cleanup0:
    return status;
}

int main(int argc, const char *argv[])
{
    gbstatus_e status = run(argv[1]);

    if (status != GBSTATUS_OK)
    {
        GBSTATUS_ERR_PRINT();
        return -1;
    }

    return 0;
}