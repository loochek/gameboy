#include <string.h>
#include <assert.h>
#include "ppu.h"
#include "gb.h"

/**
 * Note: In Nintendo documentation sprites are referred to as "objects". 
 * Both terms are used synonymously in this file.
 */

#define VRAM_SIZE 0x2000
#define OAM_SIZE  0xA0

#define OAM_ENTRY_SIZE 0x4

#define LCDC_BG_WIN_ENABLE_BIT   0
#define LCDC_OBJ_ENABLE_BIT      1
#define LCDC_OBJ_SIZE_BIT        2
#define LCDC_BG_TILEMAP_BIT      3
#define LCDC_BG_WIN_TILEDATA_BIT 4
#define LCDC_WIN_ENABLE_BIT      5
#define LCDC_WIN_TILEMAP_BIT     6
#define LCDC_PPU_ON_BIT          7

#define STAT_STATE_BIT0         0
#define STAT_STATE_BIT1         1
#define STAT_LYC_FLAG_BIT       2
#define STAT_HBLANK_INT_BIT     3
#define STAT_VBLANK_INT_BIT     4
#define STAT_OBJ_SEARCH_INT_BIT 5
#define STAT_LYC_INT_BIT        6

#define OAM_ENTRY_PALLETE_BIT  4
#define OAM_ENTRY_FLIP_X_BIT   5
#define OAM_ENTRY_FLIP_Y_BIT   6
#define OAM_ENTRY_PRIORITY_BIT 7

#define STATE_OBJ_SEARCH_DURATION 80
#define STATE_DRAWING_DURATION    172
#define STATE_HBLANK_DURATION     200
#define STATE_HBLANK_INC_DURATION 4
#define STATE_VBLANK_DURATION     452
#define STATE_VBLANK_INC_DURATION 4

#define FRAME_DURATION 70224

// In pixels

#define TILE_WIDTH  8
#define TILE_HEIGHT 8

#define WIN_GLOBAL_X_OFFSET 7

#define OBJ_HEIGHT_0 8
#define OBJ_HEIGHT_1 16
#define OBJ_WIDTH    8

#define OBJ_GLOBAL_X_OFFSET 8
#define OBJ_GLOBAL_Y_OFFSET 16

// In tiles

#define BG_WIDTH  32
#define BG_HEIGHT 32

// Relative to VRAM offset

#define TILEDATA0_ADDR 0x800
#define TILEDATA1_ADDR 0x0

#define TILEMAP0_ADDR 0x1800
#define TILEMAP1_ADDR 0x1C00

#define SET_BIT(var, bit, val) (var = ((var) & (~(1 << (bit)))) | (((val) & 0x1) << (bit)))
#define GET_BIT(val, bit) (((val) >> (bit)) & 0x1)

#define GET_TILE_PIXEL(addr, y_offs, x_offs)                               \
(  ((ppu->vram[(addr) + 2 * (y_offs)    ] >> (7 - (x_offs))) & 0x1) |      \
  (((ppu->vram[(addr) + 2 * (y_offs) + 1] >> (7 - (x_offs))) & 0x1) << 1))

static void ppu_handle_lyc(gb_ppu_t *ppu);

static void ppu_render_scanline    (gb_ppu_t *ppu);
static void ppu_render_bg_scanline (gb_ppu_t *ppu);
static void ppu_render_win_scanline(gb_ppu_t *ppu);
static void ppu_render_obj_scanline(gb_ppu_t *ppu);

/// Determines which sprites will be displayed on the current line
static void ppu_search_obj(gb_ppu_t *ppu);


