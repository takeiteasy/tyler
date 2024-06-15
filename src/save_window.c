//
//  save_window.c
//  tyler
//
//  Created by George Watson on 15/06/2024.
//

#include "save_window.h"
#define JIM_IMPLEMENTATION
#include "jim.h"

static struct {
    char *preivew;
    bool open;
    bool previewOpen;
    bool showMapExportMsg;
    ImVec4 mapExportMsgCol;
    const char *mapExportMsg;
} state = {
    .preivew = NULL,
    .open = false,
    .previewOpen = true,
    .showMapExportMsg = false
};

static size_t JimCallback(const void *ptr, size_t size, size_t nmemb, Jim_Sink sink) {
    const char *test = (const char*)ptr;
    printf("%s\n", test);
    return 1;
}

static int ClearMapFlag(ImGuiInputTextCallbackData* _) {
    state.showMapExportMsg = false;
    return 1;
}

void DrawSaveWindow(void) {
    if (!state.open)
        return;
    
    if (igBegin("Save", &state.open, ImGuiWindowFlags_AlwaysAutoResize)) {
        igText("Map:");
        if (igBeginChild_Str("Map", (ImVec2){0,0}, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
            static char mapPath[MAX_PATH];
            igInputTextWithHint("Map:", "...", mapPath, MAX_PATH * sizeof(char), ImGuiInputTextFlags_None, ClearMapFlag, NULL);
            igSameLine(0, 5);
            if (igButton("Search", (ImVec2){0,0})) {
                // TODO: Alternative formats for map? Non-priority
                osdialog_filters *filters = osdialog_filters_parse("Plain Text:txt");
                char *filename = osdialog_file(OSDIALOG_SAVE, CurrentDirectory(), NULL, filters);
                if (filename) {
                    memset(mapPath, 0, MAX_PATH * sizeof(char));
                    memcpy(mapPath, filename, MIN(strlen(filename), MAX_PATH) * sizeof(char));
                    free(filename);
                }
                osdialog_filters_free(filters);
            }
            bool validPath = false;
            bool pathExists = false;
            static bool overwritePathExists = false;
            if (mapPath[0] != '\0') {
                if (FileExists(mapPath)) {
                    igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){1.f, 1.f, 0.f, 1.f});
                    igText("WARNING! This file already exists!");
                    igPopStyleColor(1);
                    igCheckbox("Overwrite?", &overwritePathExists);
                    validPath = true;
                    pathExists = true;
                } else {
                    const char *basename = RemoveFileName(mapPath);
                    if (basename) {
                        if (!DirExists(basename)) {
                            igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){1.f, 0.f, 0.f, 1.f});
                            igText("ERROR! Invalid directory!");
                            igPopStyleColor(1);
                        } else
                            validPath = strncmp(basename, mapPath, strlen(mapPath)) && !DirExists(mapPath);
                    }
                }
            }
            if (igButton("Export", (ImVec2){0,0}) && validPath) {
                if (pathExists && !overwritePathExists) {
                    state.mapExportMsgCol = (ImVec4){1.f, 0.f, 0.f, 1.f};
                    state.showMapExportMsg = true;
                    state.mapExportMsg = "ERROR! You must check the overwrite checkbox before overwriting any files";
                    goto BAIL;
                }
                FILE *fh = fopen(mapPath, "w");
                if (!fh) {
                    state.mapExportMsgCol = (ImVec4){1.f, 0.f, 0.f, 1.f};
                    state.showMapExportMsg = true;
                    state.mapExportMsg = "ERROR! Failed to create file at destination!";
                    goto BAIL;
                }
                int gridW, gridH;
                MapDims(&gridW, &gridH, NULL, NULL);
                for (int x = 0; x < gridW; x++)
                    for (int y = 0; y < gridH; y++) {
                        char c = CheckMap(x, y) ? '1' : '0';
                        if (!fwrite((void*)&c, sizeof(char), 1, fh)) {
                            state.mapExportMsgCol = (ImVec4){1.f, 0.f, 0.f, 1.f};
                            state.showMapExportMsg = true;
                            state.mapExportMsg = "ERROR! Error while writing to file!";
                            fclose(fh);
                            goto BAIL;
                        }
                    }
                fclose(fh);
                mapPath[0] = '\0';
                state.mapExportMsgCol = (ImVec4){0.f, 1.f, 0.f, 1.f};
                state.showMapExportMsg = true;
                state.mapExportMsg = "SUCCESS! Map has been exported!";
            BAIL:;
            }
            
            igSameLine(0, 5);
            if (igButton("Preview Export", (ImVec2){0,0})) {
                if (state.preivew)
                    free(state.preivew);
                int gridW, gridH;
                MapDims(&gridW, &gridH, NULL, NULL);
                int sz = gridW * gridH;
                state.preivew = malloc((sz + 1) * sizeof(char));
                char *p = state.preivew;
                for (int x = 0; x < gridW; x++)
                    for (int y = 0; y < gridH; y++)
                        *p++ = CheckMap(x, y) ? '1' : '0';
                state.preivew[sz] = '\0';
                state.previewOpen = true;
            }
            
            if (state.showMapExportMsg) {
                igPushStyleColor_Vec4(ImGuiCol_Text, state.mapExportMsgCol);
                igText(state.mapExportMsg);
                igPopStyleColor(1);
            }
            igEndChild();
        }
        
        igSeparator();
        igText("Mask:");
        if (igBeginChild_Str("Mask", (ImVec2){0,0}, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
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
            igInputTextWithHint("Mask:", "...", maskPath, MAX_PATH * sizeof(char), ImGuiInputTextFlags_None, NULL, NULL);
            igSameLine(0, 5);
            if (igButton("Search", (ImVec2){0,0})) {
                osdialog_filters *filters = osdialog_filters_parse(exportType ? "Header:h" : "JSON:json");
                char *filename = osdialog_file(OSDIALOG_SAVE, CurrentDirectory(), NULL, filters);
                if (filename) {
                    memset(maskPath, 0, MAX_PATH * sizeof(char));
                    memcpy(maskPath, filename, MIN(strlen(filename), MAX_PATH) * sizeof(char));
                    free(filename);
                }
                osdialog_filters_free(filters);
            }
            if (igButton("Export", (ImVec2){0,0})) {
                
            }
            igSameLine(0, 5);
            if (igButton("Preview Export", (ImVec2){0,0})) {
                
            }
            igEndChild();
        }
    }
    igEnd();
    
    if (!state.previewOpen || !state.preivew)
        return;
    
    if (igBegin("Preview", &state.previewOpen, ImGuiWindowFlags_AlwaysAutoResize)) {
        igText("Preview");
        igInputTextMultiline("##output", state.preivew, 512, (ImVec2){0,0}, ImGuiInputTextFlags_ReadOnly, NULL, NULL);
    }
    igEnd();
}

void ShowSaveWindow(void) {
    state.open = true;
}
