#ifndef MESH_SYSTEM_HELPER

#define MESH_SYSTEM_HELPER

#include "UserMemAlloc.h"

namespace physx
{
class MeshSystem;
class MeshSystemContainer;
};
namespace NVSHARE
{
class Pd3dMaterial;
};

class MeshSystemRaw : public NVSHARE::Memalloc
{
public:
  MeshSystemRaw(void)
  {
    mVcount = 0;
    mTcount = 0;
    mVertices = 0;
    mIndices = 0;
  }
  NxU32 mVcount;
  NxU32 mTcount;
  NxF32 *mVertices;
  NxU32 *mIndices;
};

class MeshSystemHelper
{
public:
  virtual void debugRender(bool showMesh,
                           bool showSkeleton,
                           bool showWireframe,
                           bool showAnimation,
                           bool showCollision,
                           bool flipWinding) = 0;

  virtual physx::MeshSystem * getMeshSystem(void) const = 0;
  virtual physx::MeshSystemContainer * getMeshSystemContainer(void) = 0;
  virtual bool importMesh(const char *fname) = 0;
  virtual void advanceAnimation(NxF32 dtime,NxF32 rate) = 0;
  virtual bool exportEZM(void) = 0;
  virtual bool exportOgre(void) = 0;
  virtual bool exportObj(void) = 0;
  virtual const NxF32 * getCompositeTransforms(NxU32 &bone_count) = 0;

  virtual MeshSystemRaw * getMeshSystemRaw(void) = 0;
  virtual void            releaseMeshSystemRaw(MeshSystemRaw *mr) = 0;
  virtual void setSelectCollision(NxI32 sc) = 0;
  virtual NxU32 getMaterialCount() = 0;
  virtual NVSHARE::Pd3dMaterial* getMaterial(NxU32 idx) = 0;
};

MeshSystemHelper * createMeshSystemHelper(void);
void               releaseMeshSystemHelper(MeshSystemHelper *m);

#endif
