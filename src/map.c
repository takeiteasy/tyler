//
//  map.c
//  tyler
//
//  Created by George Watson on 06/06/2024.
//

#include "map.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

void InitMap(Map *map, int w, int h) {
    assert(w > 0 && h > 0);
    assert(map);
    map->width = w;
    map->height = h;
    assert((map->grid = malloc(w * h * sizeof(Tile))));
}

void DestroyMap(Map *map) {
    assert(map);
    if (map->grid)
        free(map->grid);
    memset(map, 0, sizeof(Map));
}

void RenderMap(Map *map, Camera *camera) {
    
}
