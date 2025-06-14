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
#include "MD3Model.h"
#include "WolfAnim.h"
#include "Skin.h"

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
    
    void init(const MDSModel& mds, const MD3Model& md3, const SkinFile &bodySkin, const SkinFile &headSkin);
    void update(float dt);
    
    void draw(glm::mat4 &mvp);
    
    void imguiDraw();
    
    void setAnimation(const AnimationEntry& entry);
    
    std::string name;
    
    // Transforms for each bone
    std::vector<glm::mat4> transforms;
    
private:
//    std::vector<Sequence> sequences;
    std::vector<int> bones;
    
    float cur_frame = 0;
    float cur_frame_time = 0;
    float cur_anim_duration = 0;
//    int cur_seq_index = 0;
    
//    unsigned int vbo;
//    unsigned int ibo;
//    unsigned int vao;
//    std::vector<unsigned int> textures;
    
    std::vector<RenderableSurface> surfaces;
    
    DrawCallList drawCallList;
    DrawCallList drawCallList2;
    
private:
//    void uploadMeshes(const std::vector<Mesh>& meshes);
    
    void updatePose();
};