gbstatus_e ppu_init(gb_ppu_t *ppu, struct gb *gb)
{
    gbstatus_e status = GBSTATUS_OK;

    assert(ppu != NULL);
    assert(gb != NULL);

    ppu->gb = gb;

    ppu->vram = calloc(VRAM_SIZE, sizeof(uint8_t));
    if (ppu->vram == NULL)
    {
        GBSTATUS(GBSTATUS_BAD_ALLOC, "unable to allocate memory");
        goto error_handler0;
    }

    ppu->oam = calloc(OAM_SIZE, sizeof(uint8_t));
    if (ppu->oam == NULL)
    {
        GBSTATUS(GBSTATUS_BAD_ALLOC, "unable to allocate memory");
        goto error_handler1;
    }

    ppu->framebuffer = calloc(GB_SCREEN_WIDTH * GB_SCREEN_HEIGHT, sizeof(char));
    if (ppu->framebuffer == NULL)
    {
        GBSTATUS(GBSTATUS_BAD_ALLOC, "unable to allocate memory");
        goto error_handler2;
    }

    ppu->bg_scanline_buffer = calloc(GB_SCREEN_WIDTH, sizeof(char));
    if (ppu->bg_scanline_buffer == NULL)
    {
        GBSTATUS(GBSTATUS_BAD_ALLOC, "unable to allocate memory");
        goto error_handler3;
    }

    ppu_reset(ppu);
    return GBSTATUS_OK;

error_handler3:
    free(ppu->framebuffer);

error_handler2:
    free(ppu->oam);

error_handler1:
    free(ppu->vram);

error_handler0:
    return status;
}

void ppu_reset(gb_ppu_t *ppu)
{
    assert(ppu != NULL);

    ppu->reg_lcdc = 0x91;
    ppu->reg_stat = 0x85;
    ppu->reg_ly   = 0x00;
    ppu->reg_lyc  = 0x00;
    ppu->reg_scx  = 0x00;
    ppu->reg_scy  = 0x00;
    ppu->reg_bgp  = 0xFC;
    ppu->reg_obp0 = 0xFF;
    ppu->reg_obp1 = 0xFF;
    ppu->reg_wy   = 0x00;
    ppu->reg_wy   = 0x00;

    ppu->lcdc_blocked = false;
    ppu->next_state   = STATE_OBJ_SEARCH;
    ppu->clocks_to_next_state = 0;

    ppu->window_line = 0;
    ppu->delayed_wy  = -1;

    memset(ppu->vram, 0, VRAM_SIZE);
    memset(ppu->oam , 0, OAM_SIZE);
}

