//
//  mask_editor.h
//  tyler
//
//  Created by George Watson on 12/06/2024.
//

#ifndef mask_editor_h
#define mask_editor_h

#include "tyler.h"

void InitMaskEditor(void);
void DestroyMaskEditor(void);
void DrawMaskEditor(tyState *ty);
void ToggleMaskEditor(void);
sg_image MaskEditorTexture(void);

#endif /* mask_editor_h */
