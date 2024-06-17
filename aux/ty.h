//
//  ty.h
//  tyler
//
//  Created by George Watson on 10/06/2024.
//

#ifndef __ty_h__
#define __ty_h__
#include <stdint.h>

typedef int(*tyCallback)(int, int);

typedef struct {
    int x, y;
} tyPoint;

typedef enum {
    TY_3X3_MINIMAL = 0,
    TY_2X2, // TODO
    TY_3X3
} tyMaskType;

typedef enum {
    TY_OK = 0,
    TY_DEAD_TILE,
    TY_NO_MATCH
} tyResultType;

typedef struct {
    tyCallback callback;
    int gridWidth, gridHeight;
    tyPoint map[256];
} tyState;

typedef struct {
    int grid[9];
    int x, y;
} tyNeighbours;

void tyInit(tyState *state, tyCallback callback, int gridW, int gridH);
void tyReadNeighbours(tyState *state, int x, int y, tyNeighbours *dst);
uint8_t tyBitmask(tyNeighbours *mask, int simplified);
uint8_t tyBitmaskAt(tyState *state, tyMaskType type, int gridX, int gridY);
tyResultType tyCalcTile(tyState *state, tyMaskType type, int gridX, int gridY, tyPoint *out);

#endif /* __ty_h__ */

#ifdef TY_IMPL
#include <assert.h>

void tyInit(tyState *state, tyCallback callback, int gridW, int gridH) {
    assert((state->callback = callback));
    assert((state->gridWidth = gridW) > 0);
    assert((state->gridHeight = gridH) > 0);
    for (int i = 0; i < 256; i++)
        state->map[i] = (tyPoint){-1,-1};
}

#include <stdio.h>

void tyReadNeighbours(tyState *state, int x, int y, tyNeighbours *dst) {
    for (int yy = 0; yy < 3; yy++)
        for (int xx = 0; xx < 3; xx++) {
            int dx = x + (xx-1);
            int dy = y + (yy-1);
            dst->grid[yy*3+xx] = dx < 0 || dy < 0 || dx >= state->gridWidth || dy >= state->gridHeight ? 0 : state->callback(dx, dy);
        }
    dst->x = x;
    dst->y = y;
}

// TODO: This covers 3x3 + 3x3 simplified, 2x2 needs implementing
uint8_t tyBitmask(tyNeighbours *mask, int simplified) {
    if (simplified) {
#define CHECK_CORNER(N, A, B) \
mask->grid[(N)] = !mask->grid[(A)] || !mask->grid[(B)] ? 0 : mask->grid[(N)];
        CHECK_CORNER(0, 1, 3);
        CHECK_CORNER(2, 1, 5);
        CHECK_CORNER(6, 7, 3);
        CHECK_CORNER(8, 7, 5);
#undef CHECK_CORNER
    }
    uint8_t result = 0;
    for (int y = 0, n = 0; y < 3; y++)
        for (int x = 0; x < 3; x++)
            if (!(y == 1 && x == 1))
                result += (mask->grid[y * 3 + x] << n++);
    return result;
}

uint8_t tyBitmaskAt(tyState *state, tyMaskType type, int gridX, int gridY) {
    tyNeighbours neighbours;
    tyReadNeighbours(state, gridX, gridY, &neighbours);
    return tyBitmask(&neighbours, 1);
}

tyResultType tyCalcTile(tyState *state, tyMaskType type, int gridX, int gridY, tyPoint *out) {
    if (!state->callback(gridX, gridY))
        return TY_DEAD_TILE; // Target tile is dead
    tyNeighbours neighbours;
    tyReadNeighbours(state, gridX, gridY, &neighbours);
    uint8_t mask = tyBitmask(&neighbours, 1);
    tyPoint *p = &state->map[mask];
    if (p->x == -1 || p->y == -1)
        return TY_NO_MATCH;
    if (out)
        *out = *p;
    return TY_OK;
}
#endif // TY_IMPL
