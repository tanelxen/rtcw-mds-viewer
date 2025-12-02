//
//  WolfCharacter.cpp
//  wolfmv
//
//  Created by Fedor Artemenkov on 16.06.25.
//

#include "WolfCharacter.h"

#include "WolfAnim.h"
#include "Skin.h"
#include "Utils.h"

void WolfCharacter::init(const std::string &dir, const std::string &skinName)
{
    std::string bodyMDSPath = dir + "/body.mds";
    
    std::string bodySkinPath = dir + "/body_" + skinName + ".skin";
    auto bodySkin = parseSkinFile(bodySkinPath);
    
    body.loadFromFile(bodyMDSPath, bodySkin);
    
    std::string headSkinPath = dir + "/head_" + skinName + ".skin";
    auto headSkin = parseSkinFile(headSkinPath);
    
    auto headMD3path = dir + "head.mdc";
    
    if (headSkin.attachments.contains("md3_part"))
    {
        headMD3path = dir + headSkin.attachments["md3_part"];
    }
    
    headMD3path = resolvePath(headMD3path, {".mdc"});
    
    head.loadFromFile(headMD3path, headSkin);
    
//    for (auto& attachment : bodySkin.attachments)
//    {
//        attachment.
//    }
}

void WolfCharacter::setAnimation(const AnimationEntry &sequence)
{
    startFrame = sequence.firstFrame;
    numFrames = sequence.length;
    fps = sequence.fps;
    cur_frame = 0;
}

void WolfCharacter::update(float dt)
{
    cur_anim_duration = (float)numFrames / fps;
    
    updatePose();
    
    cur_frame_time += dt;
    
    if (cur_frame_time >= cur_anim_duration)
    {
        cur_frame_time = 0;
    }
    
    cur_frame = (float)numFrames * (cur_frame_time / cur_anim_duration);
}

void WolfCharacter::updatePose()
{
    int currIndex = int(cur_frame);
    int nextIndex = (currIndex + 1) % numFrames;
    float factor = cur_frame - floor(cur_frame);
    
    entity.frame = startFrame + nextIndex;
    entity.torsoFrame = startFrame + nextIndex;
    entity.oldFrame = startFrame + currIndex;
    entity.oldTorsoFrame = startFrame + currIndex;
    entity.lerp = factor;
    entity.torsoLerp = factor;
}

void WolfCharacter::draw(const glm::mat4 &mvp)
{
    body.render(mvp, entity);

    Transform headTransform;
    body.lerpTag("tag_head", entity, 0, &headTransform);
    
    glm::mat4 model(1.0f);
    
    model[0][0] = headTransform.rotation[0][0];
    model[0][1] = headTransform.rotation[0][1];
    model[0][2] = headTransform.rotation[0][2];
    
    model[1][0] = headTransform.rotation[1][0];
    model[1][1] = headTransform.rotation[1][1];
    model[1][2] = headTransform.rotation[1][2];
    
    model[2][0] = headTransform.rotation[2][0];
    model[2][1] = headTransform.rotation[2][1];
    model[2][2] = headTransform.rotation[2][2];
    
    model[3][0] = headTransform.position.x;
    model[3][1] = headTransform.position.y;
    model[3][2] = headTransform.position.z;
    
    glm::mat4 headMatrix = mvp * model;
    head.render(headMatrix);
}
