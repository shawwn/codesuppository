#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "TestStanHull.h"

#include "RenderDebug.h"
#include "wavefront.h"
#include "stanhull.h"
using namespace NVSHARE;
#include "shared/MeshSystem/MeshSystemHelper.h"

void testStanHull(MeshSystemHelper *ms)
{
  SEND_TEXT_MESSAGE(0,"Demonstrating StanHull code snippet, originally published on March 4, 2006\r\n");

  gRenderDebug->reset();

  if ( ms )
  {
    MeshSystemRaw *mr = ms->getMeshSystemRaw();
    if ( mr && mr->mVcount )
    {
      SEND_TEXT_MESSAGE(0,"Generating convex hull for input mesh with %d vertices.\r\n", mr->mVcount );
      // now generate the convex hull.
      NVSHARE::HullDesc desc((NVSHARE::HullFlag)(NVSHARE::QF_TRIANGLES | NVSHARE::QF_SKIN_WIDTH),mr->mVcount,mr->mVertices,sizeof(NxF32)*3);
      desc.mMaxVertices = 64;
      desc.mSkinWidth = 0.1f;

      NVSHARE::HullLibrary h;
      NVSHARE::HullResult result;
      NVSHARE::HullError e = h.CreateConvexHull(desc,result);
      if ( e == NVSHARE::QE_OK )
      {
        SEND_TEXT_MESSAGE(0,"Output Hull contains %d triangle faces.\r\n", result.mNumFaces );
        SEND_TEXT_MESSAGE(0,"Fly the camera inside the hull to see the mesh details.\r\n");
		gRenderDebug->pushRenderState();
		gRenderDebug->setCurrentColor(0xFFFF00,0xFFFFFF);
		gRenderDebug->addToCurrentState(DebugRenderState::SolidWireShaded);
		gRenderDebug->setCurrentDisplayTime(600.0f);
        for (NxU32 i=0; i<result.mNumFaces; i++)
        {
          NxU32 i1 = result.mIndices[i*3+0];
          NxU32 i2 = result.mIndices[i*3+1];
          NxU32 i3 = result.mIndices[i*3+2];
          const NxF32 *p1 = &result.mOutputVertices[i1*3];
          const NxF32 *p2 = &result.mOutputVertices[i2*3];
          const NxF32 *p3 = &result.mOutputVertices[i3*3];
          gRenderDebug->DebugTri(p1,p2,p3);
        }
		gRenderDebug->popRenderState();
        h.ReleaseResult(result);
      }
      else
      {
        SEND_TEXT_MESSAGE(0,"Failed to create ConvexHull\r\n");
      }
    }
    else
    {
      SEND_TEXT_MESSAGE(0,"Empty input mesh.\r\n");
    }
    ms->releaseMeshSystemRaw(mr);
  }
  else
  {
    SEND_TEXT_MESSAGE(0,"No input mesh.\r\n");
  }

}

