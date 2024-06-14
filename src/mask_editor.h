//
//  mask_editor.h
//  tyler
//
//  Created by George Watson on 12/06/2024.
//

#ifndef mask_editor_h
#define mask_editor_h

#include "tyler.h"
#include <stdlib.h>
#include <math.h>

void InitMaskEditor(void);
void DestroyMaskEditor(void);
void DrawMaskEditor(tyState *ty);
void ToggleMaskEditor(void);
sg_image MaskEditorTexture(void);
bool* MaskEditorIsOpen(void);
void ChangeTexture(sg_image texture, int textureW, int textureH, int tileW, int tileH);

#endif /* mask_editor_h */
