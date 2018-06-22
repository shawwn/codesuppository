#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#pragma warning(disable:4702)

#include "MeshConsolidate.h"
#include "FloatMath.h"
#include <Nx.h>
#include <NxVec3.h>
#include <NxMat34.h>
#include "NvHashMap.h"

#pragma warning(disable:4100)

#include "UserMemAlloc.h"

#define DEBUG_SHOW 0

#if DEBUG_SHOW
#include "RenderDebug.h"
#endif

#define MESH_CONSOLIDATE_NVSHARE MESH_CONSOLIDATE_##NVSHARE

using namespace NVSHARE;

namespace MESH_CONSOLIDATE_NVSHARE
{

class Edge;
class Polygon;
class MyMeshConsolidate;

static void addPolyPoint(Edge *p,Edge **polyPoints,Polygon *parent);
static void removePolyPoint(Edge *p,Edge **polyPoints);

class TempTri //: public Memalloc TODO TODO TODO
{
public:
  NxU32     mI1;
  NxU32     mI2;
  NxU32     mI3;
  NxU32     mUV1;
  NxU32     mUV2;
  NxU32     mUV3;
  NxU32     mId;
  NxU32     mSubMesh;
  NxVec3    mNormal;
};

class Polygon;

class Edge : public Memalloc
{
public:
  Edge(void)
  {
    mI1 = 0;
    mI2 = 0;
    mUV1 = 0;
    mUV2 = 0;
    mParent = 0;
    mNext = 0;
    mPrev = 0;
    mNextPolygonEdge = 0;
    mNextPolyPoint = 0;
  }

  void init(Polygon *p,NxU32 i1,NxU32 i2,NxU32 uv1,NxU32 uv2)
  {
    mI1 = i1;
    mI2 = i2;
    mUV1 = uv1;
    mUV2 = uv2;
    mParent = p;
    mNext = 0;
    mPrev = 0;
    mNextPolygonEdge = 0;
  }

  NxU32 getHash(void) const { return (mI2<<16)|mI1; };
  NxU32 getInverseHash(void) const { return (mI1<<16)|mI2; };

#if DEBUG_SHOW
  void debugRender(fm_VertexIndex *vertices)
  {
    const NxF32 *v1 = vertices->getVertexFloat(mI1);
    const NxF32 *v2 = vertices->getVertexFloat(mI2);
	gRenderDebug->DebugSphere(v1,0.003f);
    gRenderDebug->DebugRay(v1,v2);
  }
#endif

  bool collinear(const Edge *e,const fm_VertexIndex *fm) const
  {
    const NxF32 *p1 = fm->getVertexFloat(mI1);
    const NxF32 *p2 = fm->getVertexFloat(mI2);
    const NxF32 *p3 = fm->getVertexFloat(e->mI1);
    const NxF32 *p4 = fm->getVertexFloat(e->mI2);
	return NVSHARE::fm_colinear(p1,p2,p3,p4);
  }

  bool sharedUV(const Edge *e) const
  {
    bool ret = true;

    if ( mI1 == e->mI1 )
    {
        if ( mUV1 != e->mUV1 )
        {
            ret = false;
        }
    }
    else if ( mI1 == e->mI2 )
    {
        if ( mUV1 != e->mUV2 )
        {
            ret = false;
        }
    }

    if ( mI2 == e->mI2 )
    {
        if ( mUV2 != e->mUV2 )
        {
            ret = false;
        }
    }
    else if ( mI2 == e->mI1 )
    {
        if ( mUV2 != e->mUV1 )
        {
            ret = false;
        }
    }
    return ret;
  }


  NxU32    mI1;
  NxU32    mI2;
  NxU32    mUV1;
  NxU32    mUV2;

  Polygon *mParent;

  Edge    *mNext;             // linked list pointers inside a specific polygon
  Edge    *mPrev;

  Edge    *mNextPolygonEdge; // the edge hash map list
  Edge    *mNextPolyPoint;   // the polygon-point linked list.
};

class Polygon : public Memalloc
{
public:
  Polygon(void)
  {
    mId = 0;
    mSubMesh = 0;
    mNormal.set(1,0,0);
    mHead = 0;
    mTail = 0;
    mRemoved = false;
	mMerged = false;
    mEcount  = 0;
  }

