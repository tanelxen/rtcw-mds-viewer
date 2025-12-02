//
//  MD3Model.cpp
//  wolfmv
//
//  Created by Fedor Artemenkov on 13.06.25.
//

#include "MD3Model.h"
#include "Skin.h"
#include "Utils.h"

#include <glad/glad.h>

#define VERT_POSITION_LOC 0
#define VERT_NORMAL_LOC 1
#define VERT_TEX_COORD_LOC 2

struct FileHeader
{
    int ident;
    int version;
    int nFrames;
    int nTags;
    int nSurfaces;
    int nSkins;
    int framesOffset;
    int tagNamesOffset; // compressed only
    int tagsOffset;
    int surfacesOffset;
};

struct FileSurface
{
    uint8_t *offset;
    char name[MAX_QPATH];
    int nCompressedFrames; // compressed only
    int nBaseFrames; // compressed only
    int nShaders;
    int nVertices;
    int nTriangles;
    int trianglesOffset;
    int shadersOffset;
    int uvsOffset;
    int positionNormalOffset;
    int positionNormalCompressedOffset; // compressed only
    int baseFramesOffset; // compressed only
    int compressedFramesOffset; // compressed only
};

namespace util {
    void Strncpyz(char *dest, const char *src, int destsize)
    {
        strncpy(dest, src, destsize - 1);
        dest[destsize - 1] = 0;
    }
}

