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
    struct {
        sg_image color;
        sg_image depth;
        sg_attachments attachements;
        sg_sampler sampler;
    } framebuffer;
    bool open;
} MaskEditor;

static struct {
    sg_image default_2x2;
    sg_image default_3x3;
    Map map;
} state;

static void draw_triangles(void) {
    const float PI = 3.14159265f;
    static sgp_vec2 points_buffer[4096];

    sgp_irect viewport = sgp_query_state()->viewport;
    int width = viewport.w, height = viewport.h;
    float hw = width * 0.5f;
    float hh = height * 0.5f;
    float w = height*0.3f;
    unsigned int count = 0;
    float step = (2.0f*PI)/6.0f;
    for (float theta = 0.0f; theta <= 2.0f*PI + step*0.5f; theta+=step) {
        sgp_vec2 v = {hw*1.33f + w*cosf(theta), hh*1.33f - w*sinf(theta)};
        points_buffer[count++] = v;
        if (count % 3 == 1) {
            sgp_vec2 u = {hw, hh};
            points_buffer[count++] = u;
        }
    }
    sgp_set_color(1.0f, 0.0f, 1.0f, 1.0f);
    sgp_draw_filled_triangles_strip(points_buffer, count);
}

static void draw_fbo(void) {
    sgp_begin(128, 128);
    sgp_project(0, 128, 128, 0);
    draw_triangles();

    sg_pass_action pass_action = {0};
    pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    pass_action.colors[0].clear_value.r = 1.0f;
    pass_action.colors[0].clear_value.g = 1.0f;
    pass_action.colors[0].clear_value.b = 1.0f;
    pass_action.colors[0].clear_value.a = 0.2f;
    sg_pass pass = {
        .action = pass_action,
        .attachments = MaskEditor.framebuffer.attachements
    };
    sg_begin_pass(&pass);
    sgp_flush();
    sgp_end();
    sg_end_pass();
}

static void testcb(const ImDrawList* dl, const ImDrawCmd* cmd) {
    const int cx = (int) cmd->ClipRect.x;
    const int cy = (int) cmd->ClipRect.y;
    const int cw = (int) (cmd->ClipRect.z - cmd->ClipRect.x);
    const int ch = (int) (cmd->ClipRect.w - cmd->ClipRect.y);
    sgp_scissor(cx, cy, cw, ch);
    sgp_viewport(cx, cy, 360, 360);
    sgp_set_image(0, MaskEditor.framebuffer.color);
    sgp_set_sampler(0, MaskEditor.framebuffer.sampler);
    sgp_draw_filled_rect(0, 0, 128, 128);
    sgp_reset_image(0);
    sgp_reset_sampler(0);
    sgp_flush();
    sgp_end();
}

// Called on every frame of the application.
static void frame(void) {
    int width = sapp_width(), height = sapp_height();
    sgp_begin(width, height);
    
    // draw background
    sgp_set_color(0.05f, 0.05f, 0.05f, 1.0f);
    sgp_clear();
    sgp_reset_color();
    
    
    sgp_set_blend_mode(SGP_BLENDMODE_BLEND);
    draw_fbo();

    simgui_frame_desc_t igFrameDesc = {
        .width = width,
        .height = height,
        .delta_time = sapp_frame_duration(),
        .dpi_scale = sapp_dpi_scale()
    };
    simgui_new_frame(&igFrameDesc);
    
    igSetNextWindowPos((ImVec2){20, 20}, ImGuiCond_Once, (ImVec2){0,0});
    igSetNextWindowSize((ImVec2){800, 400}, ImGuiCond_Once);
    if (igBegin("Dear ImGui", 0, 0)) {
        if (igBeginChild_Str("sokol-gfx", (ImVec2){360, 360}, true, ImGuiWindowFlags_None)) {
            ImDrawList* dl = igGetWindowDrawList();
            ImDrawList_AddCallback(dl, testcb, NULL);
        }
        igEndChild();
    }
    igEnd();
    

    sgp_viewport(0, 0, width, height);
    float ratio = width/(float)height;
       // Set drawing coordinate space to (left=-ratio, right=ratio, top=1, bottom=-1).
       sgp_project(-ratio, ratio, 1.0f, -1.0f);
    // Draw an animated rectangle that rotates and changes its colors.
    float time = sapp_frame_count() * sapp_frame_duration();
    float r = sinf(time)*0.5+0.5, g = cosf(time)*0.5+0.5;
    sgp_push_transform();
    sgp_set_color(r, g, 0.3f, 1.0f);
    sgp_rotate_at(time, 0.0f, 0.0f);
    sgp_draw_filled_rect(-0.5f, -0.5f, 1.0f, 1.0f);
    sgp_pop_transform();
    sgp_reset_color();

    // dispatch draw commands
    sg_pass pass = {.swapchain = sglue_swapchain()};
    sg_begin_pass(&pass);
    sgp_flush();
    simgui_render();
    sgp_end();
    sg_end_pass();
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
    
    // create frame buffer image
    sg_image_desc fb_image_desc = {0};
    fb_image_desc.render_target = true;
    fb_image_desc.width = 128;
    fb_image_desc.height = 128;
    MaskEditor.framebuffer.color = sg_make_image(&fb_image_desc);
    if (sg_query_image_state(MaskEditor.framebuffer.color) != SG_RESOURCESTATE_VALID) {
        fprintf(stderr, "Failed to create frame buffer image\n");
        exit(-1);
    }
    
    // create frame buffer depth stencil
    sg_image_desc fb_depth_image_desc = {0};
    fb_depth_image_desc.render_target = true;
    fb_depth_image_desc.width = 128;
    fb_depth_image_desc.height = 128;
    fb_depth_image_desc.pixel_format = sapp_depth_format();
    MaskEditor.framebuffer.depth = sg_make_image(&fb_depth_image_desc);
    if (sg_query_image_state(MaskEditor.framebuffer.depth) != SG_RESOURCESTATE_VALID) {
        fprintf(stderr, "Failed to create frame buffer depth image\n");
        exit(-1);
    }
    
    // create frame buffer attachments
    sg_attachments_desc fb_attachments_desc = {
        .colors = {{.image=MaskEditor.framebuffer.color}},
        .depth_stencil = {.image=MaskEditor.framebuffer.depth}
    };
    MaskEditor.framebuffer.attachements = sg_make_attachments(&fb_attachments_desc);
    if (sg_query_attachments_state(MaskEditor.framebuffer.attachements) != SG_RESOURCESTATE_VALID) {
        fprintf(stderr, "Failed to create frame buffer attachments\n");
        exit(-1);
    }
    
    // create linear sampler
    sg_sampler_desc linear_sampler_desc = {
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
    };
    MaskEditor.framebuffer.sampler = sg_make_sampler(&linear_sampler_desc);
    if (sg_query_sampler_state(MaskEditor.framebuffer.sampler) != SG_RESOURCESTATE_VALID) {
        fprintf(stderr, "failed to create linear sampler");
        exit(-1);
    }
    
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
