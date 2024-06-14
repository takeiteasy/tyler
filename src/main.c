#include "tyler.h"
#include "mask_editor.h"
#include "map_editor.h"
#include "new_window.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    float x, y;
    float zoom;
} Camera;

static struct {
    Camera camera;
    int mapW, mapH;
    int tileW, tileH;
    int mouseX, mouseY;
    int worldX, worldY;
    bool showCameraInfo;
    bool showTooltip;
    bool showNewWindow;
    tyState ty;
} state;

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
    
    memset(&state.camera, 0, sizeof(Camera));
    state.camera.zoom = 16.f;
    state.mapW = 32;
    state.mapH = 32;
    state.tileW = 8;
    state.tileH = 8;
    InitMap(state.mapW, state.mapH);
    InitMaskEditor();
    tyInit(&state.ty, CheckMap, state.mapW, state.mapH);
    state.showCameraInfo = false;
    state.showTooltip = true;
    state.showNewWindow = true;
}

static void input(const sapp_event *e) {
    if (!simgui_handle_event(e) && !igIsWindowHovered(ImGuiHoveredFlags_AnyWindow))
        sokol_input_handler(e);
}

static void cleanup(void) {
    DestroyMaskEditor();
    sgp_shutdown();
    simgui_shutdown();
    sg_shutdown();
}

static float Remap(float value, float in_min, float in_max, float out_min, float out_max) {
    return in_min == in_max ? NAN : out_min + (out_max - out_min) * fmax(0.0f, fmin(1.0f, (value - in_min) / (in_max - in_min)));
}

