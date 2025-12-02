//
//  WolfCharacter.h
//  wolfmv
//
//  Created by Fedor Artemenkov on 16.06.25.
//

#pragma once

#include "MDSModel.h"
#include "MD3Model.h"

#include <glm/glm.hpp>
#include <filesystem>

struct SkinFile;
struct AnimationEntry;

// Combination of body.mds and other tags (head etc) according to selected skin

struct WolfCharacter
{
    void init(const std::filesystem::path& dir, const std::string &skinName);
    void setAnimation(const AnimationEntry& sequence);
    
    void update(float dt);
    void draw(const glm::mat4 &mvp);
    
    std::string m_name;
    
private:
    float cur_frame = 0;
    float cur_frame_time = 0;
    float cur_anim_duration = 0;
    
    int startFrame = 0;
    int numFrames = 1;
    int fps = 15;
    
    MDSModel body;
    MD3Model head;
    
    std::unordered_map<std::string, MD3Model> attachments;
    
    MDSFrameInfo entity;
    
    void updatePose();
};