void MD3Model::loadFromFile(const std::string &filename, const SkinFile &skin)
{
    compressed_ = true;
    std::vector<uint8_t> data;
    
    FILE* fp = fopen(filename.c_str(), "rb" );
    
    if(fp == nullptr) {
        printf("unable to open %s\n", filename.c_str());
    }
    
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    
    data.resize(size);
    
    fseek(fp, 0, SEEK_SET);
    fread(data.data(), size, 1, fp);
    fclose(fp);
    
    // Header
    FileHeader header;
    
    if (compressed_)
    {
        auto fileHeader = (mdcHeader_t *)data.data();
        header.ident = fileHeader->ident;
        header.version = fileHeader->version;
        header.nFrames = fileHeader->numFrames;
        header.nTags = fileHeader->numTags;
        header.nSurfaces = fileHeader->numSurfaces;
        header.nSkins = fileHeader->numSkins;
        header.framesOffset = fileHeader->ofsFrames;
        header.tagsOffset = fileHeader->ofsTags;
        header.surfacesOffset = fileHeader->ofsSurfaces;
        header.tagNamesOffset = fileHeader->ofsTagNames;
    }
    else
    {
        auto fileHeader = (md3Header_t *)data.data();
        header.ident = fileHeader->ident;
        header.version = fileHeader->version;
        header.nFrames = fileHeader->numFrames;
        header.nTags = fileHeader->numTags;
        header.nSurfaces = fileHeader->numSurfaces;
        header.nSkins = fileHeader->numSkins;
        header.framesOffset = fileHeader->ofsFrames;
        header.tagsOffset = fileHeader->ofsTags;
        header.surfacesOffset = fileHeader->ofsSurfaces;
    }
    
    const int validIdent = compressed_ ? MDC_IDENT : MD3_IDENT;
    const int validVersion = compressed_ ? MDC_VERSION : MD3_VERSION;
    
    if (header.ident != validIdent)
    {
        printf("Model %s: wrong ident (%i should be %i)\n", filename.c_str(), header.ident, validIdent);
        return;
    }
    
    if (header.version != validVersion)
    {
        printf("Model %s: wrong version (%i should be %i)\n", filename.c_str(), header.version, validVersion);
        return;
    }
    
    if (header.nFrames < 1)
    {
        printf("Model %s: no frames\n", filename.c_str());
        return;
    }
    
    // Frames
    auto fileFrames = (const md3Frame_t *)&data[header.framesOffset];
    frames_.resize(header.nFrames);
    
    for (int i = 0; i < header.nFrames; i++)
    {
        Frame &frame = frames_[i];
        const md3Frame_t &fileFrame = fileFrames[i];
        
        // Tags
        frame.tags.resize(header.nTags);
        
        for (int j = 0; j < header.nTags; j++)
        {
            Transform &tag = frame.tags[j];
            
            if (compressed_)
            {
                const auto &fileTag = ((const mdcTag_t *)&data[header.tagsOffset])[j + i * header.nTags];
                vec3 angles;
                
                for (int k = 0; k < 3; k++)
                {
                    tag.position[k] = (float)fileTag.xyz[k] * MD3_XYZ_SCALE;
                    angles[k] = (float)fileTag.angles[k] * MDC_TAG_ANGLE_SCALE;
                }
                
                tag.rotation = mat3(angles);
            }
            else
            {
                const auto &fileTag = ((const md3Tag_t *)&data[header.tagsOffset])[j + i * header.nTags];
                tag.position = fileTag.origin;
                tag.rotation[0] = fileTag.axis[0];
                tag.rotation[1] = fileTag.axis[1];
                tag.rotation[2] = fileTag.axis[2];
            }
        }
    }
    
    // Tag names
    tagNames_.resize(header.nTags);
    
    if (compressed_)
    {
        auto fileTagNames = (const mdcTagName_t *)&data[header.tagNamesOffset];
        
        for (int i = 0; i < header.nTags; i++)
        {
            util::Strncpyz(tagNames_[i].name, fileTagNames[i].name, sizeof(tagNames_[i].name));
        }
    }
    else
    {
        auto fileTags = (const md3Tag_t *)&data[header.tagsOffset];
        
        for (int i = 0; i < header.nTags; i++)
        {
            util::Strncpyz(tagNames_[i].name, fileTags[i].name, sizeof(tagNames_[i].name));
        }
    }
    
    // Copy uncompressed and compression surface data into a common struct.
    std::vector<FileSurface> fileSurfaces(header.nSurfaces);
    
    if (header.surfacesOffset < data.size())
    {
        if (compressed_)
        {
            auto mdcSurface = (mdcSurface_t *)&data[header.surfacesOffset];
            
            for (int i = 0; i < header.nSurfaces; i++)
            {
                FileSurface &fs = fileSurfaces[i];
                fs.offset = (uint8_t *)mdcSurface;
                util::Strncpyz(fs.name, mdcSurface->name, sizeof(fs.name));
                fs.nCompressedFrames = mdcSurface->numCompFrames;
                fs.nBaseFrames = mdcSurface->numBaseFrames;
                fs.nShaders = mdcSurface->numShaders;
                fs.nVertices = mdcSurface->numVerts;
                fs.nTriangles = mdcSurface->numTriangles;
                fs.trianglesOffset = mdcSurface->ofsTriangles;
                fs.shadersOffset = mdcSurface->ofsShaders;
                fs.uvsOffset = mdcSurface->ofsSt;
                fs.positionNormalOffset = mdcSurface->ofsXyzNormals;
                fs.positionNormalCompressedOffset = mdcSurface->ofsXyzCompressed;
                fs.baseFramesOffset = mdcSurface->ofsFrameBaseFrames;
                fs.compressedFramesOffset = mdcSurface->ofsFrameCompFrames;
                
                // Move to the next surface.
                mdcSurface = (mdcSurface_t *)((uint8_t *)mdcSurface + mdcSurface->ofsEnd);
            }
        }
        else
        {
            auto md3Surface = (md3Surface_t *)&data[header.surfacesOffset];
            
            for (int i = 0; i < header.nSurfaces; i++)
            {
                FileSurface &fs = fileSurfaces[i];
                fs.offset = (uint8_t *)md3Surface;
                util::Strncpyz(fs.name, md3Surface->name, sizeof(fs.name));
                fs.nShaders = md3Surface->numShaders;
                fs.nVertices = md3Surface->numVerts;
                fs.nTriangles = md3Surface->numTriangles;
                fs.trianglesOffset = md3Surface->ofsTriangles;
                fs.shadersOffset = md3Surface->ofsShaders;
                fs.uvsOffset = md3Surface->ofsSt;
                fs.positionNormalOffset = md3Surface->ofsXyzNormals;
                
                // Move to the next surface.
                md3Surface = (md3Surface_t *)((uint8_t *)md3Surface + md3Surface->ofsEnd);
            }
        }
    }
    else
    {
        printf("Something wrong with %s", filename.c_str());
    }
    
    // Surfaces
    surfaces_.resize(header.nSurfaces);
    
    for (int i = 0; i < header.nSurfaces; i++)
    {
        FileSurface &fs = fileSurfaces[i];
        Surface &surface = surfaces_[i];
        
        strcpy(surface.name, fs.name);
        
        int numIndices = fs.nTriangles * 3;
        auto fileIndices = (int *)(fs.offset + fs.trianglesOffset);
        
        surface.indices.resize(numIndices);
        
        for (uint32_t j = 0; j < numIndices; j++)
        {
            surface.indices[j] = fileIndices[j];
        }
        
        auto fileTexCoords = (md3St_t *)(fs.offset + fs.uvsOffset);
        
        auto fileBaseFrames = (short *)(fs.offset + fs.baseFramesOffset);
        auto fileCompressedFrames = (short *)(fs.offset + fs.compressedFramesOffset);
        
        int j = 0;
        int positionNormalFrame;
        
        if (compressed_)
        {
            positionNormalFrame = fileBaseFrames[j];
        }
        else
        {
            positionNormalFrame = j;
        }
        
        auto fileXyzNormals = (md3XyzNormal_t *)(fs.offset + fs.positionNormalOffset + positionNormalFrame * sizeof(md3XyzNormal_t) * fs.nVertices);
        
        surface.vertices.resize(fs.nVertices);
        
        for (int k = 0; k < fs.nVertices; k++)
        {
            Vertex &v = surface.vertices[k];
            v.pos.x = fileXyzNormals[k].xyz[0] * MD3_XYZ_SCALE;
            v.pos.y = fileXyzNormals[k].xyz[1] * MD3_XYZ_SCALE;
            v.pos.z = fileXyzNormals[k].xyz[2] * MD3_XYZ_SCALE;
//            v.normal = decodeNormal(fileXyzNormals[k].normal);
            v.texCoord.x = fileTexCoords[k].st[0];
            v.texCoord.y = fileTexCoords[k].st[1];
            
            if (compressed_)
            {
                // If compressedFrameIndex isn't -1, use compressedFrameIndex as a delta from baseFrameIndex.
                short compressedFrameIndex = fileCompressedFrames[j];
                
                if (compressedFrameIndex != -1)
                {
                    auto fileXyzCompressed = (mdcXyzCompressed_t *)(fs.offset + fs.positionNormalCompressedOffset + compressedFrameIndex * sizeof(mdcXyzCompressed_t) * fs.nVertices);
                    vec3 delta;
                    delta[0] = (float((fileXyzCompressed[k].ofsVec) & 255) - MDC_MAX_OFS) * MDC_DIST_SCALE;
                    delta[1] = (float((fileXyzCompressed[k].ofsVec >> 8) & 255) - MDC_MAX_OFS) * MDC_DIST_SCALE;
                    delta[2] = (float((fileXyzCompressed[k].ofsVec >> 16) & 255) - MDC_MAX_OFS) * MDC_DIST_SCALE;
                    v.pos += delta;
//                    v.normal = vec3(s_anormals[fileXyzCompressed[k].ofsVec >> 24]);
                }
            }
        }
    }
    
    for (const auto& [mesh, texture] : skin.textures)
    {
        m_textures[mesh] = loadTexture(texture.c_str());
    }
    
    m_shader.init("assets/shaders/md3.glsl");
    
    m_drawCallList.resize(surfaces_.size());
    
    for (int i = 0; i < m_drawCallList.size(); ++i)
    {
        auto& surface = surfaces_[i];
        auto& drawCall = m_drawCallList[i];
        
        drawCall.name = surface.name;
        
        int numVertices = (int)surface.vertices.size();
        int numIndices = (int)surface.indices.size();
        
        drawCall.numVertices = numVertices;
        drawCall.numIndices = numIndices;
        
        glGenBuffers(1, &drawCall.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, drawCall.vbo);
        glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(Vertex), surface.vertices.data(), GL_STATIC_DRAW);
        
        
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
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * numIndices, surface.indices.data(), GL_STREAM_DRAW);
    }
}

