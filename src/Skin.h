//
//  Skin.hpp
//  wolfmv
//
//  Created by Fedor Artemenkov on 13.06.25.
//

#pragma once

#include <string>
#include <vector>
#include <unordered_map>

struct SkinFile
{
    std::vector<std::string> tags;
    std::unordered_map<std::string, std::string> textures;
    std::unordered_map<std::string, std::string> attachments;
    
    void print();
};

SkinFile parseSkinFile(const std::string& filepath);
