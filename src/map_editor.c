//
//  map_editor.c
//  tyler
//
//  Created by George Watson on 12/06/2024.
//

#include "map_editor.h"

static struct {
    bool *grid;
    int width, height;
    int tileW, tileH;
    bool drawGrid;
} state;

void InitMap(int width, int height) {
    state.width = width;
    state.height = height;
    state.tileW = 8;
    state.tileH = 8;
    state.drawGrid = true;
    size_t sz = width * height * sizeof(bool);
    state.grid = malloc(sz);
    memset(state.grid, 0, sz);
}

void DestroyMap(void) {
    if (state.grid)
        free(state.grid);
}

int CheckMap(int x, int y) {
    return (int)state.grid[y * state.width + x];
}

void ToggleMap(int x, int y) {
    int i = y * state.width + x;
    state.grid[i] = !state.grid[i];
}

void SetMap(int x, int y, bool v) {
    state.grid[y * state.width + x] = v;
}

void DrawMap(tyState *ty) {
    sgp_set_color(1.f, 1.f, 1.f, 1.f);
    if (state.drawGrid)
        for (int x = 0; x < state.width+1; x++) {
            float xx = x * state.tileW;
            sgp_draw_line(xx, 0, xx, (state.tileH*state.height));
            for (int y = 0; y < state.height+1; y++) {
                float yy = y * state.tileH;
                sgp_draw_line(0, yy, (state.tileW*state.width), yy);
            }
        }
    for (int x = 0; x < state.width; x++)
        for (int y = 0; y < state.height; y++) {
            sgp_rect dst = {x*state.tileW, y*state.tileH, state.tileW, state.tileH};
            tyPoint p;
            switch (tyCalcTile(ty, TY_3X3_MINIMAL, x, y, &p)) {
                case TY_OK:;
                    sgp_set_image(0, MaskEditorTexture());
                    sgp_rect src = {p.x*state.tileW, p.y*state.tileH, state.tileW, state.tileH};
                    sgp_draw_textured_rect(0, dst, src);
                    sgp_reset_image(0);
                    break;
                case TY_NO_MATCH:
                    sgp_draw_filled_rect(dst.x, dst.y, dst.w, dst.h);
                default:
                case TY_DEAD_TILE:
                    break;
            }
        }
    sgp_reset_color();
}