  void addEdge(Edge *e,Edge **polyPoints)
  {
    addPolyPoint(e,polyPoints,this);
    if ( mHead == 0 )
    {
        mHead = e;
        mTail = e;
        e->mNext = 0;
        e->mPrev = 0;
    }
    else
    {
        mTail->mNext = e;
        e->mNext = 0;
        e->mPrev = mTail;
        mTail = e;
    }
    mEcount++;
  }

  bool isMatch(const Polygon &p) const
  {
    bool ret = false;
    if ( mSubMesh == p.mSubMesh ) // must be part of the same material group.
    {
        NxF32 dot = fm_dot(&mNormal.x,&p.mNormal.x);
        ret = dot >= 0.9999f && dot <= 1.0001f ? true : false;
    }
    return ret;
  }

  void removeEdge(Edge *e,Edge **polyPoints)
  {
    removePolyPoint(e,polyPoints);
    Edge *next     = e->mNext;
    Edge *previous = e->mPrev;
    if ( previous )
    {
      previous->mNext = next;
    }
    else
    {
      mHead = next;
    }

    if ( next )
    {
      next->mPrev = previous;
    }
    else
    {
      mTail = previous;
    }
    mEcount--;
  }

  void insertEdge(Edge *edge_at,Edge *new_edge,Edge **polyPoints)
  {

    addPolyPoint(new_edge,polyPoints,this);

    Edge *his_next = edge_at->mNext;
    edge_at->mNext = new_edge;
    new_edge->mPrev = edge_at;
    new_edge->mNext     = his_next;
    if ( his_next )
    {
      his_next->mPrev = new_edge;
    }
    else
    {
      mTail = new_edge;
    }
    mEcount++;
  }

  Edge * determineInsertionPoint(Polygon *merge,Edge *e,Edge ** polyPoints)
  {
    (void)polyPoints;
    Edge *ret = 0;

    assert( merge->mEcount == 3 );

    // This edge 'e' is getting collapsed...
    Edge *tri[3];
    tri[0] = merge->mHead;
    tri[1]= tri[0]->mNext;
    tri[2]= tri[1]->mNext;

    Edge *scan=0;
    Edge *scan_prev=0;
    Edge *scan_next=0;

    if ( tri[0]->getHash() == e->getInverseHash() )
    {
      scan = tri[0];
      scan_next = tri[1];
      scan_prev = tri[2];
    }
    else if ( tri[1]->getHash() == e->getInverseHash() )
    {
      scan = tri[1];
      scan_next = tri[2];
      scan_prev = tri[0];
    }
    else
    {
      assert( tri[2]->getHash() == e->getInverseHash() );
      scan = tri[2];
      scan_next = tri[0];
      scan_prev = tri[1];
    }

    assert(scan); // we wouldn't be here unless we had a shared edge!
    assert(scan_prev);
    assert(scan_next);

    Edge *e_prev = e->mPrev ? e->mPrev : mTail;
    Edge *e_next = e->mNext ? e->mNext : mHead;

    Edge *insert_one = 0;

    if ( scan_next->getHash() == e_prev->getInverseHash() )
    {
      insert_one = scan_prev;
    }
    else if ( scan_next->getHash() == e_next->getInverseHash() )
    {
      insert_one = scan_prev;
    }
    else if ( scan_prev->getHash() == e_prev->getInverseHash() )
    {
      insert_one = scan_next;
    }
    else if ( scan_prev->getHash() == e_next->getInverseHash() )
    {
      insert_one = scan_next;
    }

    if ( !insert_one )
    {
      ret = scan_next;
    }
    return ret;
  }


  Edge * mergePolygon(Polygon *merge,Edge *e,Edge **polyPoints,MyMeshConsolidate *mmc);
#if DEBUG_SHOW
  void debugRender(fm_VertexIndex *vertices)
  {
    if ( !mRemoved )
    {
        Edge *e = mHead;
		Edge *prev = 0;
        while ( e )
        {
			if ( prev )
			{
				assert( prev->mI2 == e->mI1 );
			}
            e->debugRender(vertices);
			prev = e;
            e = e->mNext;
        }
		assert( prev->mI2 == mHead->mI1 );
    }
  }
#endif