void ppu_update(gb_ppu_t *ppu, int elapsed_cycles)
{
    assert(ppu != NULL);

    gb_t *gb = ppu->gb;

    ppu->cycles_counter += elapsed_cycles;

    while (ppu->cycles_counter >= ppu->clocks_to_next_state)
    {
        ppu->cycles_counter -= ppu->clocks_to_next_state;

        if (!GET_BIT(ppu->reg_lcdc, LCDC_PPU_ON_BIT))
        {
            // When PPU is off

            ppu->lcdc_blocked = false;
            ppu->reg_ly = 0;
            ppu->clocks_to_next_state = FRAME_DURATION;

            // Clear screen
            memset(ppu->framebuffer, 0, GB_SCREEN_WIDTH * GB_SCREEN_HEIGHT);
            ppu->new_frame_ready = true;
            
            continue;
        }

        switch (ppu->next_state)
        {
        case STATE_OBJ_SEARCH:
            ppu_handle_lyc(ppu);
                
            SET_BIT(ppu->reg_stat, STAT_STATE_BIT0, 0);
            SET_BIT(ppu->reg_stat, STAT_STATE_BIT1, 1);

            if (GET_BIT(ppu->reg_stat, STAT_OBJ_SEARCH_INT_BIT) && !ppu->lcdc_blocked)
            {
                int_request(&gb->intr_ctrl, INT_LCDC);
                ppu->lcdc_blocked = true;
            }

            ppu->next_state = STATE_DRAWING;
            ppu->clocks_to_next_state = STATE_OBJ_SEARCH_DURATION;
            break;

        case STATE_DRAWING:
            SET_BIT(ppu->reg_stat, STAT_STATE_BIT0, 1);
            SET_BIT(ppu->reg_stat, STAT_STATE_BIT1, 1);

            ppu->next_state = STATE_HBLANK;
            ppu->clocks_to_next_state = STATE_DRAWING_DURATION;
            break;

        case STATE_HBLANK:
            ppu_search_obj(ppu);
            ppu_render_scanline(ppu);

            SET_BIT(ppu->reg_stat, STAT_STATE_BIT0, 0);
            SET_BIT(ppu->reg_stat, STAT_STATE_BIT1, 0);

            if (GET_BIT(ppu->reg_stat, STAT_HBLANK_INT_BIT) && !ppu->lcdc_blocked)
            {
                int_request(&gb->intr_ctrl, INT_LCDC);
                ppu->lcdc_blocked = true;
            }

            ppu->next_state = STATE_HBLANK_INC;
            ppu->clocks_to_next_state = STATE_HBLANK_DURATION;
            break;

        case STATE_HBLANK_INC:
            ppu->lcdc_blocked = false;
            ppu->reg_ly++;

            if (ppu->reg_ly >= GB_SCREEN_HEIGHT)
                ppu->next_state = STATE_VBLANK;
            else
                ppu->next_state = STATE_OBJ_SEARCH;

            ppu->clocks_to_next_state = STATE_HBLANK_INC_DURATION;
            break;

        case STATE_VBLANK:
            if (ppu->reg_ly == GB_SCREEN_HEIGHT)
            {
                SET_BIT(ppu->reg_stat, STAT_STATE_BIT0, 1);
                SET_BIT(ppu->reg_stat, STAT_STATE_BIT1, 0);

                int_request(&gb->intr_ctrl, INT_VBLANK);
                
                ppu->new_frame_ready = true;
            }

            ppu_handle_lyc(ppu);

            if (GET_BIT(ppu->reg_stat, STAT_VBLANK_INT_BIT) && !ppu->lcdc_blocked)
            {
                int_request(&gb->intr_ctrl, INT_LCDC);
                ppu->lcdc_blocked = true;
            }

            ppu->next_state = STATE_VBLANK_INC;
            ppu->clocks_to_next_state = STATE_VBLANK_DURATION;
            break;

        case STATE_VBLANK_INC:
            ppu->reg_ly++;
            ppu->lcdc_blocked = false;

            if (ppu->reg_ly >= GB_SCREEN_HEIGHT + 10 - 1)
                ppu->next_state = STATE_VBLANK_LAST_LINE;
            else
                ppu->next_state = STATE_VBLANK;

            ppu->clocks_to_next_state = STATE_VBLANK_INC_DURATION;
            break;

        case STATE_VBLANK_LAST_LINE:
            ppu->reg_ly = 0;

            ppu_handle_lyc(ppu);
        
            ppu->next_state = STATE_VBLANK_LAST_LINE_INC;
            ppu->clocks_to_next_state = STATE_VBLANK_DURATION;
            break;

        case STATE_VBLANK_LAST_LINE_INC:
            SET_BIT(ppu->reg_stat, STAT_STATE_BIT0, 0);
            SET_BIT(ppu->reg_stat, STAT_STATE_BIT1, 0);

            ppu->lcdc_blocked = false;
            ppu->window_line  = 0;

            if (ppu->delayed_wy != -1)
            {
                ppu->reg_wy = ppu->delayed_wy;
                ppu->delayed_wy = -1;
            }

            ppu->next_state = STATE_OBJ_SEARCH;
            ppu->clocks_to_next_state = STATE_VBLANK_INC_DURATION;
        } 
    }
}

uint8_t ppu_lcdc_read(gb_ppu_t *ppu)
{
    assert(ppu != NULL);

    return ppu->reg_lcdc;
}

uint8_t ppu_stat_read(gb_ppu_t *ppu)
{
    assert(ppu != NULL);

    return ppu->reg_stat;
}

uint8_t ppu_ly_read(gb_ppu_t *ppu)
{
    assert(ppu != NULL);

    return ppu->reg_ly;
}

uint8_t ppu_lyc_read(gb_ppu_t *ppu)
{
    assert(ppu != NULL);

    return ppu->reg_lyc;
}

