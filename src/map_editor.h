//
//  map_editor.h
//  tyler
//
//  Created by George Watson on 12/06/2024.
//

#ifndef map_editor_h
#define map_editor_h

#include "tyler.h"
#include "mask_editor.h"
#include <stdbool.h>
#include <stdlib.h>

void InitMap(int gridW, int gridH, int tileW, int tileH);
void DestroyMap(void);
int CheckMap(int x, int y);
void ToggleMap(int x, int y);
void ToggleGrid(void);
bool* MapDrawGrid(void);
void SetMap(int x, int y, bool v);
void SetMapTileSize(int w, int h);
void DrawMap(tyState *ty, int mouseX, int mouseY);
bool* MapEditorIsOpen(void);
void ResetMap(int gridW, int gridH, int tileW, int tileH);
void ToggleMapEditor(void);

#endif /* map_editor_h */
