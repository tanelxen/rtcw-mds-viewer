//
//  MDSModel.hpp
//  wolfmv
//
//  Created by Fedor Artemenkov on 11.06.25.
//

#pragma once

#include <vector>
#include <string>

#include "MDSFile.h"

struct Entity
{
    int frame, torsoFrame;
    int oldFrame, oldTorsoFrame;
    float lerp, torsoLerp;
    
    mat3 torsoRotation;
};

struct Vertex
{
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
};

struct DrawCall
{
    uint32_t vbo;
    uint32_t numVertices;
    void* verticesPtr;
    
    uint32_t ibo;
    uint32_t numIndices;
    void* indicesPtr;
    
    uint32_t vao;
};

typedef std::vector<DrawCall> DrawCallList;

struct Model
{
    void loadFromFile(const std::string& filename);
    void render(DrawCallList &drawCallList, const Entity *entity) const;
    
    std::string name_;
    
    int numSurfaces() const;
    int surfaceNumVertices(int surfaceIndex) const;
    int surfaceNumTriangles(int surfaceIndex) const;
    
private:
    void init();
    
    std::vector<uint8_t> data_;
    const mdsHeader_t *header_;
    const mdsBoneInfo_t *boneInfo_;
    std::vector<const mdsFrame_t *> frames_;
    const mdsTag_t *tags_;
    
    struct Bone
    {
        mat3 rotation;
        vec3 translation;
    };
    
    struct Skeleton
    {
        Bone bones[MDS_MAX_BONES];
        bool boneCalculated[MDS_MAX_BONES] = { false };
        const mdsFrame_t *frame, *oldFrame;
        const mdsFrame_t *torsoFrame, *oldTorsoFrame;
        float frontLerp, backLerp;
        float torsoFrontLerp, torsoBackLerp;
    };
    
    void recursiveBoneListAdd(int boneIndex, int *boneList, int *nBones) const;
    Bone calculateBoneRaw(const Entity &entity, int boneIndex, const Skeleton &skeleton) const;
    Bone calculateBoneLerp(const Entity &entity, int boneIndex, const Skeleton &skeleton) const;
    Bone calculateBone(const Entity &entity, int boneIndex, const Skeleton &skeleton, bool lerp) const;
    Skeleton calculateSkeleton(const Entity &entity, int *boneList, int nBones) const;
};