uint8_t ppu_scx_read(gb_ppu_t *ppu)
{
    assert(ppu != NULL);

    return ppu->reg_scx;
}

uint8_t ppu_scy_read(gb_ppu_t *ppu)
{
    assert(ppu != NULL);

    return ppu->reg_scy;
}

uint8_t ppu_wx_read(gb_ppu_t *ppu)
{
    assert(ppu != NULL);

    return ppu->reg_wx;
}

uint8_t ppu_wy_read(gb_ppu_t *ppu)
{
    assert(ppu != NULL);

    return ppu->reg_wy;
}

uint8_t ppu_bgp_read(gb_ppu_t *ppu)
{
    assert(ppu != NULL);

    return ppu->reg_bgp;
}

uint8_t ppu_obp0_read(gb_ppu_t *ppu)
{
    assert(ppu != NULL);

    return ppu->reg_obp0;
}

uint8_t ppu_obp1_read(gb_ppu_t *ppu)
{
    assert(ppu != NULL);

    return ppu->reg_obp1;
}

uint8_t ppu_dma_read(gb_ppu_t *ppu)
{
    assert(ppu != NULL);

    return 0xFF;
}

uint8_t ppu_vram_read(gb_ppu_t *ppu, uint16_t addr)
{
    assert(ppu != NULL);

    return ppu->vram[addr - 0x8000];
}

uint8_t ppu_oam_read(gb_ppu_t *ppu, uint16_t addr)
{
    assert(ppu != NULL);

    return ppu->oam[addr - 0xFE00];
}

void ppu_lcdc_write(gb_ppu_t *ppu, uint8_t value)
{
    assert(ppu != NULL);

    if (!GET_BIT(value, LCDC_PPU_ON_BIT))
    {
        // PPU has been turned off

        SET_BIT(ppu->reg_stat, STAT_STATE_BIT0, 0);
        SET_BIT(ppu->reg_stat, STAT_STATE_BIT1, 0);
        ppu->reg_ly = 0;
    }
    else if (GET_BIT(value, LCDC_PPU_ON_BIT) && !GET_BIT(ppu->reg_lcdc, LCDC_PPU_ON_BIT))
    {
        // PPU has been turned on

        ppu->cycles_counter = 0;
        ppu->lcdc_blocked = false;
        ppu->reg_ly = 0;

        // set hblank for 80 cycles
        SET_BIT(ppu->reg_stat, STAT_STATE_BIT0, 0);
        SET_BIT(ppu->reg_stat, STAT_STATE_BIT1, 0);

        ppu->next_state = STATE_DRAWING;
        ppu->clocks_to_next_state = STATE_OBJ_SEARCH_DURATION;
    }

    if (!GET_BIT(ppu->reg_lcdc, LCDC_WIN_ENABLE_BIT) && GET_BIT(value, LCDC_WIN_ENABLE_BIT))
	{
        // Window has been turned on
		ppu->window_line = GB_SCREEN_HEIGHT;
	}

    ppu->reg_lcdc = value;
}

void ppu_stat_write(gb_ppu_t *ppu, uint8_t value)
{
    assert(ppu != NULL);

    ppu->reg_stat = (ppu->reg_stat & 0x7) | (value & 0x78);
}

void ppu_ly_write(gb_ppu_t *ppu, uint8_t value)
{
    assert(ppu != NULL);

    // writing LY is not allowed
}

void ppu_lyc_write(gb_ppu_t *ppu, uint8_t value)
{
    assert(ppu != NULL);

    ppu->reg_lyc = value;
}

void ppu_scx_write(gb_ppu_t *ppu, uint8_t value)
{
    assert(ppu != NULL);

    ppu->reg_scx = value;
}

void ppu_scy_write(gb_ppu_t *ppu, uint8_t value)
{
    assert(ppu != NULL);

    ppu->reg_scy = value;
}

void ppu_wx_write(gb_ppu_t *ppu, uint8_t value)
{
    assert(ppu != NULL);

    ppu->reg_wx = value;
}

