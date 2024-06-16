//
//  save_window.c
//  tyler
//
//  Created by George Watson on 15/06/2024.
//

#include "save_window.h"
#define JIM_IMPLEMENTATION
#include "jim.h"
#define EZCLIPBOARD_IMPLEMENTATION
#include "ez/ezclipboard.h"

#undef MIN
#define MIN(A,B) ((A) < (B) ? (A) : (B))

static struct {
    char *preivew;
    bool open;
    bool previewOpen;
    bool showMapExportMsg;
    ImVec4 mapExportMsgCol;
    const char *mapExportMsg;
    bool showMaskExportMsg;
    ImVec4 maskExportMsgCol;
    const char *maskExportMsg;
    size_t previewSize;
    size_t preivewCapacity;
} state = {
    .preivew = NULL,
    .open = false,
    .previewOpen = true,
    .showMapExportMsg = false
};

static size_t JimCallback(const void *ptr, size_t size, size_t nmemb, Jim_Sink sink) {
    size_t inc = (size * nmemb) * sizeof(char);
    size_t newSize = state.previewSize + inc;
    if (newSize > state.preivewCapacity) {
        state.preivewCapacity *= 2;
        state.preivew = realloc(state.preivew, state.preivewCapacity);
    }
    char *offset = state.preivew + state.previewSize;
    memcpy(offset, ptr, inc);
    char *tail = offset + inc + 1;
    tail[0] = '\0';
    state.previewSize = newSize;
    return inc;
}

static int ClearMapFlag(ImGuiInputTextCallbackData* _) {
    state.showMapExportMsg = false;
    return 1;
}

static void OpenMapPath(char *dst) {
    // TODO: Alternative formats for map? Non-priority
    osdialog_filters *filters = osdialog_filters_parse("Plain Text:txt");
    char *filename = osdialog_file(OSDIALOG_SAVE, CurrentDirectory(), NULL, filters);
    if (filename) {
        memset(dst, 0, MAX_PATH * sizeof(char));
        memcpy(dst, filename, MIN(strlen(filename), MAX_PATH) * sizeof(char));
        free(filename);
    }
    osdialog_filters_free(filters);
}

static void MaskToJSON(Jim *jim, tyState *ty) {
    jim_object_begin(jim);
    jim_member_key(jim, "masks");
    jim_object_begin(jim);
    for (int i = 0; i < 256; i++) {
        tyPoint *p = &ty->map[i];
        if (p->x < 0 || p->y < 0)
            continue;
        char buf[4];
        snprintf(buf, 4, "%d", i);
        jim_member_key(jim, buf);
        jim_object_begin(jim);
        jim_member_key(jim, "x");
        jim_integer(jim, p->x);
        jim_member_key(jim, "y");
        jim_integer(jim, p->y);
        jim_object_end(jim);
    }
    jim_object_end(jim);
    jim_object_end(jim);
}

// TODO: Change from bool to enum if adding more export types
static void OpenMaskPath(char *dst, bool cheader) {
    osdialog_filters *filters = osdialog_filters_parse(cheader ? "Header:h" : "JSON:json");
    char *filename = osdialog_file(OSDIALOG_SAVE, CurrentDirectory(), NULL, filters);
    if (filename) {
        memset(dst, 0, MAX_PATH * sizeof(char));
        memcpy(dst, filename, MIN(strlen(filename), MAX_PATH) * sizeof(char));
        free(filename);
    }
    osdialog_filters_free(filters);
}