void MD3Model::render(glm::mat4 &mvp)
{
    m_shader.bind();
    m_shader.setUniform("uMVP", mvp);
    
    for (int i = 0; i < m_drawCallList.size(); ++i)
    {
        auto& drawCall = m_drawCallList[i];
        
        if (drawCall.name == "h_blink") continue;
        
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

// Just for animated models
void MD3Model::render(DrawCallList &drawCallList) const
{
    for (int i = 0; i < surfaces_.size(); ++i)
    {
        auto& drawCall = drawCallList[i];
        auto& surface = surfaces_[i];
        
        drawCall.name = surface.name;
        
        if (drawCall.verticesPtr)
        {
            memcpy(drawCall.verticesPtr, surface.vertices.data(), sizeof(Vertex) * surface.vertices.size());
        }
        
        if (drawCall.indicesPtr)
        {
            memcpy(drawCall.indicesPtr, surface.indices.data(), sizeof(uint16_t) * surface.indices.size());
        }
    }
}

int MD3Model::surfaceNumVertices(int surfaceIndex) const
{
    return surfaces_[surfaceIndex].vertices.size();
}

int MD3Model::surfaceNumIndices(int surfaceIndex) const
{
    return surfaces_[surfaceIndex].indices.size();
}

MD3Model::~MD3Model()
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