void ppu_wy_write(gb_ppu_t *ppu, uint8_t value)
{
    assert(ppu != NULL);

    ppu->delayed_wy = value;
}

void ppu_bgp_write(gb_ppu_t *ppu, uint8_t value)
{
    assert(ppu != NULL);

    ppu->reg_bgp = value;
}

void ppu_obp0_write(gb_ppu_t *ppu, uint8_t value)
{
    assert(ppu != NULL);

    ppu->reg_obp0 = value;
}

void ppu_obp1_write(gb_ppu_t *ppu, uint8_t value)
{
    assert(ppu != NULL);

    ppu->reg_obp1 = value;
}

void ppu_dma_write(gb_ppu_t *ppu, uint8_t value)
{
    assert(ppu != NULL);

    if (value > 0xDF)
        return;

    for (uint8_t i = 0; i < OAM_SIZE; i++)
        ppu->oam[i] = mmu_read(&ppu->gb->mmu, (value << 8) | i);
}

void ppu_vram_write(gb_ppu_t *ppu, uint16_t addr, uint8_t byte)
{
    assert(ppu != NULL);

    ppu->vram[addr - 0x8000] = byte;
}

void ppu_oam_write(gb_ppu_t *ppu, uint16_t addr, uint8_t byte)
{
    assert(ppu != NULL);

    ppu->oam[addr - 0xFE00] = byte;
}

void ppu_deinit(gb_ppu_t *ppu)
{
    assert(ppu != NULL);

    free(ppu->vram);
    free(ppu->oam);
    free(ppu->framebuffer);
    free(ppu->bg_scanline_buffer);
}

static void ppu_handle_lyc(gb_ppu_t *ppu)
{
    if (ppu->reg_ly == ppu->reg_lyc)
    {
        SET_BIT(ppu->reg_stat, STAT_LYC_FLAG_BIT, 1);

        if (GET_BIT(ppu->reg_stat, STAT_LYC_INT_BIT) && !ppu->lcdc_blocked)
        {
            int_request(&ppu->gb->intr_ctrl, INT_LCDC);
            ppu->lcdc_blocked = true;
        }
    }
    else
        SET_BIT(ppu->reg_stat, STAT_LYC_FLAG_BIT, 0);
}

static void ppu_render_scanline(gb_ppu_t *ppu)
{
    if (GET_BIT(ppu->reg_lcdc, LCDC_BG_WIN_ENABLE_BIT))
    {
        // Background is enabled, Window can be enabled

        ppu_render_bg_scanline(ppu);

        if (GET_BIT(ppu->reg_lcdc, LCDC_WIN_ENABLE_BIT))
            ppu_render_win_scanline(ppu);
    }
    else
    {
        // Clear scanline
        memset(&ppu->framebuffer[ppu->reg_ly * GB_SCREEN_WIDTH], 0, GB_SCREEN_WIDTH);
    }

    if (GET_BIT(ppu->reg_lcdc, LCDC_OBJ_ENABLE_BIT))
        ppu_render_obj_scanline(ppu);
}

