#include "map.h"
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
        for (int x = 0; x < 3; x++) {
            if (!(y == 1 && x == 1) && mask->grid[y * 3 + x])
                result += (1 << n++);
            else
                n++;
        }
    return result;
}

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
        TileBitmask *currentBitmask;
        Texture numberAtlas;
    } mask;
    Map map;
} state;

static void InitMaskEditor(void) {
    state.mask.default_2x2.texture = sg_load_texture_path_ex("/Users/george/git/tyler/assets/default_2x2.png", &state.mask.default_2x2.width, &state.mask.default_2x2.height);
    state.mask.default_3x3.texture = sg_load_texture_path_ex("/Users/george/git/tyler/assets/default_3x3.png", &state.mask.default_3x3.width, &state.mask.default_3x3.height);
    state.mask.numberAtlas.texture = sg_load_texture_path_ex("/Users/george/git/tyler/assets/hex.png", &state.mask.numberAtlas.width, &state.mask.numberAtlas.height);
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

int is_point_inside_rect(sgp_rect r, int x, int y) {
  return (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h);
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
    if (is_point_inside_rect((sgp_rect){cx, cy, cw, ch}, mx, my)) {
        int gw = state.mask.spriteW*state.mask.scale;
        int gh = state.mask.spriteH*state.mask.scale;
        int gx = (mx - cx)/gw;
        int gy = (my - cy)/gh;
        int ox = gx * gw;
        int oy = gy * gh;
        DrawMaskEditorBox(ox, oy, gw, gh, (sg_color){1.f, 1.f, 1.f, 1.f});
    }
    
    if (state.mask.currentBitmask != NULL) {
        int gw = state.mask.spriteW*state.mask.scale;
        int gh = state.mask.spriteH*state.mask.scale;
        float ox = state.mask.currentBitmask->x * gw;
        float oy = state.mask.currentBitmask->y * gh;
        DrawMaskEditorBox(ox, oy, gw, gh, (sg_color){1.f, 0.f, 0.f, 1.f});
    }
    
    sgp_set_image(0, state.mask.numberAtlas.texture);
    for (int x = 0, i = 0; x < state.mask.spriteX; x++)
        for (int y = 0; y < state.mask.spriteY; y++) {
            float dx = ((x * state.mask.spriteW)*state.mask.scale);
            float dy = ((y * state.mask.spriteH)*state.mask.scale);
            char buf[8];
            snprintf(buf, 8, "%x", i);
            for (int j = 0; j < 8; j++) {
                char p = buf[j];
                if (p == '\0')
                    break;
                sgp_rect dst = {dx+(j*8), dy, state.mask.spriteW, state.mask.spriteH};
                sgp_rect src = {i++ * 8, 0, 8, 8};
                sgp_draw_textured_rect(0, dst, src);
            }
        }
    sgp_reset_image(0);
    sgp_flush();
    sgp_end();
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
    igSetNextWindowSize(maskEditSize, ImGuiCond_Once);
    if (state.mask.open && igBegin("Mask Editor", &state.mask.open, ImGuiWindowFlags_AlwaysAutoResize)) {
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
            igEndChild();
        }
        
        if (state.mask.currentBitmask) {
            maskEditSize.y += 10;
            if (igBeginChild_Str("Button Grid", maskEditSize, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
                igText("Tile (%d,%d)", state.mask.currentBitmask->x, state.mask.currentBitmask->y);
                igBeginTable("button_grid", 3, ImGuiTableFlags_NoBordersInBody, maskEditSize, gw);
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
            igEndChild();
        }
        igEnd();
    }
    
    sgp_viewport(0, 0, width, height);
    float ratio = width/(float)height;
    sgp_project(-ratio, ratio, 1.0f, -1.0f);
    float time = sapp_frame_count() * sapp_frame_duration();
    float r = sinf(time)*0.5+0.5, g = cosf(time)*0.5+0.5;
    sgp_push_transform();
    sgp_set_color(r, g, 0.3f, 1.0f);
    sgp_rotate_at(time, 0.0f, 0.0f);
    sgp_draw_filled_rect(-0.5f, -0.5f, 1.0f, 1.0f);
    sgp_pop_transform();
    sgp_reset_color();

    sg_pass pass = {.swapchain = sglue_swapchain()};
    sg_begin_pass(&pass);
    sgp_flush();
    simgui_render();
    sgp_end();
    sg_end_pass();
    sg_commit();
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
    InitMap(&state.map, 64, 64);
}

static void input(const sapp_event *e) {
    simgui_handle_event(e);
    sokol_input_handler(e);
}

static void cleanup(void) {
    DestroyMap(&state.map);
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
