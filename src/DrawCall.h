//
//  DrawCall.h
//  wolfmv
//
//  Created by Fedor Artemenkov on 13.06.25.
//

#pragma once

#include "Math.h"

using namespace math;

struct Vertex
{
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
};

struct DrawCall
{
    std::string name;
    
    uint32_t vbo;
    uint32_t numVertices;
    void* verticesPtr;
    
    uint32_t ibo;
    uint32_t numIndices;
    void* indicesPtr;
    
    uint32_t vao;
};

typedef std::vector<DrawCall> DrawCallList;
