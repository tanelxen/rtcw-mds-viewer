//
//  MDSModel.cpp
//  wolfmv
//
//  Created by Fedor Artemenkov on 11.06.25.
//

#include "MDSModel.h"
#include "Skin.h"
#include "Utils.h"

#include <span>
#include <glad/glad.h>

#define VERT_POSITION_LOC 0
#define VERT_NORMAL_LOC 1
#define VERT_TEX_COORD_LOC 2

void MDSModel::loadFromFile(const std::string &filename, const SkinFile &skin)
{
    FILE* fp = fopen(filename.c_str(), "rb" );

    if(fp == nullptr) {
        printf("unable to open %s\n", filename.c_str());
    }
    
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    
    data_.resize(size);
    
    fseek(fp, 0, SEEK_SET);
    fread(data_.data(), size, 1, fp);
    fclose(fp);
    
    // Header
    header_ = (mdsHeader_t *)data_.data();
    
    if (header_->ident != MDS_IDENT)
    {
        printf("Model %s: wrong ident (%i should be %i)\n", filename.c_str(), header_->ident, MDS_IDENT);
        return false;
    }
    
    if (header_->version != MDS_VERSION)
    {
        printf("Model %s: wrong version (%i should be %i)\n", filename.c_str(), header_->version, MDS_VERSION);
        return false;
    }
    
    if (header_->numFrames < 1)
    {
        printf("Model %s: no frames\n", filename.c_str());
        return false;
    }
    
    boneInfo_ = (mdsBoneInfo_t *)(data_.data() + header_->ofsBones);
    frames_.resize(header_->numFrames);
    
    for (size_t i = 0; i < frames_.size(); i++)
    {
        const size_t frameSize = sizeof(mdsFrame_t) - sizeof(mdsBoneFrameCompressed_t) + header_->numBones * sizeof(mdsBoneFrameCompressed_t);
        frames_[i] = (mdsFrame_t *)(data_.data() + header_->ofsFrames + i * frameSize);
    }
    
    tags_ = (mdsTag_t *)(data_.data() + header_->ofsTags);
    
    for (const auto& [mesh, texture] : skin.textures)
    {
        m_textures[mesh] = loadTexture(texture.c_str());
    }
    
    m_shader.init("assets/shaders/md3.glsl");
    
    m_drawCallList.resize(numSurfaces());
    
    for (int i = 0; i < m_drawCallList.size(); ++i)
    {
        auto& drawCall = m_drawCallList[i];
        
        int numVertices = surfaceNumVertices(i);
        int numIndices = surfaceNumTriangles(i) * 3;
        
        drawCall.numVertices = numVertices;
        drawCall.numIndices = numIndices;
        
        glGenBuffers(1, &drawCall.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, drawCall.vbo);
        glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(Vertex), nullptr, GL_STREAM_DRAW);
        drawCall.verticesPtr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        
        
        glGenVertexArrays(1, &drawCall.vao);
        glBindVertexArray(drawCall.vao);
        
        glEnableVertexAttribArray(VERT_POSITION_LOC);
        glVertexAttribPointer(VERT_POSITION_LOC, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
        
        glEnableVertexAttribArray(VERT_NORMAL_LOC);
        glVertexAttribPointer(VERT_NORMAL_LOC, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        
        glEnableVertexAttribArray(VERT_TEX_COORD_LOC);
        glVertexAttribPointer(VERT_TEX_COORD_LOC, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
        
        
        glGenBuffers(1, &drawCall.ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawCall.ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * numIndices, nullptr, GL_STREAM_DRAW);
        drawCall.indicesPtr = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
    }
}

void MDSModel::render(const glm::mat4 &mvp, const MDSFrameInfo &entity)
{
    render(m_drawCallList, entity);
    
    m_shader.bind();
    m_shader.setUniform("uMVP", mvp);
    
    for (int i = 0; i < m_drawCallList.size(); ++i)
    {
        auto& drawCall = m_drawCallList[i];
        
        if (m_textures.contains(drawCall.name))
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_textures[drawCall.name]);
        }
        
        glBindVertexArray(drawCall.vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawCall.ibo);
        
        glDrawElements(GL_TRIANGLES, drawCall.numIndices, GL_UNSIGNED_SHORT, 0);
    }
}

