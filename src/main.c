#define SOKOL_HELPER_IMPL
#include "sokol_helpers/sokol_input.h"
#include "sokol_helpers/sokol_img.h"
#include "sokol_helpers/sokol_scalefactor.h"
#include "sokol_helpers/sokol_generic.h"
#define SOKOL_IMPL
#include "sokol_gfx.h"
#include "sokol_gp.h"
#include "sokol_app.h"
#include "sokol_glue.h"
#include "sokol_log.h"
#include "sokol_time.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui/cimgui.h"
#include "sokol_imgui.h"

typedef struct {
    sg_image texture;
    int width, height;
} Texture;

enum {
    TILE_MASK_TL = 0,
    TILE_MASK_T,
    TILE_MASK_TR,
    TILE_MASK_L,
    TILE_MASK_IGNORE,
    TILE_MASK_R,
    TILE_MASK_BL,
    TILE_MASK_B,
    TILE_MASK_BR
};

typedef struct {
    bool grid[9];
    int x, y;
} TileBitmask;

static uint8_t Bitmask(TileBitmask *mask) {
#define CHECK_CORNER(N, A, B) \
    mask->grid[(N)] = !mask->grid[(A)] && !mask->grid[(B)] ? false : mask->grid[(N)];
    CHECK_CORNER(0, 1, 3);
    CHECK_CORNER(2, 1, 5);
    CHECK_CORNER(6, 7, 3);
    CHECK_CORNER(8, 7, 5);
    uint8_t result = 0;
    for (int y = 0, n = 0; y < 3; y++)
        for (int x = 0; x < 3; x++, n++)
            if (!(y == 1 && x == 1))
                result += (mask->grid[y * 3 + x] << n);
    return result;
}

typedef struct {
    bool on;
    int mask;
} Tile;

static struct {
    struct {
        TileBitmask *grid;
        float scale;
        bool open;
        int spriteW, spriteX;
        int spriteH, spriteY;
        Texture default_2x2;
        Texture default_3x3;
        Texture *currentAtlas;
        sg_image cross;
        TileBitmask *currentBitmask;
    } mask;
    struct {
        Tile grid[64*64];
        int width, height;
        bool showTooltip;
    } map;
    struct {
        float x, y, zoom;
    } camera;
} state;

static void InitMap(void) {
    state.map.width = 32;
    state.map.height = 32;
    state.map.showTooltip = false;
    memset(state.map.grid, 0, state.map.width*state.map.height*sizeof(Tile));
}

static void DestroyMap(void) {
    // ...
}

static void InitMaskEditor(void) {
    state.mask.default_2x2.texture = sg_load_texture_path_ex("/Users/george/git/tyler/assets/default_2x2.png", &state.mask.default_2x2.width, &state.mask.default_2x2.height);
    state.mask.default_3x3.texture = sg_load_texture_path_ex("/Users/george/git/tyler/assets/default_3x3.png", &state.mask.default_3x3.width, &state.mask.default_3x3.height);
    state.mask.cross = sg_load_texture_path("/Users/george/git/tyler/assets/x.png");
    state.mask.currentAtlas = &state.mask.default_3x3;
    
    state.mask.open = true;
    state.mask.scale = 4.f;
    
    state.mask.spriteW = state.mask.spriteH = 8;
    state.mask.spriteX = state.mask.default_3x3.width / state.mask.spriteW;
    state.mask.spriteY = state.mask.default_3x3.height / state.mask.spriteH;
    size_t sz = (state.mask.spriteX * state.mask.spriteY) * sizeof(TileBitmask);
    state.mask.grid = malloc(sz);
    memset(state.mask.grid, 0, sz);
    for (int x = 0; x < state.mask.spriteX; x++)
        for (int y = 0; y < state.mask.spriteY; y++) {
            TileBitmask *mask = &state.mask.grid[y * state.mask.spriteX + x];
            mask->grid[TILE_MASK_IGNORE] = true;
            mask->x = x;
            mask->y = y;
        }
    state.mask.currentBitmask = NULL;
}

static void DestroyMaskEditor(void) {
    sokol_helper_destroy(state.mask.default_2x2.texture);
    sokol_helper_destroy(state.mask.default_3x3.texture);
    if (state.mask.grid)
        free(state.mask.grid);
}

