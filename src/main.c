#include "tyler.h"
#include "mask_editor.h"
#include <stdlib.h>
#include <string.h>

#define MAX(A, B) ((A) > (B) ? (A) : (B))

extern int IsPointInRect(sgp_rect r, int x, int y);
extern void DrawMaskEditorBox(float x, float y, float w, float h, sg_color color);
    
typedef struct {
    sg_image texture;
    int width, height;
} Texture;

typedef struct {
    bool on;
    int mask;
} Tile;

static struct {
    struct {
        int tileW, tileH;
        Tile *grid;
        int width, height;
        bool showTooltip;
    } map;
    struct {
        float x, y, zoom;
    } camera;
    tyState ty;
} state;

static void InitMap(void) {
    state.map.width = 32;
    state.map.height = 32;
    state.map.tileW = 8;
    state.map.tileH = 8;
    state.map.showTooltip = false;
    size_t sz = state.map.width * state.map.height * sizeof(Tile);
    state.map.grid = malloc(sz);
    memset(state.map.grid, 0, sz);
}

static void DestroyMap(void) {
    if (state.map.grid)
        free(state.map.grid);
}


static int CheckMap(int x, int y) {
    return state.map.grid[y * state.map.width + x].on == true;
}

static void frame(void) {
    int width = sapp_width(), height = sapp_height();
    sgp_begin(width, height);
    
    sgp_set_color(.05f, .05f, .05f, 1.f);
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
    
    DrawMaskEditor(&state.ty);
    
    sgp_viewport(0, 0, width, height);
    sgp_project(0.f, width, 0, height);
    
    if (SAPP_MODIFIER_CHECK_ONLY(SAPP_MODIFIER_CTRL) && sapp_is_button_down(SAPP_MOUSEBUTTON_LEFT)) {
        state.camera.x -= sapp_cursor_delta_x();
        state.camera.y -= sapp_cursor_delta_y();
    }
    
    if (SAPP_MODIFIER_CHECK_ONLY(SAPP_MODIFIER_CTRL) && sapp_was_key_released(SAPP_KEYCODE_M))
        ToggleMaskEditor();

    int mx = sapp_cursor_x();
    int my = sapp_cursor_y();
    int sx = (((mx - (width/2)) + state.camera.x) / state.camera.zoom);
    int sy = ((my - (height/2)) + state.camera.y) / state.camera.zoom;
    int gx = sx / 8;
    int gy = sy / 8;
    
    sgp_push_transform();
    sgp_translate((width/2)-state.camera.x, (height/2)-state.camera.y);
    
    for (int x = 0; x < state.map.width; x++) {
        for (int y = 0; y < state.map.height; y++) {
            tyNeighbours mask;
            memset(&mask, 0, sizeof(tyNeighbours));
            mask.x = x;
            mask.y = y;
            for (int yy = 0; yy < 3; yy++)
                for (int xx = 0; xx < 3; xx++) {
                    int dx = x + (xx-1), dy = y + (yy-1);
                    mask.grid[yy * 3 + xx] = dx < 0 || dy < 0 || dx >= state.map.width || dy >=state.map.height ? 0 : state.map.grid[dy * state.map.width + dx].on;
                }
            state.map.grid[y * state.map.width + x].mask = tyBitmask(&mask, 1);
        }
    }
    
    for (int x = 0; x < state.map.width+1; x++) {
        float xx = x * state.map.tileW * state.camera.zoom;
        sgp_set_color(1.f, 1.f, 1.f, 1.f);
        sgp_draw_line(xx, 0, xx, (state.map.tileH*state.map.height)*state.camera.zoom);
        for (int y = 0; y < state.map.height+1; y++) {
            float yy = y * state.map.tileH * state.camera.zoom;
            Tile *tile = &state.map.grid[y * state.map.width + x];
            if (tile->on && x < state.map.width && y < state.map.height) {
                sgp_rect dst = {xx, yy, state.map.tileW*state.camera.zoom, state.map.tileH*state.camera.zoom};
                tyPoint *src = &state.ty.map[tile->mask];
                if (src->x == -1 || src->y == -1) {
                    sgp_draw_filled_rect(dst.x, dst.y, dst.w, dst.h);
                } else {
                    sgp_set_image(0, MaskEditorTexture());
                    sgp_draw_textured_rect(0, dst, (sgp_rect){src->x*state.map.tileW, src->y*state.map.tileH, state.map.tileW, state.map.tileH});
                    sgp_reset_image(0);
                }
            }
            sgp_draw_line(0, yy, (state.map.tileW*state.map.width)*state.camera.zoom, yy);
        }
    }
    if (IsPointInRect((sgp_rect){0,0,state.map.width,state.map.height}, gx, gy)) {
        int ox = (gx * state.map.tileW) * state.camera.zoom;
        int oy = (gy * state.map.tileH) * state.camera.zoom;
        int rw = state.map.tileW * state.camera.zoom;
        int rh = state.map.tileH * state.camera.zoom;
        DrawMaskEditorBox(ox, oy, rw, rh, (sg_color){1.f, 0.f, 0.f, 1.f});
        
        if (!igIsWindowHovered(ImGuiHoveredFlags_AnyWindow) && !sapp_any_modifiers() && SAPP_ANY_BUTTONS_DOWN(SAPP_MOUSEBUTTON_LEFT, SAPP_MOUSEBUTTON_RIGHT))
            state.map.grid[gy * state.map.width + gx].on = sapp_is_button_down(SAPP_MOUSEBUTTON_LEFT);
        
        state.map.showTooltip = true;
    } else
        state.map.showTooltip = false;
    
    sgp_reset_color();
    sgp_pop_transform();
    
    if (state.map.showTooltip && !igIsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
        if (igBegin("tooltip", &state.map.showTooltip, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration)) {
            uint8_t bitmask = state.map.grid[gy * state.map.width + gx].mask;
            char buf[9];
            memset(buf, 0, 9*sizeof(char));
            for (int i = 0; i < 8; i++)
                buf[i] = !!((bitmask << i) & 0x80) ? 'F' : '0';
            buf[8] = '\0';
            if (igBeginTooltip()) {
                igText("tile: %d, %d - mask: %d,0x%x,0b%s\n", gx, gy, bitmask, bitmask, buf);
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
    tyInit(&state.ty, CheckMap, state.map.width, state.map.height);
    state.camera.zoom = 4.f;
}

static void input(const sapp_event *e) {
    simgui_handle_event(e);
    if (!igIsWindowHovered(ImGuiHoveredFlags_AnyWindow))
        sokol_input_handler(e);
}

static void cleanup(void) {
    DestroyMap();
    DestroyMaskEditor();
    sgp_shutdown();
    simgui_shutdown();
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    return (sapp_desc) {
        .init_cb = init,
        .frame_cb = frame,
        .event_cb = input,
        .cleanup_cb = cleanup,
        .window_title = "tyler",
        .logger.func = slog_func,
    };
}