  void consolidate(fm_VertexIndex *fm)
  {
    if ( !mRemoved )
    {

        Edge *e    = mHead;
        Edge *p    = mTail;

        while ( e && e->collinear(p,fm) )
        {
            p = e;
            e = e->mNext;
        }

        Edge *new_head = e; // this is the new head...
        Edge *n = e->mNext ? e->mNext : mHead;
        do
        {
          while ( e->collinear(n,fm) )
          {
			  n = n->mNext ? n->mNext : mHead;
          }
		  e->mI2 = n->mI1;
          e->mNext = n;
          p = e;
          e = n;
        } while ( e != new_head );
        p->mI2 = e->mI1;
        p->mNext = 0;
        mTail = p;
        mHead = new_head;
    }
  }

  NxU32   mId;
  NxU32   mSubMesh;
  NxU32   mEcount;
  NxVec3  mNormal;
  Edge   *mHead;
  Edge   *mTail;
  bool    mRemoved;
  bool    mMerged;
};

static bool hasPolyPoint(Polygon *p,NxU32 i,Edge **polyPoints)
{
  bool ret = false;

  Edge *scan = polyPoints[i];
  while ( scan )
  {
    if ( scan->mParent == p )
    {
      ret = true;
      break;
    }
    scan = scan->mNextPolyPoint;
  }

  return ret;
}

static void addPolyPoint(Edge *p,Edge **polyPoints,Polygon *parent)
{
  NxU32 i = p->mI2;

#ifdef _DEBUG
  Edge *scan = polyPoints[i];
  while ( scan )
  {
    assert( scan != p );
    scan = scan->mNextPolyPoint;
  }
#endif
  p->mParent = parent;
  p->mNextPolyPoint = polyPoints[i];
  polyPoints[i] = p;
}


static void removePolyPoint(Edge *p,Edge **polyPoints)
{
  NxU32 i = p->mI2;

  Edge *scan = polyPoints[i];
  Edge *prev = 0;

  while ( scan && scan != p )
  {
    prev = scan;
    scan = scan->mNextPolyPoint;
  }

//  assert( scan );
//  assert( scan == p );

  if ( scan )
  {
    if ( prev )
    {
      prev->mNextPolyPoint = p->mNextPolyPoint;
    }
    else
    {
      polyPoints[i] = p->mNextPolyPoint;
    }
    p->mNextPolyPoint = 0;
  }

}

typedef NVSHARE::Array< TempTri > TempTriVector;
typedef NVSHARE::HashMap< NxU32, Edge * > EdgeHashMap;
typedef NVSHARE::Array< NxU32 > NxU32Vector;

class MyMeshConsolidate : public MeshConsolidate
{
public:

  MyMeshConsolidate(NxF32 epsilon)
  {
    mWeldEpsilon = epsilon;
    mTexels = fm_createVertexIndex(0.002f,false);
    mVertices = fm_createVertexIndex(mWeldEpsilon,false);
	mVertexOutput = fm_createVertexIndex(mWeldEpsilon,false);
    mPolygons = 0;
    mEdges = 0;
    mPolyCount = 0;
    mEdgeCount = 0;
    mPolyPoints = 0;
  }

  ~MyMeshConsolidate(void)
  {
    release();
  }

  void release(void)
  {
    if ( mTexels )
    {
        fm_releaseVertexIndex(mTexels);
        mTexels = 0;
    }
    if ( mVertices )
    {
        fm_releaseVertexIndex(mVertices);
		mVertices = 0;
    }

	if ( mVertexOutput )
	{
		fm_releaseVertexIndex(mVertexOutput);
		mVertexOutput = 0;
	}

    delete []mPolygons;
    delete []mEdges;
    delete []mPolyPoints;

    mPolygons = 0;
    mEdges = 0;
    mPolyCount = 0;
    mEdgeCount = 0;
    mPolyPoints = 0;
  }

  NxU32 getTexel(const NxF32 *uv) const
  {
    float p[3] = { uv[0], uv[1], 0 };
    bool np;
    return mTexels->getIndex(p,np);
  }