void MDSModel::render(DrawCallList &drawCallList, const MDSFrameInfo &entity) const
{
    auto header = (mdsHeader_t *)data_.data();
    auto surface = (mdsSurface_t *)(data_.data() + header->ofsSurfaces);
    
    for (int i = 0; i < header->numSurfaces; i++)
    {
        assert(surface->numVerts > 0);
        assert(surface->numTriangles > 0);
        
        std::vector<uint16_t> tib(surface->numTriangles * 3);
        std::vector<Vertex> tvb(surface->numVerts);
        
        auto indices = (uint16_t *)tib.data();
        auto vertices = (Vertex *)tvb.data();
        auto mdsIndices = (const int *)((uint8_t *)surface + surface->ofsTriangles);
        
        for (int i = 0; i < surface->numTriangles * 3; i++)
        {
            indices[i] = mdsIndices[i];
        }
        
        Skeleton skeleton = calculateSkeleton(entity, (int *)((uint8_t *)surface + surface->ofsBoneReferences), surface->numBoneReferences);
        auto mdsVertex = (const mdsVertex_t *)((uint8_t *)surface + surface->ofsVerts);
        
        for (int i = 0; i < surface->numVerts; i++)
        {
            Vertex &v = vertices[i];
            v.pos = vec3::empty;
            
            for (int j = 0; j < mdsVertex->numWeights; j++)
            {
                const mdsWeight_t &weight = mdsVertex->weights[j];
                const Bone &bone = skeleton.bones[weight.boneIndex];
                v.pos += (bone.translation + bone.rotation.transform(weight.offset)) * weight.boneWeight;
            }
            
            v.normal = mdsVertex->normal;
            v.texCoord = mdsVertex->texCoords;
            
            // Move to the next vertex.
            mdsVertex = (mdsVertex_t *)&mdsVertex->weights[mdsVertex->numWeights];
        }
        
        auto& drawCall = drawCallList[i];
        
        drawCall.name = surface->name;
        
        if (drawCall.verticesPtr)
        {
            memcpy(drawCall.verticesPtr, vertices, sizeof(Vertex) * tvb.size());
        }
        
        if (drawCall.indicesPtr)
        {
            memcpy(drawCall.indicesPtr, indices, sizeof(uint16_t) * tib.size());
        }
        
        // Move to the next surface.
        surface = (mdsSurface_t *)((uint8_t *)surface + surface->ofsEnd);
    }
}

static mat4 Matrix4Transform(const mat3 &rotation, vec3 translation)
{
    // mat4::transform translation is 12,13,14
    mat4 m;
    m[0] = rotation[0][0]; m[4] = rotation[1][0]; m[8] = rotation[2][0];  m[12] = 0;
    m[1] = rotation[0][1]; m[5] = rotation[1][1]; m[9] = rotation[2][1];  m[13] = 0;
    m[2] = rotation[0][2]; m[6] = rotation[1][2]; m[10] = rotation[2][2]; m[14] = 0;
    m[3] = translation[0]; m[7] = translation[1]; m[11] = translation[2]; m[15] = 1;
    return m;
}

static void Matrix4Extract(const mat4 &m, mat3 *rotation, vec3 *translation)
{
    m.extract(rotation, nullptr);
    (*translation)[0] = m[3];
    (*translation)[1] = m[7];
    (*translation)[2] = m[11];
}

#define SHORT2ANGLE( x )  ( ( x ) * ( 360.0f / 65536 ) )

/*
 =================
 AngleNormalize360
 
 returns angle normalized to the range [0 <= angle < 360]
 =================
 */
static float AngleNormalize360(float angle) {
    return (360.0f / 65536) * ((int)(angle * (65536 / 360.0f)) & 65535);
}

/*
 =================
 AngleNormalize180
 
 returns angle normalized to the range [-180 < angle <= 180]
 =================
 */
static float AngleNormalize180(float angle) {
    angle = AngleNormalize360(angle);
    if (angle > 180.0) {
        angle -= 360.0;
    }
    return angle;
}

