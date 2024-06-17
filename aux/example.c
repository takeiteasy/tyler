#define SOKOL_HELPER_IMPL
#include "sokol_helpers/sokol_input.h"
#include "sokol_helpers/sokol_img.h"
#include "sokol_helpers/sokol_generic.h"
#include "sokol_helpers/sokol_scalefactor.h"
#define SOKOL_IMPL
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_glue.h"
#include "sokol_log.h"
#include "sokol_time.h"
#include "sokol_args.h"
#include "sokol_gp.h"
#define TY_MASK_IMPLEMENTATION
#include "ty_3x3_simplified.h"
#define TY_IMPL
#include "ty.h"

static struct {
    struct {
        float x, y;
        float zoom;
    } camera;
    struct {
        int fillChance;
        int smoothIterations;
        int survive, starve;
        int *grid, width, height;
        sg_image texture;
    } map;
    int mouseX, mouseY;
    int worldX, worldY;
    tyState ty;
} state = {
    .camera = {
        .x = 0,
        .y = 0,
        .zoom = 32.f
    },
    .map = {
        .fillChance = 40,
        .smoothIterations = 5,
        .survive = 4,
        .starve = 3,
        .grid = NULL,
        .width = 32,
        .height = 32
    }
};

static void die(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    abort();
}

// This is the callback interface for our map. Since the our map is very simple
// and any tile can only be 'on' or 'off' we just return the element in the array
static int CheckMap(int x, int y) {
    return state.map.grid[y * state.map.width + x];
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
    stm_setup();
    
    srand((unsigned int)time(NULL));
    state.map.grid = malloc(state.map.width * state.map.height * sizeof(int));
    for (int x = 0; x < state.map.width; x++)
        for (int y = 0; y < state.map.height; y++)
            state.map.grid[y * state.map.width + x] = (rand() % 100) + 1 < state.map.fillChance;
    
    for (int i = 0; i < state.map.smoothIterations; i++)
        for (int x = 0; x < state.map.width; x++)
            for (int y = 0; y < state.map.height; y++) {
                int neighbours = 0;
                
                for (int nx = x - 1; nx <= x + 1; nx++)
                    for (int ny = y - 1; ny <= y + 1; ny++) {
                        if (nx >= 0 && nx < state.map.width && ny >= 0 && ny < state.map.height) {
                            if ((nx != x || ny != y) && state.map.grid[ny * state.map.width + nx])
                                neighbours++;
                        } else
                            neighbours++;
                    }
                
                if (neighbours > state.map.survive)
                    state.map.grid[y * state.map.width + x] = 1;
                else if (neighbours < state.map.starve)
                    state.map.grid[y * state.map.width + x] = 0;
            }
    
    state.map.texture = sg_load_texture_path("assets/default_3x3.png");
    state.camera.zoom = 32.f;
    
    // Initialize the ty state. This requires a callback, and a map width + height.
    // The callback recieves a coordinate and must return if the tile on your map
    // is a wall or floor.
    tyInit(&state.ty, CheckMap, state.map.width, state.map.height);
    // Load in the tile mask exported from tyler into the ty state
    tyLoadDefaultMask3x3Simplified(&state.ty);
}

#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define CLAMP(V, MI, MA) MIN(MAX((V), (MI)), (MA))

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
    
    if (sapp_check_scrolled())
        state.camera.zoom = CLAMP(state.camera.zoom - sapp_scroll_y(), 16.f, 512.f);
    
    if (sapp_is_button_down(SAPP_MOUSEBUTTON_LEFT)) {
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
    state.worldX = (int)((state.worldX + state.camera.x) / 8);
    state.worldY = (int)((state.worldY + state.camera.y) / 8);
    
    int mapW = state.map.width * 8, mapH = state.map.height * 8;
    sgp_draw_line(0, 0, mapW, 0);
    sgp_draw_line(mapW, 0, mapW, mapH);
    sgp_draw_line(0, 0, 0, mapH);
    sgp_draw_line(0, mapH, mapW, mapH);
    
    // Map rendering
    for (int x = 0; x < state.map.width; x++)
        for (int y = 0; y < state.map.height; y++)
            if (state.map.grid[y * state.map.width + x]) {
                sgp_rect dst = {x*8, y*8, 8, 8};
                // offset will hold the atlas coordinates for the tile
                tyPoint offset;
                // Ask ty to calculate the current tile and return the coordinates
                // of the tile we're rendering from the Atlas
                tyResultType result = tyCalcTile(&state.ty, TY_3X3_MINIMAL, x, y, &offset);
                switch (result) {
                    case TY_OK:; // Tile mask found
                        sgp_set_image(0, state.map.texture);
                        // Atlas coordinates must be multiplied manually by the
                        // user. Our tiles are 8x8 in the Atlas so must be adjusted
                        // accordingly.
                        sgp_rect src = {offset.x*8,offset.y*8, 8, 8};
                        sgp_draw_textured_rect(0, dst, src);
                        sgp_reset_image(0);
                        break;
                    case TY_DEAD_TILE: // Tile doesn't need drawing
                        break;
                    case TY_NO_MATCH: // Tile doesn't match any mask (error)
                        // If the mask is not complete, some tile configurations
                        // may have been missed. Or the ty state has been modified.
                        // Or the mask hasn't been loaded in the the ty state properly.
                        // Or memory corruption... any way you cut it there is a problem!
                        sgp_draw_filled_rect(dst.x, dst.y, dst.w, dst.h);
                        break;
                }
            }

    sg_pass pass = {.swapchain = sglue_swapchain()};
    sg_begin_pass(&pass);
    sgp_flush();
    sgp_end();
    sg_end_pass();
    sg_commit();
    sokol_input_update();
}

static void cleanup(void) {
    sg_destroy(state.map.texture);
    free(state.map.grid);
    sgp_shutdown();
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_desc desc = { .argc=argc, .argv=argv };
    sargs_setup(&desc);
#define ARGS                            \
    X("width", width)                   \
    X("height", height)                 \
    X("fill-chance", fillChance)        \
    X("iterations", smoothIterations)   \
    X("survive", survive)               \
    X("starve", starve)
#define X(ARG, VAR)                         \
    if (sargs_exists(ARG)) {                \
        const char *arg = sargs_value(ARG); \
        if (!(state.map.VAR = atoi(arg)))   \
            die("Invalid " ARG);            \
    }
    ARGS
#undef X
    sargs_shutdown();
    return (sapp_desc) {
        .init_cb = init,
        .frame_cb = frame,
        .event_cb = sokol_input_handler,
        .cleanup_cb = cleanup,
        .window_title = "ty"
    };
}
