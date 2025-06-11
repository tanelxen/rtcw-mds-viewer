//
//  RenderableModel.cpp
//  hlmv
//
//  Created by Fedor Artemenkov on 17.11.24.
//

#include "RenderableModel.h"
#include <glad/glad.h>

#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/gtx/quaternion.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "../deps/stb_image.h"

RenderableModel::~RenderableModel()
{
//    printf("Delete %s", name.c_str());
//    
//    glDeleteTextures(textures.size(), textures.data());
//    glDeleteVertexArrays(1, &vao);
//    glDeleteBuffers(1, &vbo);
//    glDeleteBuffers(1, &ibo);
}

#define VERT_POSITION_LOC 0
#define VERT_NORMAL_LOC 1
#define VERT_TEX_COORD_LOC 2
#define VERT_BONE_INDEX_LOC 3

GLuint loadTexture(char const *filename)
{
    GLuint id;
    glGenTextures(1, &id);
    
    int width, height;
    int num_channels = 3;
    unsigned char* image = stbi_load(filename, &width, &height, &num_channels, 3);
    
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(image);
    
    return id;
}

void RenderableModel::init(const Model &model)
{
    int width, height;
    int num_channels = 3;
    
    GLuint body_tex_id = loadTexture("eg_body3.jpg");
    GLuint boot_tex_id = loadTexture("eg_boot.jpg");
    
    this->name = model.name_;
    
    Entity entity;
    entity.frame = 1135;
    entity.torsoFrame = 1135;
    entity.oldFrame = 1134;
    entity.oldTorsoFrame = 1134;
    entity.lerp = 0.0;
    entity.torsoLerp = 0.0;
    
    DrawCallList drawCallList;
    
    model.render(&drawCallList, &entity);
    
    for (int i = 0; i < drawCallList.size(); ++i)
    {
        auto& drawcall = drawCallList[i];
        auto& surface = surfaces.emplace_back();
        
        surface.bufferOffset = 0;
        surface.indicesCount = drawcall.indices.size();
        surface.tex = (i == 0) ? body_tex_id : boot_tex_id;
        
        glGenBuffers(1, &surface.ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, surface.ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * drawcall.indices.size(), drawcall.indices.data(), GL_STATIC_DRAW);
        
        glGenBuffers(1, &surface.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, surface.vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * drawcall.vertices.size(), drawcall.vertices.data(), GL_STATIC_DRAW);
        
        glGenVertexArrays(1, &surface.vao);
        glBindVertexArray(surface.vao);
        
        glEnableVertexAttribArray(VERT_POSITION_LOC);
        glVertexAttribPointer(VERT_POSITION_LOC, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
        
        glEnableVertexAttribArray(VERT_NORMAL_LOC);
        glVertexAttribPointer(VERT_NORMAL_LOC, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        
        glEnableVertexAttribArray(VERT_TEX_COORD_LOC);
        glVertexAttribPointer(VERT_TEX_COORD_LOC, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    }
}

void RenderableModel::updatePose()
{
}

void RenderableModel::update(float dt)
{
}

void RenderableModel::draw()
{
    for (auto& surface : surfaces)
    {
//        unsigned int texId = textures[surface.tex];
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, surface.tex);
        
        glBindVertexArray(surface.vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, surface.ibo);
        
        glDrawElements(GL_TRIANGLES, surface.indicesCount, GL_UNSIGNED_SHORT, (void*)surface.bufferOffset);
    }
}