  virtual bool addTriangle(const NxF32 *p1,
                           const NxF32 *p2,
                           const NxF32 *p3,
                           const NxF32 *uv1,
                           const NxF32 *uv2,
                           const NxF32 *uv3,
                           NxU32 id,
                           NxU32 subMesh)
  {
    bool ret = false;

	bool np;
	TempTri t;

	t.mI1 = mVertices->getIndex(p1,np);
	t.mI2 = mVertices->getIndex(p2,np);
	t.mI3 = mVertices->getIndex(p3,np);

    t.mUV1 = getTexel(uv1);
    t.mUV2 = getTexel(uv2);
    t.mUV3 = getTexel(uv3);

	t.mId = id;
	t.mSubMesh = subMesh;

	if ( t.mI1 == t.mI2 || t.mI1 == t.mI3 || t.mI2 == t.mI3 )
	{
        assert(0);
	}
	else
	{
		fm_computePlane(p1,p2,p3,&t.mNormal.x);
		ret = true;
		mInputTriangles.pushBack(t);
	}

    return ret;
  }


  virtual bool meshConsolidate(MeshConsolidateOutput &results)
  {
    bool ret = false;

    mEpsilon = results.mEpsilon;

    if ( !mInputTriangles.empty() )
    {
        mPolyCount = (NxU32)mInputTriangles.size();
        mPolygons  = MEMALLOC_NEW(Polygon)[mPolyCount];
        mEdges     = MEMALLOC_NEW(Edge)[mPolyCount*3];
        TempTri *tri = &mInputTriangles[0];
        Edge *e = mEdges;
        Polygon *p = mPolygons;
        mVcount = (NxU32)mVertices->getVcount();
        mPolyPoints = (Edge **)MEMALLOC_MALLOC(sizeof(Edge *)*mVcount);
		memset(mPolyPoints,0,sizeof(Edge *)*mVcount);
        for (NxU32 i=0; i<mPolyCount; i++)
        {
            p->mId = tri->mId;
            p->mSubMesh = tri->mSubMesh;
            p->mNormal  = tri->mNormal;
            e = initEdge(p,e,tri->mI1,tri->mI2,tri->mUV1,tri->mUV2);
            e = initEdge(p,e,tri->mI2,tri->mI3,tri->mUV2,tri->mUV3);
            e = initEdge(p,e,tri->mI3,tri->mI1,tri->mUV3,tri->mUV1);
            p++;
			tri++;
        }
        mInputTriangles.clear();
    }

    {
        Polygon *p = mPolygons;
        for (NxU32 i=0; i<mPolyCount; i++)
        {
            consolidate(p);
            p++;
        }
#if DEBUG_SHOW
        debugRender();
#endif        
    }


    {
      Polygon *p = mPolygons;
      fm_Triangulate *t = fm_createTriangulate(); // create the 3d polygon triangulator

      for (NxU32 j=0; j<mPolyCount; j++)
      {
        if ( !p->mRemoved )
        {
          #define MAX_VERTS 2048
          assert( p->mEcount < MAX_VERTS );
          if ( p->mEcount < MAX_VERTS )
          {
            NxF32 _vertices[MAX_VERTS*3];
			NxF32 vertices[MAX_VERTS*3];
			Edge *e = p->mHead;
            NxU32 pcount = 0;
            NxF32 *dest = _vertices;
            while ( e )
            {
              const NxF32 *p = mVertices->getVertexFloat(e->mI2);
              dest[0] = p[0];
              dest[1] = p[1];
              dest[2] = p[2];
              dest+=3;
              pcount++;
              e = e->mNext;
			}

            pcount = (NxU32)fm_consolidatePolygon(pcount,_vertices,sizeof(NxF32)*3,vertices,1-mEpsilon);

            switch ( pcount )
            {
              case 0:
              case 1:
              case 2:
				  assert(0);
                break;
              case 3:
                addTriangleOutput(p,vertices,vertices+3,vertices+6);
                break;
              case 4:
                {
                  const NxF32 *p1 = vertices;
                  const NxF32 *p2 = vertices+3;
                  const NxF32 *p3 = vertices+6;
                  const NxF32 *p4 = vertices+9;
                  addTriangleOutput(p,p1,p2,p3);
                  addTriangleOutput(p,p1,p3,p4);
                }
                break;
              default:
                {
                  NxU32 tcount;
                  const NxF32 *triangles = t->triangulate3d(pcount,vertices,sizeof(NxF32)*3,tcount,false,mEpsilon);
                  if ( triangles )
                  {
                    for (NxU32 i=0; i<tcount; i++)
                    {
                      const NxF32 *p1 = triangles;
                      const NxF32 *p2 = triangles+3;
                      const NxF32 *p3 = triangles+6;
                      addTriangleOutput(p,p3,p2,p1);
                      triangles+=9;
                    }
                  }
				  else
				  {
					  printf("debug me");
				      t->triangulate3d(pcount,vertices,sizeof(NxF32)*3,tcount,false,mEpsilon);
				  }
                }
                break;
            }
          }
        }
        p++;
      }
	  fm_releaseTriangulate(t);
    }


    if ( !mOutputIndices.empty() )
    {
      ret = true;
      results.mVcount    = (NxU32)mVertexOutput->getVcount();
      results.mTcount    = (NxU32)mOutputIndices.size()/3;
      results.mIndices   = &mOutputIndices[0];
      results.mIds       = &mOutputIds[0];
      results.mSubMeshes = &mOutputSubMeshes[0];
      results.mVertices  = mVertexOutput->getVerticesFloat();
    }

    return ret;
  }