MDSModel::Bone MDSModel::calculateBoneRaw(const MDSFrameInfo &entity, int boneIndex, const Skeleton &skeleton) const
{
    const mdsBoneInfo_t &bi = boneInfo_[boneIndex];
    bool isTorso = false, fullTorso = false;
    const mdsBoneFrameCompressed_t *compressedTorsoBone = nullptr;
    
    if (bi.torsoWeight)
    {
        compressedTorsoBone = &skeleton.torsoFrame->bones[boneIndex];
        isTorso = true;
        
        if (bi.torsoWeight == 1.0f)
        {
            fullTorso = true;
        }
    }
    
    const mdsBoneFrameCompressed_t &compressedBone = skeleton.frame->bones[boneIndex];
    Bone bone;
    
    // we can assume the parent has already been uncompressed for this frame + lerp
    const Bone *parentBone = nullptr;
    
    if (bi.parent >= 0)
    {
        parentBone = &skeleton.bones[bi.parent];
    }
    
    // rotation
    vec3 angles;
    
    if (fullTorso)
    {
        for (int i = 0; i < 3; i++)
            angles[i] = SHORT2ANGLE(compressedTorsoBone->angles[i]);
    }
    else
    {
        for (int i = 0; i < 3; i++)
            angles[i] = SHORT2ANGLE(compressedBone.angles[i]);
        
        if (isTorso)
        {
            vec3 torsoAngles;
            
            for (int i = 0; i < 3; i++)
                torsoAngles[i] = SHORT2ANGLE(compressedTorsoBone->angles[i]);
            
            // blend the angles together
            for (int i = 0; i < 3; i++)
            {
                float diff = torsoAngles[i] - angles[i];
                
                if (fabs(diff) > 180)
                    diff = AngleNormalize180(diff);
                
                angles[i] = angles[i] + bi.torsoWeight * diff;
            }
        }
    }
    
    bone.rotation = mat3(angles);
    
    // translation
    if (parentBone)
    {
        vec3 vec;
        
        if (fullTorso)
        {
            angles[0] = SHORT2ANGLE(compressedTorsoBone->ofsAngles[0]);
            angles[1] = SHORT2ANGLE(compressedTorsoBone->ofsAngles[1]);
            angles[2] = 0;
            angles.toAngleVectors(&vec);
        }
        else
        {
            angles[0] = SHORT2ANGLE(compressedBone.ofsAngles[0]);
            angles[1] = SHORT2ANGLE(compressedBone.ofsAngles[1]);
            angles[2] = 0;
            angles.toAngleVectors(&vec);
            
            if (isTorso)
            {
                vec3 torsoAngles;
                torsoAngles[0] = SHORT2ANGLE(compressedTorsoBone->ofsAngles[0]);
                torsoAngles[1] = SHORT2ANGLE(compressedTorsoBone->ofsAngles[1]);
                torsoAngles[2] = 0;
                vec3 v2;
                torsoAngles.toAngleVectors(&v2);
                
                // blend the angles together
                vec = vec3::lerp(vec, v2, bi.torsoWeight);
            }
        }
        
        bone.translation = parentBone->translation + vec * bi.parentDist;
    }
    else // just use the frame position
    {
        bone.translation = frames_[entity.frame]->parentOffset;
    }
    
    return bone;
}

