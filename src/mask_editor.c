//
//  mask_editor.c
//  tyler
//
//  Created by George Watson on 12/06/2024.
//

#include "mask_editor.h"
#include <stdlib.h>

typedef struct {
    sg_image texture;
    int width, height;
} Texture;

static struct {
    tyNeighbours *grid;
    float scale;
    bool open;
    int spriteW, spriteX;
    int spriteH, spriteY;
    Texture default_2x2;
    Texture default_3x3;
    Texture *currentAtlas;
    sg_image cross;
    tyNeighbours *currentBitmask;
} state;

void InitMaskEditor(void) {
    state.default_2x2.texture = sg_load_texture_path_ex("/Users/george/git/tyler/assets/default_2x2.png", &state.default_2x2.width, &state.default_2x2.height);
    state.default_3x3.texture = sg_load_texture_path_ex("/Users/george/git/tyler/assets/default_3x3.png", &state.default_3x3.width, &state.default_3x3.height);
    state.cross = sg_load_texture_path("/Users/george/git/tyler/assets/x.png");
    state.currentAtlas = &state.default_3x3;
    
    state.open = false;
    state.scale = 4.f;
    
    state.spriteW = state.spriteH = 8;
    state.spriteX = state.default_3x3.width / state.spriteW;
    state.spriteY = state.default_3x3.height / state.spriteH;
    state.grid = malloc(state.spriteX * state.spriteY * sizeof(tyNeighbours));
    for (int x = 0; x < state.spriteX; x++)
        for (int y = 0; y < state.spriteY; y++) {
            tyNeighbours *mask = &state.grid[y * state.spriteX + x];
            memset(mask->grid, 0, sizeof(int) * 9);
            mask->x = x;
            mask->y = y;
        }
    state.currentBitmask = NULL;
}

void DestroyMaskEditor(void) {
    sokol_helper_destroy(state.default_2x2.texture);
    sokol_helper_destroy(state.default_3x3.texture);
    if (state.grid)
        free(state.grid);
}