  Edge * initEdge(Polygon *p,Edge *e,NxU32 i1,NxU32 i2,NxU32 uv1,NxU32 uv2)
  {
    e->init(p,i1,i2,uv1,uv2);
    addEdge(e);
    p->addEdge(e,mPolyPoints);
    e++;
    return e;
  }

  void addEdge(Edge *e)
  {
    NxU32 hash = e->getHash();
    const EdgeHashMap::Entry *found = mEdgeHash.find( hash );
    if ( found == NULL )
    {
        mEdgeHash[hash] = e;
    }
    else
    {
        Edge *next = (*found).second;
        e->mNextPolygonEdge = next;
        mEdgeHash.erase(hash); // erase the old one..
        mEdgeHash[hash] = e;   // assign the new one
    }
  }

  void addTriangleOutput(const Polygon *p,const NxF32 *p1,const NxF32 *p2,const NxF32 *p3)
  {
    bool np;
    NxU32 i1 = (NxU32)mVertexOutput->getIndex(p1,np);
    NxU32 i2 = (NxU32)mVertexOutput->getIndex(p2,np);
    NxU32 i3 = (NxU32)mVertexOutput->getIndex(p3,np);
    mOutputIndices.pushBack(i1);
    mOutputIndices.pushBack(i2);
    mOutputIndices.pushBack(i3);
    mOutputIds.pushBack(p->mId);
    mOutputSubMeshes.pushBack(p->mSubMesh);
  }

  void removeEdge(Edge *e)
  {
    NxU32 hash = e->getHash();
    const EdgeHashMap::Entry *found = mEdgeHash.find(hash);
    if ( found == NULL )
    {
//        assert(0);
    }
    else
    {
      Edge *scan = (*found).second;
      if ( scan == e )
      {
        if ( e->mNext )
        {
          mEdgeHash.erase(hash);
          mEdgeHash[hash] = e->mNextPolygonEdge;
        }
        else
        {
          mEdgeHash.erase(hash);
        }
      }
      else
      {
        Edge *prev = 0;
        while ( scan && scan != e )
        {
          prev = scan;
          scan = scan->mNextPolygonEdge;
        }
        assert(scan);
        assert(prev);
        if ( scan )
        {
          prev->mNextPolygonEdge = e->mNextPolygonEdge;
        }
      }
    }
    e->mNextPolygonEdge = 0;
  }

  void removeEdges(Polygon *p)
  {
    Edge *e = p->mHead;
    while ( e )
    {
        removeEdge(e);
        e = e->mNext;
    }
  }

  Edge * locateSharedEdge(NxU32 hash,Polygon *p)
  {
    Edge *ret = 0;
    const EdgeHashMap::Entry *found = mEdgeHash.find(hash);
    if ( found != NULL )
    {
        Edge *e = (*found).second;
        while ( e )
        {
            if ( p->isMatch(*e->mParent) )
            {
                ret = e;
                break;
            }
            e = e->mNextPolygonEdge;
        }
    }
    return ret;
  }