MDSModel::Bone MDSModel::calculateBoneLerp(const MDSFrameInfo &entity, int boneIndex, const Skeleton &skeleton) const
{
    const mdsBoneInfo_t &bi = boneInfo_[boneIndex];
    const Bone *parentBone = nullptr;
    
    if (bi.parent >= 0)
    {
        parentBone = &skeleton.bones[bi.parent];
    }
    
    bool isTorso = false, fullTorso = false;
    const mdsBoneFrameCompressed_t *compressedTorsoBone = nullptr, *oldCompressedTorsoBone = nullptr;
    
    if (bi.torsoWeight)
    {
        compressedTorsoBone = &skeleton.torsoFrame->bones[boneIndex];
        oldCompressedTorsoBone = &skeleton.oldTorsoFrame->bones[boneIndex];
        isTorso = true;
        
        if (bi.torsoWeight == 1.0f)
            fullTorso = true;
    }
    
    const mdsBoneFrameCompressed_t &compressedBone = skeleton.frame->bones[boneIndex];
    const mdsBoneFrameCompressed_t &oldCompressedBone = skeleton.oldFrame->bones[boneIndex];
    Bone bone;
    
    // rotation (take into account 170 to -170 lerps, which need to take the shortest route)
    vec3 angles;
    
    if (fullTorso)
    {
        for (int i = 0; i < 3; i++)
        {
            const float a1 = SHORT2ANGLE(compressedTorsoBone->angles[i]);
            const float a2 = SHORT2ANGLE(oldCompressedTorsoBone->angles[i]);
            const float diff = AngleNormalize180(a1 - a2);
            angles[i] = a1 - skeleton.torsoBackLerp * diff;
        }
    }
    else
    {
        for (int i = 0; i < 3; i++)
        {
            const float a1 = SHORT2ANGLE(compressedBone.angles[i]);
            const float a2 = SHORT2ANGLE(oldCompressedBone.angles[i]);
            const float diff = AngleNormalize180(a1 - a2);
            angles[i] = a1 - skeleton.backLerp * diff;
        }
        
        if (isTorso)
        {
            vec3 torsoAngles;
            
            for (int i = 0; i < 3; i++)
            {
                const float a1 = SHORT2ANGLE(compressedTorsoBone->angles[i]);
                const float a2 = SHORT2ANGLE(oldCompressedTorsoBone->angles[i]);
                const float diff = AngleNormalize180(a1 - a2);
                torsoAngles[i] = a1 - skeleton.torsoBackLerp * diff;
            }
            
            // blend the angles together
            for (int j = 0; j < 3; j++)
            {
                float diff = torsoAngles[j] - angles[j];
                
                if (fabs(diff) > 180)
                    diff = AngleNormalize180(diff);
                
                angles[j] = angles[j] + bi.torsoWeight * diff;
            }
        }
    }
    
    bone.rotation = mat3(angles);
    
    if (parentBone)
    {
        const short *sh1, *sh2;
        
        if (fullTorso)
        {
            sh1 = compressedTorsoBone->ofsAngles;
            sh2 = oldCompressedTorsoBone->ofsAngles;
        }
        else
        {
            sh1 = compressedBone.ofsAngles;
            sh2 = oldCompressedBone.ofsAngles;
        }
        
        angles[0] = SHORT2ANGLE(sh1[0]);
        angles[1] = SHORT2ANGLE(sh1[1]);
        angles[2] = 0;
        vec3 v2;
        angles.toAngleVectors(&v2); // new
        
        angles[0] = SHORT2ANGLE(sh2[0]);
        angles[1] = SHORT2ANGLE(sh2[1]);
        angles[2] = 0;
        vec3 vec;
        angles.toAngleVectors(&vec); // old
        
        // blend the angles together
        vec3 dir;
        
        if (fullTorso)
        {
            dir = vec3::lerp(vec, v2, skeleton.torsoFrontLerp);
        }
        else
        {
            dir = vec3::lerp(vec, v2, skeleton.frontLerp);
        }
        
        // translation
        if (!fullTorso && isTorso)
        {
            // partial legs/torso, need to lerp according to torsoWeight
            // calc the torso frame
            angles[0] = SHORT2ANGLE(compressedTorsoBone->ofsAngles[0]);
            angles[1] = SHORT2ANGLE(compressedTorsoBone->ofsAngles[1]);
            angles[2] = 0;
            vec3 v2;
            angles.toAngleVectors(&v2); // new
            
            angles[0] = SHORT2ANGLE(oldCompressedTorsoBone->ofsAngles[0]);
            angles[1] = SHORT2ANGLE(oldCompressedTorsoBone->ofsAngles[1]);
            angles[2] = 0;
            vec3 vec;
            angles.toAngleVectors(&vec); // old
            
            // blend the angles together
            v2 = vec3::lerp(vec, v2, skeleton.torsoFrontLerp);
            
            // blend the torso/legs together
            dir = vec3::lerp(dir, v2, bi.torsoWeight);
        }
        
        bone.translation = parentBone->translation + dir * bi.parentDist;
    }
    else
    {
        // just interpolate the frame positions
        const mdsFrame_t *frame = frames_[entity.frame], *oldFrame = frames_[entity.oldFrame];
        bone.translation[0] = skeleton.frontLerp * frame->parentOffset[0] + skeleton.backLerp * oldFrame->parentOffset[0];
        bone.translation[1] = skeleton.frontLerp * frame->parentOffset[1] + skeleton.backLerp * oldFrame->parentOffset[1];
        bone.translation[2] = skeleton.frontLerp * frame->parentOffset[2] + skeleton.backLerp * oldFrame->parentOffset[2];
    }
    
    return bone;
}

