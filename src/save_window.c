//
//  save_window.c
//  tyler
//
//  Created by George Watson on 15/06/2024.
//

#include "save_window.h"

static struct {
    bool open;
} state = {
    .open = false
};

void DrawSaveWindow(void) {
    if (!state.open)
        return;
    
    if (igBegin("Save", &state.open, ImGuiWindowFlags_AlwaysAutoResize)) {
        igText("Map:");
        static char mapPath[MAX_PATH];
        igInputTextWithHint(" ", "export path ...", mapPath, MAX_PATH * sizeof(char), ImGuiInputTextFlags_None, NULL, NULL);
        if (igButton("Export Map", (ImVec2){0,0})) {
            
        }
        igSameLine(0, 5);
        if (igButton("Preview Map Export", (ImVec2){0,0})) {
            
        }
        igSeparator();
        igText("Mask:");
        static const char *exts[] = {"JSON", "C Header"};
        static int exportType = 0;
        if (igBeginCombo("Export As", exts[exportType], ImGuiComboFlags_None)) {
            for (int i = 0; i < sizeof(exts) / sizeof(const char*); i++) {
                bool selected = i == exportType;
                if (igSelectable_Bool(exts[i], selected, ImGuiSelectableFlags_None, (ImVec2){0,0}))
                    exportType = i;
                if (selected)
                    igSetItemDefaultFocus();
            }
            igEndCombo();
        }
        static char maskPath[MAX_PATH];
        igInputTextWithHint("  ", "export path ...", maskPath, MAX_PATH * sizeof(char), ImGuiInputTextFlags_None, NULL, NULL);
        if (igButton("Export Mask", (ImVec2){0,0})) {
            
        }
        igSameLine(0, 5);
        if (igButton("Preview Mask Export", (ImVec2){0,0})) {
            
        }
    }
    igEnd();
}

void ShowSaveWindow(void) {
    state.open = true;
}
