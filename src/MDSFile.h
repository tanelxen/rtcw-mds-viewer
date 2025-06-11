//
//  MDSFile.h
//  wolfmv
//
//  Created by Fedor Artemenkov on 11.06.25.
//

#pragma once

#include "Math.h"

/*
 ==============================================================================
 
 MDS file format (Wolfenstein Skeletal Format)
 
 ==============================================================================
 */

#define MDS_IDENT           ( ( 'W' << 24 ) + ( 'S' << 16 ) + ( 'D' << 8 ) + 'M' )
#define MDS_VERSION         4
#define MDS_MAX_VERTS       6000
#define MDS_MAX_TRIANGLES   8192
#define MDS_MAX_BONES       128
#define MDS_MAX_SURFACES    32
#define MDS_MAX_TAGS        128

#define MDS_TRANSLATION_SCALE   ( 1.0 / 64 )
#define BONEFLAG_TAG        1       // this bone is actually a tag
#define MAX_QPATH 64

using namespace math;

struct mdsWeight_t
{
    int boneIndex;              // these are indexes into the boneReferences,
    float boneWeight;           // not the global per-frame bone list
    vec3 offset;
};

struct mdsVertex_t
{
    vec3 normal;
    vec2 texCoords;
    int numWeights;
    int fixedParent;            // stay equi-distant from this parent
    float fixedDist;
    mdsWeight_t weights[1];     // variable sized
};

struct mdsTriangle_t
{
    int indexes[3];
};

struct mdsSurface_t
{
    int ident;
    
    char name[MAX_QPATH];           // polyset name
    char shader[MAX_QPATH];
    int shaderIndex;                // for in-game use
    
    int minLod;
    
    int ofsHeader;                  // this will be a negative number
    
    int numVerts;
    int ofsVerts;
    
    int numTriangles;
    int ofsTriangles;
    
    int ofsCollapseMap;           // numVerts * int
    
    int numBoneReferences;
    int ofsBoneReferences;
    
    int ofsEnd;                     // next surface follows
};

struct mdsBoneFrameCompressed_t
{
    short angles[4];            // to be converted to axis at run-time (this is also better for lerping)
    short ofsAngles[2];         // PITCH/YAW, head in this direction from parent to go to the offset position
};

struct mdsFrame_t
{
    vec3 mins, maxs;              // bounds of all surfaces of all LOD's for this frame
    vec3 localOrigin;             // midpoint of bounds, used for sphere cull
    float radius;                   // dist from localOrigin to corner
    vec3 parentOffset;            // one bone is an ascendant of all other bones, it starts the hierachy at this position
    mdsBoneFrameCompressed_t bones[1];              // [numBones]
};

struct mdsLOD_t
{
    int numSurfaces;
    int ofsSurfaces;                // first surface, others follow
    int ofsEnd;                     // next lod follows
};

struct mdsTag_t
{
    char name[MAX_QPATH];           // name of tag
    float torsoWeight;
    int boneIndex;                  // our index in the bones
};

struct mdsBoneInfo_t
{
    char name[MAX_QPATH];           // name of bone
    int parent;                     // not sure if this is required, no harm throwing it in
    float torsoWeight;              // scale torso rotation about torsoParent by this
    float parentDist;
    int flags;
};

struct mdsHeader_t
{
    int ident;
    int version;
    
    char name[MAX_QPATH];           // model name
    
    float lodScale;
    float lodBias;
    
    // frames and bones are shared by all levels of detail
    int numFrames;
    int numBones;
    int ofsFrames;                  // mdsFrame_t[numFrames]
    int ofsBones;                   // mdsBoneInfo_t[numBones]
    int torsoParent;                // index of bone that is the parent of the torso
    
    int numSurfaces;
    int ofsSurfaces;
    
    // tag data
    int numTags;
    int ofsTags;                    // mdsTag_t[numTags]
    
    int ofsEnd;                     // end of file
};
