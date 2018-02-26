#ifndef INC_RENDER_OBJECTS_H_
#define INC_RENDER_OBJECTS_H_

bool createRenderObjects();
void destroyRenderObjects();

#include "render/cte_buffer.h"
#include "ctes.h"
extern CRenderCte<CCteCamera> cb_camera;
extern CRenderCte<CCteObject> cb_object;

void activateCamera(const CCamera& cam);
void setWorldTransform(MAT44 new_matrix, VEC4 color = VEC4(1, 1, 1, 1));


#endif