int IsPointInRect(sgp_rect r, int x, int y) {
  return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

static void DrawMaskEditorBox(float x, float y, float w, float h, sg_color color) {
    sgp_set_color(color.r, color.g, color.b, color.a);
    sgp_draw_line(x, y, x + w, y);
    sgp_draw_line(x, y, x, y + h);
    sgp_draw_line(x + w, y, x + w, y + h);
    sgp_draw_line(x, y + h, x + w, y + h);
    sgp_reset_color();
}

static void igDrawMaskEditorCb(const ImDrawList* dl, const ImDrawCmd* cmd) {
    int sw = state.mask.currentAtlas->width*state.mask.scale;
    int sh = state.mask.currentAtlas->height*state.mask.scale;
    int cx = (int) cmd->ClipRect.x;
    int cy = (int) cmd->ClipRect.y;
    int cw = (int) (cmd->ClipRect.z - cmd->ClipRect.x);
    int ch = (int) (cmd->ClipRect.w - cmd->ClipRect.y);
    sgp_scissor(cx, cy, cw, ch);
    cx = cx ? cx : cw - sw;
    cy = cy ? cy : ch - sh;
    sgp_viewport(cx, cy, sw, sh);
    sgp_set_image(0, state.mask.currentAtlas->texture);
    sgp_push_transform();
    sgp_scale(state.mask.scale, state.mask.scale);
    sgp_draw_filled_rect(0, 0, state.mask.currentAtlas->width, state.mask.currentAtlas->height);
    sgp_pop_transform();
    sgp_reset_image(0);
    
    int mx = sapp_cursor_x(), my = sapp_cursor_y();
    if (IsPointInRect((sgp_rect){cx, cy, cw, ch}, mx, my)) {
        int gw = state.mask.spriteW*state.mask.scale;
        int gh = state.mask.spriteH*state.mask.scale;
        int gx = (mx - cx)/gw;
        int gy = (my - cy)/gh;
        int ox = MAX(gx * gw, 1);
        int oy = MAX(gy * gh, 1);
        int dx = ox + gw >= cw ? gw-2 : gw;
        int dy = oy + gh >= ch ? gh-2 : gh;
        DrawMaskEditorBox(ox, oy, dx, dy, (sg_color){1.f, 1.f, 1.f, 1.f});
    }
    
    if (state.mask.currentBitmask != NULL) {
        int gw = state.mask.spriteW*state.mask.scale;
        int gh = state.mask.spriteH*state.mask.scale;
        float ox = MAX(state.mask.currentBitmask->x * gw, 1);
        float oy = MAX(state.mask.currentBitmask->y * gh, 1);
        int dx = ox + gw >= cw ? gw-2 : gw;
        int dy = oy + gh >= ch ? gh-2 : gh;
        DrawMaskEditorBox(ox, oy, dx, dy, (sg_color){1.f, 0.f, 0.f, 1.f});
    }
    
    sgp_set_image(0, state.mask.cross);
    for (int x = 0; x < state.mask.spriteX; x++)
        for (int y = 0; y < state.mask.spriteY; y++) {
            float dx = ((x * state.mask.spriteW)*state.mask.scale);
            float dy = ((y * state.mask.spriteH)*state.mask.scale);
            int gw = state.mask.spriteW*state.mask.scale;
            int gh = state.mask.spriteH*state.mask.scale;
            float ex = gw/3, ey = gh/3;
            for (int yy = 0; yy < 3; yy++)
                for (int xx = 0; xx < 3; xx++)
                    if (!(xx == 1 && yy == 1) && state.mask.grid[y * state.mask.spriteX + x].grid[yy * 3 + xx]) {
                        sgp_rect dst = {dx+(yy*ey)+2, dy+(xx*ex)+2, state.mask.spriteW, state.mask.spriteH};
                        sgp_rect src = {0, 0, 8, 8};
                        sgp_draw_textured_rect(0, dst, src);
                    }
        }
    sgp_reset_image(0);
    sgp_flush();
}


static void UpdateMap(void) {
    for (int x = 0; x < state.map.width; x++) {
        for (int y = 0; y < state.map.height; y++) {
            TileBitmask mask;
            memset(&mask, 0, sizeof(TileBitmask));
            mask.x = x;
            mask.y = y;
            for (int yy = 0; yy < 3; yy++)
                for (int xx = 0; xx < 3; xx++) {
                    int dx = x + (xx-1), dy = y + (yy-1);
                    mask.grid[yy * 3 + xx] = dx < 0 || dy < 0 || dx >= state.map.width || dy >=state.map.height ? 0 : state.map.grid[dy * state.map.width + dx].on;
                }
            state.map.grid[y * state.map.width + x].mask = Bitmask(&mask);
        }
    }
}

static void frame(void) {
    int width = sapp_width(), height = sapp_height();
    sgp_begin(width, height);
    
    sgp_set_color(0.05f, 0.05f, 0.05f, 1.0f);
    sgp_clear();
    sgp_reset_color();
    sgp_set_blend_mode(SGP_BLENDMODE_BLEND);
    
    simgui_frame_desc_t igFrameDesc = {
        .width = width,
        .height = height,
        .delta_time = sapp_frame_duration(),
        .dpi_scale = sapp_dpi_scale()
    };
    simgui_new_frame(&igFrameDesc);
    
    igSetNextWindowPos((ImVec2){20, 20}, ImGuiCond_Once, (ImVec2){0,0});
    ImVec2 maskEditSize = (ImVec2) {
        .x = state.mask.currentAtlas->width*state.mask.scale,
        .y = state.mask.currentAtlas->height*state.mask.scale
    };
    int gw = state.mask.spriteW*state.mask.scale;
    int gh = state.mask.spriteH*state.mask.scale;
    
    
    if (state.mask.open) {
        igSetNextWindowSize(maskEditSize, ImGuiCond_Once);
        if (igBegin("Mask Editor", &state.mask.open, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (igBeginChild_Str("Mask Editor", maskEditSize, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
                ImDrawList* dl = igGetWindowDrawList();
                ImDrawList_AddCallback(dl, igDrawMaskEditorCb, NULL);
                for (int x = 0; x < state.mask.spriteX; x++)
                    for (int y = 0; y < state.mask.spriteY; y++) {
                        igSetCursorPos((ImVec2){x*gw, y*gh});
                        char buf[32];
                        snprintf(buf, 32, "grid%dx%d", x, y);
                        if (igInvisibleButton(buf, (ImVec2){gw, gh}, ImGuiButtonFlags_MouseButtonLeft)) {
                            if (state.mask.currentBitmask)
                                if (x == state.mask.currentBitmask->x &&
                                    y == state.mask.currentBitmask->y) {
                                    state.mask.currentBitmask = NULL;
                                    continue;
                                }
                            state.mask.currentBitmask = &state.mask.grid[y * state.mask.spriteX + x];
                        }
                    }
            }
            igEndChild();

            if (state.mask.currentBitmask) {
                maskEditSize.y += 10;
                if (igBeginChild_Str("Button Grid", maskEditSize, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
                    igText("Tile (%d,%d)", state.mask.currentBitmask->x, state.mask.currentBitmask->y);
                    if (igBeginTable("button_grid", 3, ImGuiTableFlags_NoBordersInBody, maskEditSize, gw)) {
                        for (int x = 0; x < 3; x++) {
                            igTableNextRow(ImGuiTableRowFlags_None, gh);
                            for (int y = 0; y < 3; y++) {
                                igTableSetColumnIndex(y);
                                static const char *labels[9] = {
                                    "TL", "L", "BL", "T", "X", "B", "TR", "R", "BR"
                                };
                                if (x == 1 && y == 1) {
                                    uint8_t bitmask = Bitmask(state.mask.currentBitmask);
                                    char buf[9];
                                    for (int i = 0; i < 8; i++)
                                        buf[i] = !!((bitmask << i) & 0x80) ? 'F' : '0';
                                    buf[8] = '\0';
                                    igText("mask: 0b%s", buf);
                                } else {
                                    bool b = state.mask.currentBitmask->grid[y * 3 + x];
                                    igPushStyleColor_Vec4(ImGuiCol_Button, b ? (ImVec4){0.f, 1.f, 0.f, 1.f} : (ImVec4){1.f, 0.f, 0.f, 1.f});
                                    if (igButton(labels[y * 3 + x], (ImVec2){gw, gh}))
                                        state.mask.currentBitmask->grid[y * 3 + x] = !b;
                                    igPopStyleColor(1);
                                }
                            }
                        }
                        igEndTable();
                    }
                }
                igEndChild();
            }
        }
        igEnd();
    }
    
    sgp_viewport(0, 0, width, height);
    sgp_project(0.f, width, 0, height);
    
    if (SAPP_MODIFIER_CHECK_ONLY(SAPP_MODIFIER_CTRL) && sapp_is_button_down(SAPP_MOUSEBUTTON_LEFT)) {
        state.camera.x -= sapp_cursor_delta_x();
        state.camera.y -= sapp_cursor_delta_y();
    }
    
    if (SAPP_MODIFIER_CHECK_ONLY(SAPP_MODIFIER_CTRL) && sapp_was_key_released(SAPP_KEYCODE_M))
        state.mask.open = !state.mask.open;

    int mx = sapp_cursor_x();
    int my = sapp_cursor_y();
    int sx = (((mx - (width/2)) + state.camera.x) / state.mask.scale);
    int sy = ((my - (height/2)) + state.camera.y) / state.mask.scale;
    int gx = sx / 8;
    int gy = sy / 8;
    
    sgp_push_transform();
    sgp_translate((width/2)-state.camera.x, (height/2)-state.camera.y);
    
    sgp_set_color(1.f, 1.f, 1.f, 1.f);
    for (int x = 0; x < state.map.width+1; x++) {
        float xx = x * state.mask.spriteW * state.mask.scale;
        sgp_draw_line(xx, 0, xx, (state.mask.spriteH*state.map.height)*state.mask.scale);
        for (int y = 0; y < state.map.height+1; y++) {
            float yy = y * state.mask.spriteH * state.mask.scale;
            if (state.map.grid[y * state.map.width + x].on &&
                x < state.map.width && y < state.map.height)
                sgp_draw_filled_rect(xx, yy, state.mask.spriteW*state.mask.scale, state.mask.spriteH*state.mask.scale);
            sgp_draw_line(0, yy, (state.mask.spriteW*state.map.width)*state.mask.scale, yy);
        }
    }
    if (IsPointInRect((sgp_rect){0,0,state.map.width,state.map.height}, gx, gy)) {
        int ox = (gx * state.mask.spriteW) * state.mask.scale;
        int oy = (gy * state.mask.spriteH) * state.mask.scale;
        int rw = state.mask.spriteW * state.mask.scale;
        int rh = state.mask.spriteH * state.mask.scale;
        DrawMaskEditorBox(ox, oy, rw, rh, (sg_color){1.f, 0.f, 0.f, 1.f});
        
        if (!sapp_any_modifiers() && SAPP_ANY_BUTTONS_DOWN(SAPP_MOUSEBUTTON_LEFT, SAPP_MOUSEBUTTON_RIGHT))
            state.map.grid[gy * state.map.width + gx].on = sapp_is_button_down(SAPP_MOUSEBUTTON_LEFT);
        
        state.map.showTooltip = true;
    } else
        state.map.showTooltip = false;
    
    sgp_reset_color();
    sgp_pop_transform();
    
    if (state.map.showTooltip && !igIsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
        if (igBegin("tooltip", &state.mask.open, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration)) {
            uint8_t bitmask = state.map.grid[gy * state.map.width + gx].mask;
            char buf[9];
            memset(buf, 0, 9*sizeof(char));
            for (int i = 0; i < 8; i++)
                buf[i] = !!((bitmask << i) & 0x80) ? 'F' : '0';
            buf[8] = '\0';
            if (igBeginTooltip()) {
                igText("%d, %d: mask: 0b%s\n", gx, gy, buf);
                igEndTooltip();
            }
        }
        igEnd();
    }
    
    sg_pass pass = {.swapchain = sglue_swapchain()};
    sg_begin_pass(&pass);
    sgp_flush();
    simgui_render();
    sgp_end();
    sg_end_pass();
    sg_commit();
    sokol_input_update();
    UpdateMap();
}

static void init(void) {
    sg_desc sgdesc = {
        .environment = sglue_environment(),
        .logger.func = slog_func
    };
    sg_setup(&sgdesc);
    if (!sg_isvalid()) {
        fprintf(stderr, "Failed to create Sokol GFX context!\n");
        exit(-1);
    }

    sgp_desc sgpdesc = {0};
    sgp_setup(&sgpdesc);
    if (!sgp_is_valid()) {
        fprintf(stderr, "Failed to create Sokol GP context: %s\n", sgp_get_error_message(sgp_get_last_error()));
        exit(-1);
    }
    
    simgui_desc_t imgui_desc = {
        .logger.func = slog_func,
    };
    simgui_setup(&imgui_desc);
    
    stm_setup();
    
    InitMaskEditor();
    InitMap();
}

static void input(const sapp_event *e) {
    simgui_handle_event(e);
    sokol_input_handler(e);
}

static void cleanup(void) {
    DestroyMap();
    DestroyMaskEditor();
    sgp_shutdown();
    simgui_shutdown();
    sg_shutdown();
}

// Implement application main through Sokol APP.
sapp_desc sokol_main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .event_cb = input,
        .cleanup_cb = cleanup,
        .window_title = "tyler",
        .logger.func = slog_func,
    };
}
