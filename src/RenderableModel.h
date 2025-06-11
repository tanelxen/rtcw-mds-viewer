//
//  RenderableModel.hpp
//  hlmv
//
//  Created by Fedor Artemenkov on 17.11.24.
//

#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "MDSModel.h"

struct RenderableSurface
{
    unsigned int tex;
    int bufferOffset;
    int indicesCount;
    
    unsigned int vbo;
    unsigned int ibo;
    unsigned int vao;
};

struct RenderableModel
{
    ~RenderableModel();
    
    void init(const Model& model);
    void update(float dt);
    void draw();
    
    std::string name;
    
    // Transforms for each bone
    std::vector<glm::mat4> transforms;
    
private:
//    std::vector<Sequence> sequences;
    std::vector<int> bones;
    
    float cur_frame = 0;
    float cur_frame_time = 0;
    float cur_anim_duration = 0;
    int cur_seq_index = 0;
    
//    unsigned int vbo;
//    unsigned int ibo;
//    unsigned int vao;
    std::vector<unsigned int> textures;
    
    std::vector<RenderableSurface> surfaces;
    
private:
//    void uploadMeshes(const std::vector<Mesh>& meshes);
    
    void updatePose();
};
