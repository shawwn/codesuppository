#ifndef AUTO_GEOMETRY_H

#define AUTO_GEOMETRY_H

#include "UserMemAlloc.h"

namespace NVSHARE
{

	class SimpleHull
	{
	public:
		SimpleHull(void)
		{
			mBoneIndex = 0;
			mVertexCount = 0;
			mVertices  = 0;
			mTriCount = 0;
			mIndices = 0;
			mMeshVolume = 0;
			mParentIndex = -1;
			mBoneName = 0;
		}
		int           mBoneIndex;
		int           mParentIndex;
		const char   *mBoneName;
		float         mTransform[16];
		unsigned int  mVertexCount;
		float        *mVertices;
		unsigned int  mTriCount;
		unsigned int *mIndices;
		float         mMeshVolume;
	};

	enum BoneOption
	{
		BO_INCLUDE,
		BO_EXCLUDE,
		BO_COLLAPSE
	};

	class SimpleBone
	{
	public:
		SimpleBone(void)
		{
			mOption = BO_INCLUDE;
			mParentIndex = -1;
			mBoneName = 0;
			mBoneMinimalWeight = 0.4f;
		}
		BoneOption   mOption;
		const char  *mBoneName;
		int          mParentIndex;
		float        mBoneMinimalWeight;
		float        mTransform[16];
		float        mInverseTransform[16];
	};

	class SimpleSkinnedVertex
	{
	public:
		float          mPos[3];
		unsigned short mBone[4];
		float          mWeight[4];
	};

	class SimpleSkinnedMesh
	{
	public:
		unsigned int         mVertexCount;
		SimpleSkinnedVertex *mVertices;

	};

	class AutoGeometry
	{
	public:

		virtual SimpleHull ** createCollisionVolumes(float collapse_percentage,          // percentage volume to collapse a child into a parent
			unsigned int bone_count,
			const SimpleBone *bones,
			const SimpleSkinnedMesh *mesh,
			unsigned int &geom_count) = 0;


		virtual SimpleHull ** createCollisionVolumes(float collapse_percentage,unsigned int &geom_count) = 0;

		virtual void addSimpleSkinnedTriangle(const SimpleSkinnedVertex &v1,
			const SimpleSkinnedVertex &v2,
			const SimpleSkinnedVertex &v3) = 0;

		virtual void addSimpleBone(const SimpleBone &b) = 0;

		virtual const char * stristr(const char *str,const char *match) = 0; // case insensitive ststr

	};

	AutoGeometry * createAutoGeometry();
	void           releaseAutoGeometry(AutoGeometry *g);

}; // end of namespace


#endif