  //
  void consolidate(Polygon *p)
  {

    if ( !p->mRemoved ) // if it has not already been removed..
    {
       removeEdges(p); // removes the edges..

       Edge *e = p->mHead;

       while ( e ) // find as many triangles we can possibly collapse
       {
         Edge *emerge = locateSharedEdge( e->getInverseHash(), p );

         if ( emerge && !emerge->sharedUV(e) )
         {
            emerge = 0;
         }

		 Polygon *merge = emerge ? emerge->mParent : 0;
         if ( merge )
         {

           Edge *insertion_point = p->determineInsertionPoint(merge,e,mPolyPoints);

           if ( insertion_point )
           {
             NxU32 new_point = insertion_point->mI2;
             if ( hasPolyPoint(p,new_point,mPolyPoints) )
             {
               merge = 0;
             }
           }

           if ( merge )
           {
             removeEdges(merge);
			 removePolyPoints(merge);
             e = p->mergePolygon(merge,e,mPolyPoints,this);
           }
           else
           {
             e = e->mNext; // we skipped this triangle because it would have inserted a point touching the polygon we are building
           }
         }
         else
         {
           e = e->mNext;
         }
       }
    }

    // Collapse colinear points and re-order the polygon
	if ( !p->mRemoved )
	{
		p->consolidate(mVertices);
	}

#if DEBUG_SHOW
	if ( !p->mRemoved )
	{
		static float d = 0.0f;
		d+=1;
		NxMat34 pose;
		pose.t.set(d,0,1);
		gRenderDebug->beginDrawGroup(pose);
		gRenderDebug->setCurrentArrowSize(0.01f);
		p->debugRender(mVertices);
		gRenderDebug->endDrawGroup();
	}
#endif

  }

  void remove(Polygon *p)
  {
    Edge *e = p->mHead;
    while ( e )
    {
      removeEdge(e);
      e = e->mNext;
    }
  }

  void removePolyPoints(Polygon *p)
  {
    assert( p->mEcount == 3 );
    Edge *e1 = p->mHead;
    Edge *e2 = e1->mNext;
    Edge *e3 = e2->mNext;
    removePolyPoint(e1,mPolyPoints);
    removePolyPoint(e2,mPolyPoints);
    removePolyPoint(e3,mPolyPoints);
  }
#if DEBUG_SHOW
  void debugRender(void)
  {
    NxMat34 pose;
    pose.t.set(1,0,0);
    gRenderDebug->beginDrawGroup(pose);
	gRenderDebug->setCurrentArrowSize(0.01f);
    for (NxU32 i=0; i<mPolyCount; i++)
    {
		NxU32 color = gRenderDebug->getDebugColor();
		gRenderDebug->setCurrentColor(color,color);
        mPolygons[i].debugRender(mVertices);
    }
    gRenderDebug->endDrawGroup();
  }
#endif
  NxF32           mEpsilon;
  NxF32           mWeldEpsilon;
  NxU32           mPolyCount;
  NxU32           mEdgeCount;
  NxU32           mVcount; // number of vertices
  Polygon        *mPolygons;
  Edge           *mEdges;
  Edge          **mPolyPoints;
  EdgeHashMap     mEdgeHash;
  NxU32Vector     mOutputIndices;
  NxU32Vector     mOutputIds;
  NxU32Vector     mOutputSubMeshes;

