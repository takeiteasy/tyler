//
//  map.h
//  tyler
//
//  Created by George Watson on 06/06/2024.
//

#ifndef map_h
#define map_h
#include "tyler.h"
#include "sokol_gfx.h"
#include "sokol_gp.h"

typedef struct {
    float x, y, zoom;
} Camera;

typedef struct {
    // ...
} Tile;

typedef struct {
    Tile *grid;
    int width, height;
} Map;

void InitMap(Map *map, int w, int h);
void DestroyMap(Map *map);
void RenderMap(Map *map, Camera *camera);

#endif /* map_h */