MDSModel::Bone MDSModel::calculateBone(const MDSFrameInfo &entity, int boneIndex, const Skeleton &skeleton, bool lerp) const
{
    return lerp ? calculateBoneLerp(entity, boneIndex, skeleton) : calculateBoneRaw(entity, boneIndex, skeleton);
}

MDSModel::Skeleton MDSModel::calculateSkeleton(const MDSFrameInfo &entity, int *boneList, int nBones) const
{
    assert(boneList);
    Skeleton skeleton;
    
    if (entity.oldFrame == entity.frame)
    {
        skeleton.backLerp = 0;
        skeleton.frontLerp = 1;
    }
    else
    {
        skeleton.backLerp = 1.0f - entity.lerp;
        skeleton.frontLerp = entity.lerp;
    }
    
    if (entity.oldTorsoFrame == entity.torsoFrame)
    {
        skeleton.torsoBackLerp = 0;
        skeleton.torsoFrontLerp = 1;
    }
    else
    {
        skeleton.torsoBackLerp = 1.0f - entity.torsoLerp;
        skeleton.torsoFrontLerp = entity.torsoLerp;
    }
    
    
    skeleton.frame = frames_[entity.frame];
    skeleton.oldFrame = frames_[entity.oldFrame];
    skeleton.torsoFrame = entity.torsoFrame >= 0 && entity.torsoFrame < (int)frames_.size() ? frames_[entity.torsoFrame] : nullptr;
    skeleton.oldTorsoFrame = entity.oldTorsoFrame >= 0 && entity.oldTorsoFrame < (int)frames_.size() ? frames_[entity.oldTorsoFrame] : nullptr;
    
    // Lerp all the needed bones (torsoParent is always the first bone in the list).
    int *boneRefs = boneList;
    mat3 torsoRotation(entity.torsoRotation);
    torsoRotation.transpose();
    const bool lerp = skeleton.backLerp || skeleton.torsoBackLerp;
    
    for (int i = 0; i < nBones; i++, boneRefs++)
    {
        if (skeleton.boneCalculated[*boneRefs])
            continue;
        
        // find our parent, and make sure it has been calculated
        const int parentBoneIndex = boneInfo_[*boneRefs].parent;
        
        if (parentBoneIndex >= 0 && !skeleton.boneCalculated[parentBoneIndex])
        {
            skeleton.bones[parentBoneIndex] = calculateBone(entity, parentBoneIndex, skeleton, lerp);
            skeleton.boneCalculated[parentBoneIndex] = true;
        }
        
        skeleton.bones[*boneRefs] = calculateBone(entity, *boneRefs, skeleton, lerp);
        skeleton.boneCalculated[*boneRefs] = true;
    }
    
    // Get the torso parent.
    vec3 torsoParentOffset;
    boneRefs = boneList;
    
    for (int i = 0; i < nBones; i++, boneRefs++)
    {
        if (*boneRefs == header_->torsoParent)
        {
            torsoParentOffset = skeleton.bones[*boneRefs].translation;
        }
    }
    
    // Adjust for torso rotations.
    float torsoWeight = 0;
    boneRefs = boneList;
    mat4 m2;
    
    for (int i = 0; i < nBones; i++, boneRefs++)
    {
        const mdsBoneInfo_t &bi = boneInfo_[*boneRefs];
        Bone *bone = &skeleton.bones[*boneRefs];
        
        // add torso rotation
        if (bi.torsoWeight > 0)
        {
            if (!(bi.flags & BONEFLAG_TAG))
            {
                // 1st multiply with the bone->matrix
                // 2nd translation for rotation relative to bone around torso parent offset
                const vec3 t = bone->translation - torsoParentOffset;
                mat4 m1 = Matrix4Transform(bone->rotation, t);
                // 3rd scaled rotation
                // 4th translate back to torso parent offset
                // use previously created matrix if available for the same weight
                if (torsoWeight != bi.torsoWeight)
                {
                    mat3 scaledRotation;
                    
                    for (int j = 0; j < 3; j++)
                    {
                        for (int k = 0; k < 3; k++)
                        {
                            scaledRotation[j][k] = torsoRotation[j][k] * bi.torsoWeight;
                            
                            if (j == k)
                                scaledRotation[j][k] += 1.0f - bi.torsoWeight;
                        }
                    }
                    
                    m2 = Matrix4Transform(scaledRotation, torsoParentOffset);
                    torsoWeight = bi.torsoWeight;
                }
                
                // multiply matrices to create one matrix to do all calculations
                Matrix4Extract(m1 * m2, &bone->rotation, &bone->translation);
            }
            else // tags require special handling
            {
                // rotate each of the axis by the torsoAngles
                for (int j = 0; j < 3; j++)
                    bone->rotation[j] = bone->rotation[j] * (1 - bi.torsoWeight) + torsoRotation.transform(bone->rotation[j]) * bi.torsoWeight;
                
                // rotate the translation around the torsoParent
                const vec3 t = bone->translation - torsoParentOffset;
                bone->translation = t * (1 - bi.torsoWeight) + torsoRotation.transform(t) * bi.torsoWeight + torsoParentOffset;
            }
        }
    }
    
    return skeleton;
}