static void ppu_render_bg_scanline(gb_ppu_t *ppu)
{
    uint16_t bg_tilemap_addr = GET_BIT(ppu->reg_lcdc, LCDC_BG_TILEMAP_BIT) ? TILEMAP1_ADDR : TILEMAP0_ADDR;

    int bg_line = (ppu->reg_scy + ppu->reg_ly) % (BG_HEIGHT * TILE_HEIGHT);

    int bg_tile_row    = bg_line / TILE_HEIGHT;
    int bg_tile_offs_y = bg_line % TILE_HEIGHT;

    for (int x = 0; x < GB_SCREEN_WIDTH; x++)
    {
        int bg_col = (ppu->reg_scx + x) % (BG_WIDTH * TILE_WIDTH);

        int bg_tile_col    = bg_col / TILE_WIDTH;
        int bg_tile_offs_x = bg_col % TILE_WIDTH;

        uint8_t tile_id = ppu->vram[bg_tilemap_addr + bg_tile_row * BG_WIDTH + bg_tile_col];

        // two addressing modes
        uint16_t tiledata_addr = 0;
        if (GET_BIT(ppu->reg_lcdc, LCDC_BG_WIN_TILEDATA_BIT))
            tiledata_addr = TILEDATA1_ADDR + tile_id * 16; // one tile takes 16 bytes
        else
            tiledata_addr = TILEDATA0_ADDR + (128 + (int8_t)tile_id) * 16;

        int pixel_pallete_id = GET_TILE_PIXEL(tiledata_addr, bg_tile_offs_y, bg_tile_offs_x);

        ppu->bg_scanline_buffer[x] = pixel_pallete_id;
        ppu->framebuffer[ppu->reg_ly * GB_SCREEN_WIDTH + x] = (ppu->reg_bgp >> (pixel_pallete_id * 2)) & 0x3;  
    }
}

static void ppu_render_win_scanline(gb_ppu_t *ppu)
{
    if (ppu->reg_wx < GB_SCREEN_WIDTH + WIN_GLOBAL_X_OFFSET &&
        ppu->reg_wy < GB_SCREEN_HEIGHT &&
        ppu->window_line < GB_SCREEN_HEIGHT &&
        ppu->reg_ly >= ppu->reg_wy)
    {
        uint16_t win_tilemap_addr = GET_BIT(ppu->reg_lcdc, LCDC_WIN_TILEMAP_BIT) ? TILEMAP1_ADDR : TILEMAP0_ADDR;

        int win_tile_row    = ppu->window_line / TILE_HEIGHT;
        int win_tile_offs_y = ppu->window_line % TILE_HEIGHT;

        int window_col = 0;
        if (ppu->reg_wx < WIN_GLOBAL_X_OFFSET)
            window_col = WIN_GLOBAL_X_OFFSET - ppu->reg_wx;

        int win_x_offs = ppu->reg_wx - WIN_GLOBAL_X_OFFSET;
        if (win_x_offs < 0)
            win_x_offs = 0;

        for (int x = win_x_offs; x < GB_SCREEN_WIDTH; x++)
        {
            int win_tile_col    = window_col / TILE_WIDTH;
            int win_tile_offs_x = window_col % TILE_WIDTH;

            uint8_t tile_id = ppu->vram[win_tilemap_addr + win_tile_row * BG_WIDTH + win_tile_col];

            // two addressing modes
            uint16_t tiledata_addr = 0;
            if (GET_BIT(ppu->reg_lcdc, LCDC_BG_WIN_TILEDATA_BIT))
                tiledata_addr = TILEDATA1_ADDR + tile_id * 16; // one tile takes 16 bytes
            else
                tiledata_addr = TILEDATA0_ADDR + (128 + (int8_t)tile_id) * 16;

            int pixel_pallete_id = GET_TILE_PIXEL(tiledata_addr, win_tile_offs_y, win_tile_offs_x);

            ppu->bg_scanline_buffer[x] = pixel_pallete_id;
            ppu->framebuffer[ppu->reg_ly * GB_SCREEN_WIDTH + x] = (ppu->reg_bgp >> (pixel_pallete_id * 2)) & 0x3;
        
            window_col++;
        }

        ppu->window_line++;
    }
}

