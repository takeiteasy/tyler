//
//  new_window.c
//  tyler
//
//  Created by George Watson on 14/06/2024.
//

#include "new_window.h"

static struct {
    bool open;
} state = {
    .open = false
};

void DrawNewWindow(void) {
    if (!state.open)
        return;
    
    if (igBegin("New", &state.open, ImGuiWindowFlags_AlwaysAutoResize)) {
        igText("Map Size:");
        static int gridW = 32, gridH = 32;
        static bool lockGrid = true;
        igDragInt("Grid Width", &gridW, 1, 8, 1024, "%d", ImGuiSliderFlags_None);
        igDragInt("Grid Height", lockGrid ? &gridW : &gridH, 1, 8, 1024, "%d", ImGuiSliderFlags_None);
        gridW = CLAMP(gridW, 8, 1024);
        gridH = CLAMP(gridH, 8, 1024);
        igCheckbox("Lock Grid", &lockGrid);
        if (lockGrid)
            gridH = gridW;
        
        igSeparator();
        igText("Atlas:");
        static char path[MAX_PATH];
        igInputTextWithHint(" ", "texture path ...", path, MAX_PATH * sizeof(char), ImGuiInputTextFlags_None, NULL, NULL);
        igSameLine(0, 0);
        if (igButton("Search", (ImVec2){0,0})) {
            // TODO: Open file dialog, fill path buffer with result
        }
        bool validPath = false;
        if (path[0] != '\0') {
            if (!FileExists(path)) {
                igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){1.f, 0.f, 0.f, 1.f});
                igText("ERROR! This file doesn't exist!");
                igPopStyleColor(1);
            } else {
                static const char *validExtensions[11] = {
                    "jpg", "jpeg", "png", "bmp", "psd", "tga", "hdr", "pic", "ppm", "pgm", "qoi"
                };
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
        static int tileW = 8, tileH = 8;
        static bool lockTile = true;
        igDragInt("Tile Width", &tileW, 1, 8, 64, "%d", ImGuiSliderFlags_None);
        igDragInt("Tile Height", lockTile ? &tileW : &tileH, 1, 8, 64, "%d", ImGuiSliderFlags_None);
        tileW = CLAMP(tileW, 8, 64);
        tileH = CLAMP(tileH, 8, 64);
        igCheckbox("Lock Tile", &lockTile);
        if (lockTile)
            tileH = tileW;
    }
    igEnd();
    
}

void ShowNewWindow(void) {
    state.open = true;
}
