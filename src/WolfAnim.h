//
//  WolfAnim.h
//  wolfmv
//
//  Created by Fedor Artemenkov on 13.06.25.
//

#pragma once

#include <string>
#include <vector>

struct AnimationEntry
{
    std::string name;
    int firstFrame = 0;
    int length = 0;
    int looping = 0;
    int fps = 0;
    float moveSpeed = 0.0f;
};

std::vector<AnimationEntry> parseWolfAnimFile(const std::string& filename);
