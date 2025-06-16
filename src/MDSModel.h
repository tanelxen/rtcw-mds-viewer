//
//  MDSModel.hpp
//  wolfmv
//
//  Created by Fedor Artemenkov on 11.06.25.
//

#pragma once

#include <vector>
#include <string>
#include <unordered_map>

#include "MDSFile.h"
#include "DrawCall.h"
#include "Shader.h"

struct SkinFile;

struct MDSFrameInfo
{
    int frame, torsoFrame;
    int oldFrame, oldTorsoFrame;
    float lerp, torsoLerp;
    
    mat3 torsoRotation;
};

struct MDSModel
{
    void loadFromFile(const std::string& filename, const SkinFile &skin);
    void render(const glm::mat4 &mvp, const MDSFrameInfo &entity);
    int lerpTag(const char *name, const MDSFrameInfo &entity, int startIndex, Transform *transform) const;
    
    ~MDSModel();
    
private:
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
    Bone calculateBoneRaw(const MDSFrameInfo &entity, int boneIndex, const Skeleton &skeleton) const;
    Bone calculateBoneLerp(const MDSFrameInfo &entity, int boneIndex, const Skeleton &skeleton) const;
    Bone calculateBone(const MDSFrameInfo &entity, int boneIndex, const Skeleton &skeleton, bool lerp) const;
    Skeleton calculateSkeleton(const MDSFrameInfo &entity, int *boneList, int nBones) const;
    
    // Render stuff
private:
    std::unordered_map<std::string, unsigned int> m_textures;
    Shader m_shader;
    DrawCallList m_drawCallList;
    
    void render(DrawCallList &drawCallList, const MDSFrameInfo &entity) const;
    
    int numSurfaces() const;
    int surfaceNumVertices(int surfaceIndex) const;
    int surfaceNumTriangles(int surfaceIndex) const;
};