static void ppu_render_obj_scanline(gb_ppu_t *ppu)
{
    for (int i = 0; i < ppu->line_sprite_count; i++)
    {        
        uint16_t obj_oam_addr = (ppu->sprite_draw_order[i] & 0xFF) * OAM_ENTRY_SIZE;

        int     obj_y_offs = ppu->oam[obj_oam_addr];
        int     obj_x_offs = ppu->oam[obj_oam_addr + 1];
        uint8_t tile_id    = ppu->oam[obj_oam_addr + 2];
        uint8_t flags      = ppu->oam[obj_oam_addr + 3];

        // Check sprite for visibility
        if (obj_x_offs == 0 || obj_x_offs >= GB_SCREEN_WIDTH + OBJ_GLOBAL_X_OFFSET)
            continue;

        uint16_t tiledata_addr = TILEDATA1_ADDR + tile_id * 16;
        uint8_t  obj_pallete   = GET_BIT(flags, OAM_ENTRY_PALLETE_BIT) ? ppu->reg_obp1 : ppu->reg_obp0;

        int obj_height = GET_BIT(ppu->reg_lcdc, LCDC_OBJ_SIZE_BIT) ? OBJ_HEIGHT_1 : OBJ_HEIGHT_0;

        bool flip_x   = GET_BIT(flags, OAM_ENTRY_FLIP_X_BIT);
        bool flip_y   = GET_BIT(flags, OAM_ENTRY_FLIP_Y_BIT);
        bool priority = GET_BIT(flags, OAM_ENTRY_PRIORITY_BIT);

        int obj_line = ppu->reg_ly - (obj_y_offs - OBJ_GLOBAL_Y_OFFSET);

        int obj_col = 0;
        if (flip_x)
        {
            obj_col = OBJ_WIDTH - 1;
            if (obj_x_offs > GB_SCREEN_WIDTH)
                obj_col -= (obj_x_offs - GB_SCREEN_WIDTH);
        }
        else
        {
            obj_col = 0;
            if (obj_x_offs < OBJ_GLOBAL_X_OFFSET)
                obj_col = OBJ_GLOBAL_X_OFFSET - obj_x_offs;
        }
        
        obj_x_offs -= OBJ_GLOBAL_X_OFFSET;
        if (obj_x_offs < 0)
            obj_x_offs = 0;

        if (flip_y)
            obj_line = obj_height - 1 - obj_line;

        for (int x = obj_x_offs; x < obj_x_offs + OBJ_WIDTH && x < GB_SCREEN_WIDTH; x++)
        {
            int pixel_pallete_id = GET_TILE_PIXEL(tiledata_addr, obj_line, obj_col);

            if ((priority && ppu->bg_scanline_buffer[x] == 0) || !priority)
            {
                if (pixel_pallete_id != 0)
                    ppu->framebuffer[ppu->reg_ly * GB_SCREEN_WIDTH + x] = (obj_pallete >> (pixel_pallete_id * 2)) & 0x3;
            }

            if (flip_x)
                obj_col--;
            else
                obj_col++;
        }
    }
}

static int ppu_draw_order_cmp(const void *a, const void *b)
{
    return *(int*)b - *(int*)a;
}

static void ppu_search_obj(gb_ppu_t *ppu)
{
    for (int i = 0; i < MAX_SPRITE_PER_LINE; i++)
        ppu->sprite_draw_order[i] = -1;

    ppu->line_sprite_count = 0;

    int obj_height = GET_BIT(ppu->reg_lcdc, LCDC_OBJ_SIZE_BIT) ? OBJ_HEIGHT_1 : OBJ_HEIGHT_0;

    // Less x coord - higher priority, less OAM index - higher priority if coords are same

    for (char i = 0; i < OAM_SIZE / OAM_ENTRY_SIZE; i++)
    {
        uint8_t obj_y_offs = ppu->oam[OAM_ENTRY_SIZE * i];
        uint8_t obj_x_offs = ppu->oam[OAM_ENTRY_SIZE * i + 1];

        if (obj_y_offs - OBJ_GLOBAL_Y_OFFSET <= ppu->reg_ly &&
            ppu->reg_ly < obj_y_offs - OBJ_GLOBAL_Y_OFFSET + obj_height)
        {
            // Less value - higher priority
            ppu->sprite_draw_order[ppu->line_sprite_count++] = (obj_x_offs << 8) | i;

            if (ppu->line_sprite_count >= MAX_SPRITE_PER_LINE)
                break;
        }
    }

    // Draw order - priorities from less to high
    qsort(ppu->sprite_draw_order, ppu->line_sprite_count, sizeof(int), ppu_draw_order_cmp);
}