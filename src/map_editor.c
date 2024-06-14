//
//  map_editor.c
//  tyler
//
//  Created by George Watson on 12/06/2024.
//

#include "map_editor.h"

static struct {
    bool *grid;
    int gridW, gridH;
    int tmpGridW, tmpGridH;
    int tileW, tileH;
    bool drawGrid;
    bool open;
    struct {
        bool grid[9];
    } anchor;
} state;

void InitMap(int width, int height) {
    state.tmpGridW = state.gridW = width;
    state.tmpGridH = state.gridH = height;
    state.tileW = 8;
    state.tileH = 8;
    state.drawGrid = true;
    state.open = false;
    size_t sz = width * height * sizeof(bool);
    state.grid = malloc(sz);
    memset(state.grid, 0, sz);
    memset(state.anchor.grid, 0, 9 * sizeof(bool));
    state.anchor.grid[0] = true;
}

void DestroyMap(void) {
    if (state.grid)
        free(state.grid);
}

int CheckMap(int x, int y) {
    return (int)state.grid[y * state.gridW + x];
}

void ToggleMap(int x, int y) {
    int i = y * state.gridW + x;
    state.grid[i] = !state.grid[i];
}

void ToggleGrid(void) {
    state.drawGrid = !state.drawGrid;
}

bool* MapDrawGrid(void) {
    return &state.drawGrid;
}

void SetMap(int x, int y, bool v) {
    state.grid[y * state.gridW + x] = v;
}

extern void DrawMaskEditorBox(float x, float y, float w, float h, sg_color color);

void DrawMap(tyState *ty, int mouseX, int mouseY) {
    sgp_set_color(1.f, 1.f, 1.f, 1.f);
    if (state.drawGrid)
        for (int x = 0; x < state.gridW+1; x++) {
            float xx = x * state.tileW;
            sgp_draw_line(xx, 0, xx, (state.tileH*state.gridH));
            for (int y = 0; y < state.gridH+1; y++) {
                float yy = y * state.tileH;
                sgp_draw_line(0, yy, (state.tileW*state.gridW), yy);
            }
        }
    for (int x = 0; x < state.gridW; x++)
        for (int y = 0; y < state.gridH; y++) {
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
    if (mouseX >= 0 && mouseY >= 0 && mouseX < state.gridW && mouseY < state.gridH)
        DrawMaskEditorBox(mouseX*state.tileW, mouseY*state.tileH, state.tileW, state.tileH, (sg_color){1.f, 0.f, 0.f, 1.f});
    sgp_reset_color();
    
    if (!state.open)
        return;
    
    if (igBegin("Map Editor", &state.open, ImGuiWindowFlags_AlwaysAutoResize)) {
        igText("Anchor:");
        if (igBeginTable("button_grid2", 3, ImGuiTableFlags_NoBordersInBody, (ImVec2){165,150}, 50)) {
            int newSelection = -1;
            for (int y = 0; y < 3; y++) {
                igTableNextRow(ImGuiTableRowFlags_None, 50);
                for (int x = 0; x < 3; x++) {
                    igTableSetColumnIndex(x);
                    int i = y * 3 + x;
                    bool b = state.anchor.grid[i];
                    igPushStyleColor_Vec4(ImGuiCol_Button, b ? (ImVec4){0.f, 1.f, 0.f, 1.f} : (ImVec4){1.f, 0.f, 0.f, 1.f});
                    static const char *labels[9] = {
                        "TL", "T", "TR", "L", "X", "R", "BL", "B", "BR"
                    };
                    if (igButton(labels[i], (ImVec2){50, 50}))
                        newSelection = i;
                    igPopStyleColor(1);
                }
            }
            if (newSelection > -1) {
                memset(state.anchor.grid, 0, 9 * sizeof(bool));
                state.anchor.grid[newSelection] = true;
            }
            igEndTable();
        }
        igSeparator();
        igText("Map Size:");
        igDragInt("Width", &state.tmpGridW, 1, 8, 512, "%d", ImGuiSliderFlags_None);
        igDragInt("Height", &state.tmpGridH, 1, 8, 512, "%d", ImGuiSliderFlags_None);
        state.tmpGridW = CLAMP(state.tmpGridW, 8, 512);
        state.tmpGridH = CLAMP(state.tmpGridH, 8, 512);
        if (igButton("Apply", (ImVec2){0,0}) && (state.tmpGridW != state.gridW || state.tmpGridH != state.gridH)) {
            // TODO: Resize map
        }
        igSeparator();
        igAlignTextToFramePadding();
        igText("Clear map:");
        igSameLine(0, 50);
        if (igButton("CLEAR", (ImVec2){0,0})) // TODO: Add yes/no dialog
            memset(state.grid, 0, state.gridW * state.gridH * sizeof(bool));
    }
    igEnd();
}

bool* MapEditorIsOpen(void) {
    return &state.open;
}

void ToggleMapEditor(void) {
    state.open = !state.open;
}
