//
//  MD3File.h
//  wolfmv
//
//  Created by Fedor Artemenkov on 16.06.25.
//

#pragma once

#define MAX_QPATH 64

typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];

using namespace math;

/*
 ========================================================================
 
 .MD3 triangle model file format
 
 ========================================================================
 */

#define MD3_IDENT            (('3'<<24)+('P'<<16)+('D'<<8)+'I')
#define MD3_VERSION            15

// limits
#define MD3_MAX_LODS        3
#define    MD3_MAX_TRIANGLES    8192    // per surface
#define MD3_MAX_VERTS        4096    // per surface
#define MD3_MAX_SHADERS        256        // per surface
#define MD3_MAX_FRAMES        1024    // per model
#define    MD3_MAX_SURFACES    32        // per model
#define MD3_MAX_TAGS        16        // per frame

// vertex scales
#define    MD3_XYZ_SCALE        (1.0f/64)

typedef struct md3Frame_s {
    vec3_t        bounds[2];
    vec3_t        localOrigin;
    float        radius;
    char        name[16];
} md3Frame_t;

typedef struct md3Tag_s {
    char        name[MAX_QPATH];    // tag name
    vec3_t        origin;
    vec3_t        axis[3];
} md3Tag_t;

/*
 ** md3Surface_t
 **
 ** CHUNK            SIZE
 ** header            sizeof( md3Surface_t )
 ** shaders            sizeof( md3Shader_t ) * numShaders
 ** triangles[0]        sizeof( md3Triangle_t ) * numTriangles
 ** st                sizeof( md3St_t ) * numVerts
 ** XyzNormals        sizeof( md3XyzNormal_t ) * numVerts * numFrames
 */
typedef struct {
    int        ident;                //
    
    char    name[MAX_QPATH];    // polyset name
    
    int        flags;
    int        numFrames;            // all surfaces in a model should have the same
    
    int        numShaders;            // all surfaces in a model should have the same
    int        numVerts;
    
    int        numTriangles;
    int        ofsTriangles;
    
    int        ofsShaders;            // offset from start of md3Surface_t
    int        ofsSt;                // texture coords are common for all frames
    int        ofsXyzNormals;        // numVerts * numFrames
    
    int        ofsEnd;                // next surface follows
} md3Surface_t;

typedef struct {
    char            name[MAX_QPATH];
    int                shaderIndex;    // for in-game use
} md3Shader_t;

typedef struct {
    int            indexes[3];
} md3Triangle_t;

typedef struct {
    float        st[2];
} md3St_t;

typedef struct {
    short        xyz[3];
    short        normal;
} md3XyzNormal_t;

typedef struct {
    int            ident;
    int            version;
    
    char        name[MAX_QPATH];    // model name
    
    int            flags;
    
    int            numFrames;
    int            numTags;
    int            numSurfaces;
    
    int            numSkins;
    
    int            ofsFrames;            // offset for first frame
    int            ofsTags;            // numFrames * numTags
    int            ofsSurfaces;        // first surface, others follow
    
    int            ofsEnd;                // end of file
} md3Header_t;

/*
 ==============================================================================
 
 MDC file format
 
 ==============================================================================
 */

#define MDC_IDENT           ( ( 'C' << 24 ) + ( 'P' << 16 ) + ( 'D' << 8 ) + 'I' )
#define MDC_VERSION         2

// version history:
// 1 - original
// 2 - changed tag structure so it only lists the names once

typedef struct {
    unsigned int ofsVec;                    // offset direction from the last base frame
    //    unsigned short    ofsVec;
} mdcXyzCompressed_t;

typedef struct {
    char name[MAX_QPATH];           // tag name
} mdcTagName_t;

#define MDC_TAG_ANGLE_SCALE ( 360.0f / 32700.0f )

typedef struct {
    short xyz[3];
    short angles[3];
} mdcTag_t;

/*
 ** mdcSurface_t
 **
 ** CHUNK            SIZE
 ** header            sizeof( md3Surface_t )
 ** shaders            sizeof( md3Shader_t ) * numShaders
 ** triangles[0]        sizeof( md3Triangle_t ) * numTriangles
 ** st                sizeof( md3St_t ) * numVerts
 ** XyzNormals        sizeof( md3XyzNormal_t ) * numVerts * numBaseFrames
 ** XyzCompressed    sizeof( mdcXyzCompressed ) * numVerts * numCompFrames
 ** frameBaseFrames    sizeof( short ) * numFrames
 ** frameCompFrames    sizeof( short ) * numFrames (-1 if frame is a baseFrame)
 */
typedef struct {
    int ident;                  //
    
    char name[MAX_QPATH];       // polyset name
    
    int flags;
    int numCompFrames;          // all surfaces in a model should have the same
    int numBaseFrames;          // ditto
    
    int numShaders;             // all surfaces in a model should have the same
    int numVerts;
    
    int numTriangles;
    int ofsTriangles;
    
    int ofsShaders;             // offset from start of md3Surface_t
    int ofsSt;                  // texture coords are common for all frames
    int ofsXyzNormals;          // numVerts * numBaseFrames
    int ofsXyzCompressed;       // numVerts * numCompFrames
    
    int ofsFrameBaseFrames;     // numFrames
    int ofsFrameCompFrames;     // numFrames
    
    int ofsEnd;                 // next surface follows
} mdcSurface_t;

typedef struct {
    int ident;
    int version;
    
    char name[MAX_QPATH];           // model name
    
    int flags;
    
    int numFrames;
    int numTags;
    int numSurfaces;
    
    int numSkins;
    
    int ofsFrames;                  // offset for first frame, stores the bounds and localOrigin
    int ofsTagNames;                // numTags
    int ofsTags;                    // numFrames * numTags
    int ofsSurfaces;                // first surface, others follow
    
    int ofsEnd;                     // end of file
} mdcHeader_t;

// NOTE: MDC_MAX_ERROR is effectively the compression level. the lower this value, the higher
// the accuracy, but with lower compression ratios.
#define MDC_MAX_ERROR       0.1f     // if any compressed vert is off by more than this from the
// actual vert, make this a baseframe

#define MDC_DIST_SCALE      0.05f    // lower for more accuracy, but less range

// note: we are locked in at 8 or less bits since changing to byte-encoded normals
#define MDC_BITS_PER_AXIS   8
#define MDC_MAX_OFS         127.0f   // to be safe

#define MDC_MAX_DIST        ( MDC_MAX_OFS * MDC_DIST_SCALE )
