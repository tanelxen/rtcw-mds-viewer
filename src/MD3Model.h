//
//  MD3Model.hpp
//  wolfmv
//
//  Created by Fedor Artemenkov on 13.06.25.
//

#pragma once

#include <vector>
#include <string>
#include <unordered_map>

#include "DrawCall.h"
#include "Shader.h"

#include "MD3File.h"

struct SkinFile;

struct MD3Model
{
    void loadFromFile(const std::string& filename, const SkinFile &skin);
    void render(glm::mat4 &mvp);
    
    ~MD3Model();
    
private:
    struct Frame
    {
        std::vector<Transform> tags;
        
        /// Vertex data in system memory. Used by animated models.
        std::vector<Vertex> vertices;
    };
    
    struct Surface
    {
        char name[MAX_QPATH]; // polyset name
//        uint32_t startIndex;
//        uint32_t nIndices;
//        uint32_t startVertex;
//        uint32_t nVertices;
        
        std::vector<Vertex> vertices;
        std::vector<uint16_t> indices;
    };
    
    struct TagName
    {
        char name[MAX_QPATH];
    };
    
//    vec3 decodeNormal(short normal) const;
    int getTag(const char *name, int frame, int startIndex, Transform *transform) const;
    
    bool compressed_;
    
    std::vector<uint16_t> indices_;
    
    /// Static model vertex buffer.
//    VertexBuffer vertexBuffer_;
    
    /// The number of vertices in all the surfaces of a single frame.
    uint32_t nVertices_;
    
    std::vector<Frame> frames_;
    std::vector<TagName> tagNames_;
    std::vector<Surface> surfaces_;
    
private:
    std::unordered_map<std::string, unsigned int> m_textures;
    Shader m_shader;
    DrawCallList m_drawCallList;
    
    void render(DrawCallList &drawCallList) const;
    
    int surfaceNumVertices(int surfaceIndex) const;
    int surfaceNumIndices(int surfaceIndex) const;
};
