#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "TestMeshConsolidation.h"
#include "MeshImport.h"
using namespace NVSHARE;
#include "ConsolidateMesh.h"
#include "MeshConsolidate.h"
#include "shared/MeshSystem/MeshSystemHelper.h"
#include "RenderDebug.h"
#include "RemoveTjunctions.h"

#define MESH_CONSOLIDATE 1

void testMeshConsolidation(MeshSystemHelper * ms)
{
  SEND_TEXT_MESSAGE(0,"Demonstrates how to consolidate a mesh\r\n");

   if ( ms )
   {
 	    gRenderDebug->reset();
        MeshSystemRaw *mr = ms->getMeshSystemRaw();
        if ( mr && mr->mVcount )
        {
          RemoveTjunctionsDesc desc;
          RemoveTjunctions *rt = createRemoveTjunctions();
          desc.mVcount = mr->mVcount;
          desc.mVertices = mr->mVertices;
          desc.mTcount    = mr->mTcount;
          desc.mIndices   = mr->mIndices;
          desc.mIds       = 0;
          size_t tcount = rt->removeTjunctions(desc);
		  SEND_TEXT_MESSAGE(0,"Input triangle count %d output triangle count %d.\r\n", mr->mTcount, tcount );
#if MESH_CONSOLIDATE
 		  MeshConsolidate *cm = createMeshConsolidate(0.0001f);
#else
		  ConsolidateMesh *cm = createConsolidateMesh();
#endif
  		  for (NxU32 i=0; i<tcount; i++)
  		  {
  			NxU32 i1 = desc.mIndicesOut[i*3+0];
  			NxU32 i2 = desc.mIndicesOut[i*3+1];
  			NxU32 i3 = desc.mIndicesOut[i*3+2];
			if ( i1 != i2 && i1 != i3 && i2 != i3 )
			{
  				const NxF32 *p1 = &desc.mVertices[i1*3];
  				const NxF32 *p2 = &desc.mVertices[i2*3];
  				const NxF32 *p3 = &desc.mVertices[i3*3];
#if MESH_CONSOLIDATE
  				cm->addTriangle(p1,p2,p3,0,0,0,i,0);
#else
  				cm->addTriangle(p1,p2,p3);
#endif
			}
			else
			{
				SEND_TEXT_MESSAGE(0,"Skipped degenerate triangle.\r\n");
			}
  		  }
#if MESH_CONSOLIDATE
  		  MeshConsolidateOutput results;
  		  cm->meshConsolidate(results);
		  gRenderDebug->pushRenderState();
		  gRenderDebug->addToCurrentState(DebugRenderState::DoubleSidedWire);
		  gRenderDebug->setCurrentColor(0x0000FF,0xFF00FF);
		  gRenderDebug->setCurrentDisplayTime(6000);

  		  for (NxU32 i=0; i<results.mTcount; i++)
  		  {
  			NxU32 i1 = results.mIndices[i*3+0];
  			NxU32 i2 = results.mIndices[i*3+1];
  			NxU32 i3 = results.mIndices[i*3+2];
  			const NxF32 *p1 = &results.mVertices[i1*3];
  			const NxF32 *p2 = &results.mVertices[i2*3];
  			const NxF32 *p3 = &results.mVertices[i3*3];
			gRenderDebug->DebugTri(p1,p2,p3);
  		  }
		  gRenderDebug->popRenderState();
  		  SEND_TEXT_MESSAGE(0,"Input TriCount: %d Output TriCount: %d\r\n", tcount, results.mTcount );
  		  releaseMeshConsolidate(cm);
          releaseRemoveTjunctions(rt);
#else
  		  ConsolidateMeshOutput results;
  		  cm->consolidateMesh(results);
  		  for (NxU32 i=0; i<results.mTcount; i++)
  		  {
  			NxU32 i1 = results.mIndices[i*3+0];
  			NxU32 i2 = results.mIndices[i*3+1];
  			NxU32 i3 = results.mIndices[i*3+2];
  			const NxF32 *p1 = &results.mVerticesFloat[i1*3];
  			const NxF32 *p2 = &results.mVerticesFloat[i2*3];
  			const NxF32 *p3 = &results.mVerticesFloat[i3*3];
  			gRenderDebug->DebugTri(p1,p2,p3,0xFFFF00,6000);
  		  }
  		  SEND_TEXT_MESSAGE(0,"Input TriCount: %d Output TriCount: %d\r\n", tcount, results.mTcount );
  		  releaseConsolidateMesh(cm);
          releaseRemoveTjunctions(rt);
#endif
        }
   }
}

