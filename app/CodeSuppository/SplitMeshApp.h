#ifndef SPLIT_MESH_APP_H

#define SPLIT_MESH_APP_H

#include "UserMemAlloc.h"

enum SplitMeshCommand
{
  SMC_WIREFRAME,
  SMC_SOLID,
  SMC_PLANE_YUP,
  SMC_PLANE_XUP,
  SMC_PLANE_ZUP,
  SMC_PLANE_D,
  SMC_EXPLODE,
  SMC_SHOW_LEFT,
  SMC_SHOW_RIGHT,
  SMC_SHOW_RINGS,
  SMC_SHOW_CLOSURE,
  SMC_SHOW_TRIANGLES,
  SMC_SHOW_SPLIT_PLANE,
  SMC_ROTATE_X,
  SMC_ROTATE_Y,
  SMC_ROTATE_Z,
  SMC_TT_CONVEX,
  SMC_TT_EAR_SPLITTING,
  SMC_TT_GAME_SWF,
  SMC_COLLAPSE_COLINEAR,
  SMC_TEST_INTERSECTION,
  SMC_EDGE_INTERSECT,
  SMC_SAVE_LEFT_EDGES,
  SMC_TEST_OBB,
  SMC_SAVE_RESULTS,
  SMC_TEST_EDGE_WALK,
  SMC_SAVE_OBJ,
  SMC_CONSOLIDATE_MESH,
  SMC_CONVEX_DECOMPOSITION,
  SMC_DEBUG_VIEW,
  SMC_NOISE,
  SMC_TESSELATE,
  SMC_REMOVE_TJUNCTIONS,
  SMC_FILL_BASIN,
  SMC_FILL_BASIN_PER,
  SMC_ERODE_ITERATIONS,
  SMC_ERODE_RATE,
  SMC_ERODE_POWER,
  SMC_SMOOTH_RATE,
  SMC_ERODE_THRESHOLD,
  SMC_ERODE_SEDIMENTATION,
};

class MeshSystemHelper;

void appSetMeshSystemHelper(MeshSystemHelper *msh);
void appRender(void);
void appCommand(SplitMeshCommand command,bool state=true,const NxF32 *data=0);
void appImportTer(const char *fname);


#endif