int IsPointInRect(sgp_rect r, int x, int y) {
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

void DrawMaskEditorBox(float x, float y, float w, float h, sg_color color) {
    sgp_set_color(color.r, color.g, color.b, color.a);
    sgp_draw_line(x, y, x + w, y);
    sgp_draw_line(x, y, x, y + h);
    sgp_draw_line(x + w, y, x + w, y + h);
    sgp_draw_line(x, y + h, x + w, y + h);
    sgp_reset_color();
}

static void igDrawMaskEditorCb(const ImDrawList* dl, const ImDrawCmd* cmd) {
    int sw = state.currentAtlas->width*state.scale;
    int sh = state.currentAtlas->height*state.scale;
    int cx = (int)cmd->ClipRect.x;
    int cy = (int)cmd->ClipRect.y;
    int cw = (int)(cmd->ClipRect.z - cmd->ClipRect.x);
    int ch = (int)(cmd->ClipRect.w - cmd->ClipRect.y);
    sgp_scissor(cx, cy, cw, ch);
    cx = cx ? cx : cw - sw;
    cy = cy ? cy : ch - sh;
    sgp_viewport(cx, cy, sw, sh);
    sgp_set_image(0, state.currentAtlas->texture);
    sgp_push_transform();
    sgp_scale(state.scale, state.scale);
    sgp_draw_filled_rect(0, 0, state.currentAtlas->width, state.currentAtlas->height);
    sgp_pop_transform();
    sgp_reset_image(0);
    
    if (state.currentBitmask != NULL) {
        int gw = state.spriteW*state.scale;
        int gh = state.spriteH*state.scale;
        float ox = MAX(state.currentBitmask->x * gw, 1);
        float oy = MAX(state.currentBitmask->y * gh, 1);
        int dx = ox + gw >= cw ? gw-2 : gw;
        int dy = oy + gh >= ch ? gh-2 : gh;
        DrawMaskEditorBox(ox, oy, dx, dy, (sg_color){1.f, 1.f, 1.f, 1.f});
    }
    
    sgp_set_image(0, state.cross);
    for (int x = 0; x < state.spriteX; x++)
        for (int y = 0; y < state.spriteY; y++) {
            float dx = ((x * state.spriteW)*state.scale);
            float dy = ((y * state.spriteH)*state.scale);
            int gw = state.spriteW*state.scale;
            int gh = state.spriteH*state.scale;
            float ex = gw/3, ey = gh/3;
            for (int yy = 0; yy < 3; yy++)
                for (int xx = 0; xx < 3; xx++)
                    if (state.grid[y * state.spriteX + x].grid[yy * 3 + xx]) {
                        sgp_rect dst = {dx+(xx*ex)+2, dy+(yy*ey)+2, state.spriteW, state.spriteH};
                        sgp_rect src = {0, 0, 8, 8};
                        sgp_draw_textured_rect(0, dst, src);
                    }
        }
    sgp_reset_image(0);
    sgp_flush();
}

void DrawMaskEditor(tyState *ty) {
    if (!state.open)
        return;
    
    igSetNextWindowPos((ImVec2){20,20}, ImGuiCond_Once, (ImVec2){0,0});
    ImVec2 maskEditSize = (ImVec2) {
        .x = state.currentAtlas->width*state.scale,
        .y = state.currentAtlas->height*state.scale
    };
    int gw = state.spriteW*state.scale;
    int gh = state.spriteH*state.scale;
    
    igSetNextWindowSize(maskEditSize, ImGuiCond_Once);
    if (igBegin("Mask Editor", &state.open, ImGuiWindowFlags_AlwaysAutoResize)) {
        igText("Mask View:");
        if (igBeginChild_Str("Mask Editor", maskEditSize, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
            ImDrawList* dl = igGetWindowDrawList();
            ImDrawList_AddCallback(dl, igDrawMaskEditorCb, NULL);
            for (int x = 0; x < state.spriteX; x++)
                for (int y = 0; y < state.spriteY; y++) {
                    igSetCursorPos((ImVec2){x*gw, y*gh});
                    char buf[32];
                    snprintf(buf, 32, "grid%dx%d", x, y);
                    if (igInvisibleButton(buf, (ImVec2){gw, gh}, ImGuiButtonFlags_MouseButtonLeft)) {
                        if (state.currentBitmask)
                            if (x == state.currentBitmask->x &&
                                y == state.currentBitmask->y) {
                                state.currentBitmask = NULL;
                                continue;
                            }
                        state.currentBitmask = &state.grid[y * state.spriteX + x];
                    }
                }
            igEndChild();
        }
        
        
        for (int i = 0; i < 256; i++)
            ty->map[i] = (tyPoint){-1,-1};
        for (int x = 0; x < state.spriteX; x++)
            for (int y = 0; y < state.spriteY; y++) {
                tyNeighbours *tmask = &state.grid[y * state.spriteX + x];
                uint8_t mask = tyBitmask(tmask, 0);
                if (!mask && !tmask->grid[4])
                    continue;
                tyPoint *dst = &ty->map[mask];
                if (dst->x == -1 || dst->y == -1) {
                    dst->x = x;
                    dst->y = y;
                } else {
                    igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){1.f, 0.f, 0.f, 1.f});
                    igText("ERROR: Tile (%d,%d) has same mask as (%d,%d)", x, y, dst->x, dst->y);
                    igPopStyleColor(1);
                }
            }
        
        if (state.currentBitmask) {
            maskEditSize.y += 10;
            igText("Mask Editor:");
            if (igBeginChild_Str("Button Grid", maskEditSize, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
                uint8_t bitmask = tyBitmask(state.currentBitmask, 0);
                char buf[9];
                for (int i = 0; i < 8; i++)
                    buf[i] = !!((bitmask << i) & 0x80) ? 'F' : '0';
                buf[8] = '\0';
                igText("tile: %d, %d - mask: %d,0x%x,0b%s", state.currentBitmask->x, state.currentBitmask->y, bitmask, bitmask, buf);
                if (igBeginTable("button_grid", 3, ImGuiTableFlags_NoBordersInBody, maskEditSize, gw)) {
                    for (int y = 0; y < 3; y++) {
                        igTableNextRow(ImGuiTableRowFlags_None, gh);
                        for (int x = 0; x < 3; x++) {
                            igTableSetColumnIndex(x);
                            static const char *labels[9] = {
                                "TL", "T", "TR", "L", "X", "R", "BL", "B", "BR"
                            };
                            bool b = state.currentBitmask->grid[y * 3 + x];
                            igPushStyleColor_Vec4(ImGuiCol_Button, b ? (ImVec4){0.f, 1.f, 0.f, 1.f} : (ImVec4){1.f, 0.f, 0.f, 1.f});
                            if (igButton(labels[y * 3 + x], (ImVec2){gw, gh}))
                                state.currentBitmask->grid[y * 3 + x] = !b;
                            igPopStyleColor(1);
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

void ToggleMaskEditor(void) {
    state.open = !state.open;
}

sg_image MaskEditorTexture(void) {
    return state.currentAtlas ? state.currentAtlas->texture : (sg_image){SG_INVALID_ID};
}

bool* MaskEditorIsOpen(void) {
    return &state.open;
}