void DrawSaveWindow(tyState *ty) {
    if (!state.open)
        return;
    
    if (igBegin("Save", &state.open, ImGuiWindowFlags_AlwaysAutoResize)) {
        igText("Map:");
        if (igBeginChild_Str("Map", (ImVec2){0,0}, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
            static char mapPath[MAX_PATH];
            igInputTextWithHint("Map:", "...", mapPath, MAX_PATH * sizeof(char), ImGuiInputTextFlags_None, ClearMapFlag, NULL);
            igSameLine(0, 5);
            if (igButton("Search", (ImVec2){0,0}))
                OpenMapPath(mapPath);
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
            if (igButton("Export", (ImVec2){0,0})) {
                if (mapPath[0] == '\0') {
                    OpenMapPath(mapPath);
                    goto BAIL;
                }
                if (!validPath)
                    goto BAIL;
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
            bool validPath = false;
            bool pathExists = false;
            static bool overwritePathExists = false;
            if (maskPath[0] != '\0') {
                if (FileExists(maskPath)) {
                    igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){1.f, 1.f, 0.f, 1.f});
                    igText("WARNING! This file already exists!");
                    igPopStyleColor(1);
                    igCheckbox("Overwrite?", &overwritePathExists);
                    validPath = true;
                    pathExists = true;
                } else {
                    const char *basename = RemoveFileName(maskPath);
                    if (basename) {
                        if (!DirExists(basename)) {
                            igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){1.f, 0.f, 0.f, 1.f});
                            igText("ERROR! Invalid directory!");
                            igPopStyleColor(1);
                        } else
                            validPath = strncmp(basename, maskPath, strlen(maskPath)) && !DirExists(maskPath);
                    }
                }
            }
            if (igButton("Export", (ImVec2){0,0})) {
                if (maskPath[0] == '\0') {
                    OpenMaskPath(maskPath, exportType);
                    goto FLEE;
                }
                if (!validPath)
                    goto FLEE;
                if (pathExists && !overwritePathExists) {
                    state.mapExportMsgCol = (ImVec4){1.f, 0.f, 0.f, 1.f};
                    state.showMapExportMsg = true;
                    state.mapExportMsg = "ERROR! You must check the overwrite checkbox before overwriting any files";
                    goto FLEE;
                }
                FILE *fh = fopen(maskPath, "w");
                if (!fh) {
                    state.mapExportMsgCol = (ImVec4){1.f, 0.f, 0.f, 1.f};
                    state.showMapExportMsg = true;
                    state.mapExportMsg = "ERROR! Failed to create file at destination!";
                    goto FLEE;
                }
                
                // TODO: Change this to a switch w/ enum if adding different export types
                if (!exportType) { // JSON
                    Jim jim = {
                        .sink = fh,
                        .write = (Jim_Write)fwrite
                    };
                    MaskToJSON(&jim, ty);
                } else {
                    // TODO: C Header export
                }
                
                fclose(fh);
                maskPath[0] = '\0';
                state.maskExportMsgCol = (ImVec4){0.f, 1.f, 0.f, 1.f};
                state.showMaskExportMsg = true;
                state.maskExportMsg = "SUCCESS! Mask has been exported!";
            FLEE:;
            }
            
            igSameLine(0, 5);
            if (igButton("Preview Export", (ImVec2){0,0})) {
                if (state.preivew)
                    free(state.preivew);
                Jim jim = {.write = JimCallback};
                state.previewSize = 0;
                state.preivewCapacity = 512;
                state.preivew = malloc(state.preivewCapacity * sizeof(char));
                MaskToJSON(&jim, ty);
                state.previewOpen = true;
            }
            
            if (state.showMaskExportMsg) {
                igPushStyleColor_Vec4(ImGuiCol_Text, state.maskExportMsgCol);
                igText(state.maskExportMsg);
                igPopStyleColor(1);
            }
            igEndChild();
        }
    }
    igEnd();
    
    if (!state.previewOpen || !state.preivew)
        return;
    
    if (igBegin("Preview", &state.previewOpen, ImGuiWindowFlags_AlwaysAutoResize)) {
        igText("Preview");
        igPushTextWrapPos(500);
        igTextWrapped("%s", state.preivew);
        igPopTextWrapPos();
        if (igButton("Copy to Clipboard", (ImVec2){0,0}))
            ezSetClipboard(state.preivew);
    }
    igEnd();
}

void ShowSaveWindow(void) {
    state.open = true;
}
