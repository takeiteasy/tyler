//
//  new_window.c
//  tyler
//
//  Created by George Watson on 14/06/2024.
//

#include "new_window.h"

static struct {
    int gridW, gridH;
    int tileW, tileH;
    sg_image preview;
    int atlasW, atlasH;
    int atlasGridW, atlasGridH;
    ImVec4 gridColor;
    bool skipLoad;
    bool open;
    float previewScale;
} state = {
    .gridW = 32,
    .gridH = 32,
    .tileW = 8,
    .tileH = 8,
    .preview = (sg_image){SG_INVALID_ID},
    .skipLoad = false,
    .open = false,
    .previewScale = 1.f,
    .gridColor = (ImVec4){1.f, 1.f, 1.f, 1.f}
};

static const char *validExtensions[11] = {
    "jpg", "jpeg", "png", "bmp", "psd", "tga", "hdr", "pic", "ppm", "pgm", "qoi"
};

extern int IsPointInRect(sgp_rect r, int x, int y);

static void igDrawPreviewCb(const ImDrawList* dl, const ImDrawCmd* cmd) {
    int cx = (int)cmd->ClipRect.x;
    int cy = (int)cmd->ClipRect.y;
    int cw = (int)(cmd->ClipRect.z - cmd->ClipRect.x);
    int ch = (int)(cmd->ClipRect.w - cmd->ClipRect.y);
    float iw = state.atlasW * state.previewScale;
    float ih = state.atlasH * state.previewScale;
    
    sgp_scissor(cx, cy, iw, ih);
    cx = cx ? cx : cw - iw;
    cy = cy ? cy : ch - ih;
    sgp_viewport(cx, cy, iw, ih);
    sgp_set_image(0, state.preview);
    sgp_draw_filled_rect(0, 0, iw, ih);
    sgp_reset_image(0);
    
    sgp_set_color(state.gridColor.x, state.gridColor.y, state.gridColor.z, state.gridColor.w);
    for (int x = 0; x < state.atlasGridW+1; x++) {
        float xx = x * (state.tileW * state.previewScale);
        float bottom = (state.atlasGridH * state.tileH) * state.previewScale;
        sgp_draw_line(xx, 0, xx, bottom);
        for (int y = 0; y < state.atlasGridH+1; y++) {
            float yy = x * (state.tileH * state.previewScale);
            float right = (state.atlasGridW * state.tileW) * state.previewScale;
            sgp_draw_line(0, yy, right, yy);
        }
    }
    
    sgp_reset_color();
    sgp_reset_viewport();
}

int ClearSkipFlag(ImGuiInputTextCallbackData* _) {
    printf("test\n");
    state.skipLoad = false;
    return 1;
}