int MDSModel::numSurfaces() const
{
    auto header = (mdsHeader_t *)data_.data();
    return header->numSurfaces;
}

int MDSModel::surfaceNumVertices(int surfaceIndex) const
{
    auto header = (mdsHeader_t *)data_.data();
    auto surface = (mdsSurface_t *)(data_.data() + header->ofsSurfaces);
    
    for (int i = 0; i < header->numSurfaces; i++)
    {
        if (i == surfaceIndex) return surface->numVerts;
        surface = (mdsSurface_t *)((uint8_t *)surface + surface->ofsEnd);
    }
    
    return 0;
}

int MDSModel::surfaceNumTriangles(int surfaceIndex) const
{
    auto header = (mdsHeader_t *)data_.data();
    auto surface = (mdsSurface_t *)(data_.data() + header->ofsSurfaces);
    
    for (int i = 0; i < header->numSurfaces; i++)
    {
        if (i == surfaceIndex) return surface->numTriangles;
        surface = (mdsSurface_t *)((uint8_t *)surface + surface->ofsEnd);
    }
    
    return 0;
}

void MDSModel::recursiveBoneListAdd(int boneIndex, int *boneList, int *nBones) const
{
    assert(boneList);
    assert(nBones);

    if (boneInfo_[boneIndex].parent >= 0)
    {
        recursiveBoneListAdd(boneInfo_[boneIndex].parent, boneList, nBones);
    }

    boneList[(*nBones)++] = boneIndex;
}

int MDSModel::lerpTag(const char *name, const MDSFrameInfo &entity, int startIndex, Transform *transform) const
{
    assert(transform);
    
    for (int i = 0; i < header_->numTags; i++)
    {
        const mdsTag_t &tag = tags_[i];
        
        if (i >= startIndex && !strcmp(tags_[i].name, name))
        {
            // Now build the list of bones we need to calc to get this tag's bone information.
            int boneList[MDS_MAX_BONES];
            int nBones = 0;
            recursiveBoneListAdd(tags_[i].boneIndex, boneList, &nBones);
            
            // Calculate the skeleton.
            Skeleton skeleton = calculateSkeleton(entity, boneList, nBones);
            
            // Now extract the transform for the bone that represents our tag.
            transform->position = skeleton.bones[tag.boneIndex].translation;
            transform->rotation = skeleton.bones[tag.boneIndex].rotation;
            return i;
        }
    }
    
    return -1;
}

MDSModel::~MDSModel()
{
    for (const auto& [mesh, texture] : m_textures)
    {
        glDeleteTextures(1, &texture);
    }
    
    for (const auto& drawCall : m_drawCallList)
    {
        glDeleteBuffers(1, &drawCall.ibo);
        glDeleteBuffers(1, &drawCall.vbo);
        glDeleteVertexArrays(1, &drawCall.vao);
    }
}
