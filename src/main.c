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

static uint8_t fake_bitmask(int t, int tr, int r, int br, int b, int bl, int l, int tl) {
    if (!t && !l) tl = 0;
    if (!t && !r) tr = 0;
    if (!b && !l) bl = 0;
    if (!b && !r) br = 0;
    uint8_t result = 0;
    if (tl) result += (1 << 0);
    if (t)  result += (1 << 1);
    if (tr) result += (1 << 2);
    if (l)  result += (1 << 3);
    if (r)  result += (1 << 4);
    if (bl) result += (1 << 5);
    if (b)  result += (1 << 6);
    if (br) result += (1 << 7);
    return result;
}

typedef bool TileBitmask[9];

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
    } mask;
    Map map;
} state;

static void InitMaskEditor(void) {
    state.mask.default_2x2.texture = sg_load_texture_path_ex("/Users/george/git/tyler/assets/default_2x2.png", &state.mask.default_2x2.width, &state.mask.default_2x2.height);
    state.mask.default_3x3.texture = sg_load_texture_path_ex("/Users/george/git/tyler/assets/default_3x3.png", &state.mask.default_3x3.width, &state.mask.default_3x3.height);
    state.mask.currentAtlas = &state.mask.default_3x3;
    
    state.mask.open = true;
    state.mask.scale = 4.f;
    
    state.mask.spriteW = state.mask.spriteH = 8;
    state.mask.spriteX = state.mask.default_3x3.width / state.mask.spriteW;
    state.mask.spriteY = state.mask.default_3x3.height / state.mask.spriteH;
    size_t sz = state.mask.spriteX * state.mask.spriteY * sizeof(TileBitmask);
    state.mask.grid = malloc(sz);
    memset(state.mask.grid, 0, sz);
    for (int x = 0; x < state.mask.spriteX; x++)
        for (int y = 0; y < state.mask.spriteY; y++)
            state.mask.grid[y * 3 + x][4] = true;
}

static void DestroyMaskEditor(void) {
    sokol_helper_destroy(state.mask.default_2x2.texture);
    sokol_helper_destroy(state.mask.default_3x3.texture);
    if (state.mask.grid)
        free(state.mask.grid);
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
    sgp_set_color(1.f, 0.f, 0.f, 1.f);
    sgp_set_blend_mode(SGP_BLENDMODE_NONE);
    sgp_reset_color();
    sgp_pop_transform();
    sgp_reset_image(0);
    sgp_reset_sampler(0);
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
    igSetNextWindowSize((ImVec2){800, 400}, ImGuiCond_Once);
    if (state.mask.open && igBegin("Mask Editor", &state.mask.open, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (igBeginChild_Str("Mask Editor", (ImVec2){state.mask.currentAtlas->width*state.mask.scale, state.mask.currentAtlas->height*state.mask.scale}, true, ImGuiWindowFlags_None)) {
            ImDrawList* dl = igGetWindowDrawList();
            ImDrawList_AddCallback(dl, igDrawMaskEditorCb, NULL);
        }
        igEndChild();
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