void DrawNewWindow(void) {
    if (!state.open)
        return;
    
    if (igBegin("New", &state.open, ImGuiWindowFlags_AlwaysAutoResize)) {
        igText("Map Size:");
        static bool lockGrid = true;
        igDragInt("Grid Width", &state.gridW, 1, 8, 1024, "%d", ImGuiSliderFlags_None);
        igDragInt("Grid Height", lockGrid ? &state.gridW : &state.gridH, 1, 8, 1024, "%d", ImGuiSliderFlags_None);
        state.gridW = CLAMP(state.gridW, 8, 1024);
        state.gridH = CLAMP(state.gridH, 8, 1024);
        igCheckbox("Lock Grid", &lockGrid);
        if (lockGrid)
            state.gridH = state.gridW;
        
        igSeparator();
        igText("Atlas:");
        static char path[MAX_PATH];
        igInputTextWithHint(" ", "texture path ...", path, MAX_PATH * sizeof(char), ImGuiInputTextFlags_CallbackEdit, ClearSkipFlag, NULL);
        igSameLine(0, 0);
        if (igButton("Search", (ImVec2){0,0})) {
            osdialog_filters *filter = osdialog_filters_parse("Images:jpg,jpeg,png,bmp,psd,tga,hdr,pic,ppm,pgm,qoi");
            char *filename = osdialog_file(OSDIALOG_OPEN, CurrentDirectory(), NULL, filter);
            if (filename) {
                memset(path, 0, MAX_PATH * sizeof(char));
                memcpy(path, filename, MIN(strlen(filename), MAX_PATH) * sizeof(char));
                free(filename);
                sokol_helper_destroy(state.preview);
            }
            osdialog_filters_free(filter);
        }
        bool validPath = false;
        if (path[0] != '\0') {
            if (!FileExists(path)) {
                igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){1.f, 0.f, 0.f, 1.f});
                igText("ERROR! This file doesn't exist!");
                igPopStyleColor(1);
            } else {
                const char *ext = FileExt(path);
                bool validExt = false;
                if (!ext || ext[0] == '\0')
                    goto BAIL;
                for (char *p = (char*)ext; *p; p++)
                    if (*p >= 65 && *p <= 90)
                        *p += 32;
                size_t extLength = strlen(ext);
                for (int i = 0; i < 11; i++)
                    if (!strncmp(ext, validExtensions[i], extLength)) {
                        validExt = true;
                        break;
                    }
            BAIL:
                if (!validExt) {
                    igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){1.f, 0.f, 0.f, 1.f});
                    igText("ERROR! Unknown extension!");
                    igPopStyleColor(1);
                    igSameLine(0, 5);
                    igTextDisabled("(?)");
                    if (igBeginItemTooltip()) {
                        igPushTextWrapPos(igGetFontSize() * 35.0f);
                        igText("Supported image formats: jpg, jpeg, png, bmp, psd, tga, hdr, pic, ppm, pgm, qoi (provided by stb_image.h + qoi.h)");
                        igPopTextWrapPos();
                        igEndTooltip();
                    }
                } else
                    validPath = true;
            }
        }
        
        static const char* autotileTypes[] = {
            "3x3 Simplified (48)",
            "2x2 (16)",
            "3x3 (256)"
        };
        static int autotileSelected = TY_3X3_MINIMAL;
        igSliderInt("Mask Type", &autotileSelected, 0, 2, autotileSelected >= 0 && autotileSelected <= 2 ? autotileTypes[autotileSelected] : "???", ImGuiSliderFlags_None);
        static bool lockTile = true;
        igDragInt("Tile Width", &state.tileW, 1, 8, 64, "%d", ImGuiSliderFlags_None);
        igDragInt("Tile Height", lockTile ? &state.tileW : &state.tileH, 1, 8, 64, "%d", ImGuiSliderFlags_None);
        state.tileW = CLAMP(state.tileW, 8, 64);
        state.tileH = CLAMP(state.tileH, 8, 64);
        igCheckbox("Lock Tile", &lockTile);
        if (lockTile)
            state.tileH = state.tileW;
        
        if (validPath) {
            if (sg_query_state(state.preview) != SG_RESOURCESTATE_VALID && !state.skipLoad) {
                state.preview = sg_load_texture_path_ex(path, &state.atlasW, &state.atlasH);
                state.skipLoad = sg_query_state(state.preview) != SG_RESOURCESTATE_VALID;
            }
        } else
            sokol_helper_destroy(state.preview);
        
        if (state.skipLoad) {
            igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){1.f, 0.f, 0.f, 1.f});
            igText("ERROR! Failed to load image from path!");
            igPopStyleColor(1);
        }
        
        if (sg_query_state(state.preview) == SG_RESOURCESTATE_VALID) {
            ImVec2 size;
            igGetWindowSize(&size);
            float aspectRatio = size.x / (float)size.y;
            size.y = size.x * aspectRatio;
            size.x = 0;
            
            float fgridW = state.atlasW / (float)state.tileW;
            float fgridH = state.atlasH / (float)state.tileH;
            if (fmod(fgridW, 1.f) != 0) {
                igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){1.f, 1.f, 0.f, 1.f});
                igText("WARNING! Atlas width is not divisible by %d!", state.tileW);
                igPopStyleColor(1);
            }
            if (fmod(fgridH, 1.f) != 0) {
                igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){1.f, 1.f, 0.f, 1.f});
                igText("WARNING! Atlas height is not divisible by %d!", state.tileH);
                igPopStyleColor(1);
            }
            state.atlasGridW = (int)floorf(fgridW);
            state.atlasGridH = (int)floorf(fgridH);

            igSeparator();
            igText("Preview:");
            igSameLine(0, 5);
            if (igButton("+", (ImVec2){0,0}))
                state.previewScale += .25f;
            igSameLine(0, 5);
            if (igButton("-", (ImVec2){0,0}))
                state.previewScale = MAX(state.previewScale - .25f, .25f);
            igSameLine(0, 5);
            igColorEdit3("Grid Color", (float*)&state.gridColor, ImGuiColorEditFlags_NoAlpha);
            
            if (igBeginChild_Str("Preview", size, true, ImGuiWindowFlags_None)) {
                ImDrawList* dl = igGetWindowDrawList();
                ImDrawList_AddCallback(dl, igDrawPreviewCb, NULL);
            }
            igEndChild();
        }
    }
    igEnd();
    
}

void ShowNewWindow(void) {
    state.open = true;
}