static void frame(void) {
    int width = sapp_width(), height = sapp_height();
    double delta = sapp_frame_duration();
    sgp_begin(width, height);
    sgp_viewport(0, 0, width, height);
    sgp_set_color(.05f, .05f, .05f, 1.f);
    sgp_clear();
    sgp_reset_color();
    sgp_set_blend_mode(SGP_BLENDMODE_BLEND);
    
    simgui_frame_desc_t igFrameDesc = {
        .width = width,
        .height = height,
        .delta_time = delta,
        .dpi_scale = sapp_dpi_scale()
    };
    simgui_new_frame(&igFrameDesc);
    
    igSetNextWindowPos((ImVec2){0,0}, ImGuiCond_Once, (ImVec2){0,0});
    igSetNextWindowSize((ImVec2){width, 10}, ImGuiCond_Once);
    igBegin("Main Menu", NULL, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
    if (igBeginMenuBar()) {
        if (igBeginMenu("File", true)) {
            if (igMenuItemEx("New", NULL, "Ctrl+N", false, true))
                ShowNewWindow();
            if (igMenuItemEx("Open", NULL, "Ctrl+O", false, true)) {}
            if (igMenuItemEx("Save", NULL, "Ctrl+S", false, true)) {}
            igEndMenu();
        }
        igEndMenuBar();
    }
    if (igBeginMenuBar()) {
        if (igBeginMenu("View", true)) {
            if (igMenuItem_BoolPtr("Map Editor", "Ctrl+E", MapEditorIsOpen(), true)) {}
            if (igMenuItem_BoolPtr("Mask Editor", "Ctrl+M", MaskEditorIsOpen(), true)) {}
            if (igMenuItem_BoolPtr("Camera Info", "Ctrl+I", &state.showCameraInfo, true)) {}
            if (igMenuItem_BoolPtr("Show Tooltip", NULL, &state.showTooltip, true)) {}
            if (igMenuItem_BoolPtr("Draw Grid", "Ctrl+G", MapDrawGrid(), true)) {}
            igEndMenu();
        }
        igEndMenuBar();
    }
    igEnd();

    DrawMaskEditor(&state.ty);
    
    if (SAPP_MODIFIER_CHECK_ONLY(SAPP_MODIFIER_CTRL) &&
        sapp_was_key_released(SAPP_KEYCODE_E))
        ToggleMapEditor();
    
    if (SAPP_MODIFIER_CHECK_ONLY(SAPP_MODIFIER_CTRL) &&
        sapp_was_key_released(SAPP_KEYCODE_N))
        ShowNewWindow();

    if (SAPP_MODIFIER_CHECK_ONLY(SAPP_MODIFIER_CTRL) &&
        sapp_was_key_released(SAPP_KEYCODE_M))
        ToggleMaskEditor();
    
    if (SAPP_MODIFIER_CHECK_ONLY(SAPP_MODIFIER_CTRL) &&
        sapp_was_key_released(SAPP_KEYCODE_I))
        state.showCameraInfo = !state.showCameraInfo;
    
    if (SAPP_MODIFIER_CHECK_ONLY(SAPP_MODIFIER_CTRL) &&
        sapp_was_key_released(SAPP_KEYCODE_G))
        ToggleGrid();
        
    if (SAPP_MODIFIER_CHECK_ONLY(SAPP_MODIFIER_CTRL) && sapp_check_scrolled())
        state.camera.zoom = CLAMP(state.camera.zoom - sapp_scroll_y(), 16.f, 512.f);
    
    if (SAPP_MODIFIER_CHECK_ONLY(SAPP_MODIFIER_CTRL) && sapp_is_button_down(SAPP_MOUSEBUTTON_LEFT)) {
        float scale = Remap(state.camera.zoom, 16.f, 512.f, 5.f, 100.f);
        state.camera.x -= (sapp_cursor_delta_x() * scale) * delta;
        state.camera.y -= (sapp_cursor_delta_y() * scale) * delta;
    }

    float aspectRatio = width/(float)height;
    float viewVolumeLeft = -state.camera.zoom * aspectRatio;
    float viewVolumeRight = state.camera.zoom * aspectRatio;
    float viewVolumeBottom = -state.camera.zoom;
    float viewVolumeTop = state.camera.zoom;
    float adjustedLeft = viewVolumeLeft + state.camera.x;
    float adjustedRight = viewVolumeRight + state.camera.x;
    float adjustedBottom = viewVolumeBottom + state.camera.y;
    float adjustedTop = viewVolumeTop + state.camera.y;
    sgp_project(adjustedLeft, adjustedRight, adjustedBottom, adjustedTop);
    state.mouseX = sapp_cursor_x();
    state.mouseY = sapp_cursor_y();
    state.worldX = state.mouseX * (viewVolumeRight - viewVolumeLeft) / width + viewVolumeLeft;
    state.worldY = state.mouseY * (viewVolumeTop - viewVolumeBottom) / height + viewVolumeBottom;
    state.worldX = (int)((state.worldX + state.camera.x) / state.tileW);
    state.worldY = (int)((state.worldY + state.camera.y) / state.tileH);
    
    DrawMap(&state.ty, state.worldX, state.worldY);
    
    if (state.showCameraInfo) {
        if (igBegin("Camera", &state.showCameraInfo, ImGuiWindowFlags_AlwaysAutoResize)) {
            igText("Position: %f, %f", state.camera.x, state.camera.y);
            igText("Zoom: %f", state.camera.zoom);
            igText("Mouse: %d, %d", state.mouseX, state.mouseY);
            igText("Grid: %d, %d", state.worldX, state.worldY);
        }
        igEnd();
    }
    
    DrawNewWindow();
    
    if (state.worldX >= 0 && state.worldY >= 0 &&
        state.worldX < state.mapW && state.worldY < state.mapH) {
        if (!igIsWindowHovered(ImGuiHoveredFlags_AnyWindow) && !sapp_any_modifiers() && SAPP_ANY_BUTTONS_DOWN(SAPP_MOUSEBUTTON_LEFT, SAPP_MOUSEBUTTON_RIGHT))
            SetMap(state.worldX, state.worldY, sapp_is_button_down(SAPP_MOUSEBUTTON_LEFT));
        
        if (state.showTooltip && !igIsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
            if (igBegin("tooltip", &state.showTooltip, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration)) {
                tyNeighbours neighbours;
                tyReadNeighbours(&state.ty, state.worldX, state.worldY, &neighbours);
                uint8_t bitmask = tyBitmask(&neighbours, 0);
                char buf[9];
                memset(buf, 0, 9*sizeof(char));
                for (int i = 0; i < 8; i++)
                    buf[i] = !!((bitmask << i) & 0x80) ? 'F' : '0';
                buf[8] = '\0';
                if (igBeginTooltip()) {
                    igText("tile: %d, %d - mask: %d,0x%x,0b%s\n", state.worldX, state.worldY, bitmask, bitmask, buf);
                    igEndTooltip();
                }
                igEnd();
            }
        }
    }
    
    sgp_reset_color();
    sg_pass pass = {.swapchain = sglue_swapchain()};
    sg_begin_pass(&pass);
    sgp_flush();
    simgui_render();
    sgp_flush();
    sgp_end();
    sg_end_pass();
    sg_commit();
    sokol_input_update();
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