  fm_VertexIndex *mTexels;
  fm_VertexIndex *mVertices;
  fm_VertexIndex *mVertexOutput;
  TempTriVector   mInputTriangles;
};


Edge * Polygon::mergePolygon(Polygon *merge,Edge *e,Edge **polyPoints,MyMeshConsolidate *mmc)
{
  assert( merge->mEcount == 3 );

  merge->mRemoved = true;
  mMerged = true;

  Edge *ret = 0;

  // This edge 'e' is getting collapsed...
  Edge *tri[3];
  tri[0] = merge->mHead;
  tri[1]= tri[0]->mNext;
  tri[2]= tri[1]->mNext;

  Edge *scan=0;
  Edge *scan_prev=0;
  Edge *scan_next=0;

  if ( tri[0]->getHash() == e->getInverseHash() )
  {
    scan = tri[0];
    scan_next = tri[1];
    scan_prev = tri[2];
  }
  else if ( tri[1]->getHash() == e->getInverseHash() )
  {
    scan = tri[1];
    scan_next = tri[2];
    scan_prev = tri[0];
  }
  else
  {
    assert( tri[2]->getHash() == e->getInverseHash() );
    scan = tri[2];
    scan_next = tri[0];
    scan_prev = tri[1];
  }

  assert(scan); // we wouldn't be here unless we had a shared edge!
  assert(scan_prev);
  assert(scan_next);

  Edge *e_prev = e->mPrev ? e->mPrev : mTail;
  Edge *e_next = e->mNext ? e->mNext : mHead;

  Edge *kill1 = 0;
  Edge *kill2 = 0;
  Edge *insert_one = 0;

  if ( scan_next->getHash() == e_prev->getInverseHash() )
  {
    // kill e_prev
    // kill e
    // insert scan_prev
    kill1 = e_prev;
    kill2 = e;
    insert_one = scan_prev;
  }
  else if ( scan_next->getHash() == e_next->getInverseHash() )
  {
    // kill e
    // kill e_next
    // insert scan_prev
    kill1 = e;
    kill2 = e_prev;
    insert_one = scan_prev;
  }
  else if ( scan_prev->getHash() == e_prev->getInverseHash() )
  {
    // kill e_prev
    // kill e
    // insert scan_next
    kill1 = e_prev;
    kill2 = e;
    insert_one = scan_next;
  }
  else if ( scan_prev->getHash() == e_next->getInverseHash() )
  {
    // kill e
    // kill e_next
    // insert scan_next
    kill1 = e;
    kill2 = e_next;
    insert_one = scan_next;
  }

  if ( insert_one )
  {
    Edge *insert_at = kill1->mPrev ? kill1->mPrev : mTail;
    assert( insert_at != kill1 );
    assert( insert_at != kill2 );

    removeEdge(kill1,polyPoints);
    removeEdge(kill2,polyPoints);

    insertEdge(insert_at,insert_one,polyPoints);

    // now, after we have done the insert, we need to make sure this did not cause an additional collapse!
    Edge *collapse_point = insert_one;
    Edge *collapse_next  = collapse_point->mNext ? collapse_point->mNext : mHead;

    while ( collapse_point->mI1 == collapse_next->mI2 && collapse_point->mI2 == collapse_next->mI1 )
    {
      Edge *collapse_previous = collapse_point->mPrev ? collapse_point->mPrev : mTail;

      removeEdge(collapse_point,polyPoints);
      removeEdge(collapse_next,polyPoints);

      collapse_point = collapse_previous;
      collapse_next  = collapse_point->mNext ? collapse_point->mNext : mHead;
    }
    ret = insert_one;
  }
  else
  {

    Edge *previous = e->mPrev;

    Edge *insert = scan_next;
    insert->mPrev = previous;

    assert(insert);

    ret = insert;

    if ( previous )
      previous->mNext = insert;
    else
    {
      assert( e == mHead );
      mHead = insert;
    }

    Edge *patch = insert->mNext ? insert->mNext : merge->mHead;
    assert( patch != scan );
    insert->mNext = patch;

    patch->mPrev = insert;
    patch->mNext     = e->mNext;
    mEcount++; // added two edges but removed one, change is one new edge

    if ( patch->mNext == 0 )
    {
      mTail = patch;
    }
    else
    {
      patch->mNext->mPrev = patch;
    }
    addPolyPoint(ret,polyPoints,this);
  }



  merge->mHead = 0; // we snarfed all of his edges

  return ret;
}

};  // end of namepsace

using namespace MESH_CONSOLIDATE_NVSHARE;

namespace NVSHARE
{

MeshConsolidate * createMeshConsolidate(NxF32 epsilon)
{
  MyMeshConsolidate *mcm = new MyMeshConsolidate(epsilon);
  return static_cast< MeshConsolidate *>(mcm);
}

void              releaseMeshConsolidate(MeshConsolidate *cm)
{
  MyMeshConsolidate *mcm = static_cast< MyMeshConsolidate *>(cm);
  delete mcm;
}

}; // end of namespace

