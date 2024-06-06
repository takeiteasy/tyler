#include "map.h"
#define SOKOL_HELPER_IMPL
#include "sokol_helpers/sokol_input.h"
#include "sokol_helpers/sokol_img.h"
#include "sokol_helpers/sokol_scalefactor.h"
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

static struct {
    sg_image default_2x2;
    sg_image default_3x3;
    Map map;
} state;

// Called on every frame of the application.
static void frame(void) {
    // Get current window size.
    int width = sapp_width(), height = sapp_height();
    int fb_width = sapp_framebuffer_width(), fb_height = sapp_framebuffer_height();
//    float ratio = width/(float)height;
    
    // Begin recording draw commands for a frame buffer of size (width, height).
    sgp_begin(width, height);
    // Set frame buffer drawing region to (0,0,width,height).
    sgp_viewport(0, 0, fb_width, fb_height);
    // Set drawing coordinate space to (left=-ratio, right=ratio, top=1, bottom=-1).
    sgp_project(-1.f, 1.f, 1.0f, -1.0f);
    
    // Clear the frame buffer.
    sgp_set_color(0.1f, 0.1f, 0.1f, 1.0f);
    sgp_clear();
    
    // Begin a render pass.
    sg_pass pass = {.swapchain = sglue_swapchain()};
    sg_begin_pass(&pass);
    // Dispatch all draw commands to Sokol GFX.
    sgp_flush();
    // Finish a draw command queue, clearing it.
    sgp_end();
    // End render pass.
    sg_end_pass();
    // Commit Sokol render.
    sg_commit();
}

// Called when the application is initializing.
static void init(void) {
    // Initialize Sokol GFX.
    sg_desc sgdesc = {
        .environment = sglue_environment(),
        .logger.func = slog_func
    };
    sg_setup(&sgdesc);
    if (!sg_isvalid()) {
        fprintf(stderr, "Failed to create Sokol GFX context!\n");
        exit(-1);
    }

    // Initialize Sokol GP, adjust the size of command buffers for your own use.
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
    
    state.default_2x2 = sg_load_texture_path("/Users/george/git/tyler/assets/default_2x2.png");
    state.default_3x3 = sg_load_texture_path("/Users/george/git/tyler/assets/default_3x3.png");
    InitMap(&state.map, 64, 64);
}

static void input(const sapp_event *e) {
    simgui_handle_event(e);
    sokol_input_handler(e);
}

// Called when the application is shutting down.
static void cleanup(void) {
    DestroyMap(&state.map);
    // Cleanup Sokol GP and Sokol GFX resources.
    sgp_shutdown();
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
