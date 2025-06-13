//
//  WolfAnim.cpp
//  wolfmv
//
//  Created by Fedor Artemenkov on 13.06.25.
//

#include "WolfAnim.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>

bool isCommentOrEmpty(const std::string& line)
{
    std::string trimmed = line;
    trimmed.erase(0, trimmed.find_first_not_of(" \t"));
    return trimmed.empty() || trimmed.rfind("//", 0) == 0;
}

std::vector<std::string> tokenize(const std::string& line)
{
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;
    while (iss >> token)
        tokens.push_back(token);
    return tokens;
}

std::vector<AnimationEntry> parseWolfAnimFile(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    
    std::vector<AnimationEntry> animations;
    std::string line;
    bool parsingAnims = false;
    
    while (std::getline(file, line))
    {
        if (isCommentOrEmpty(line))
            continue;
        
        if (!parsingAnims)
        {
            if (line.find("STARTANIMS") != std::string::npos)
            {
                parsingAnims = true;
            }
            continue;
        }
        
        auto tokens = tokenize(line);
        if (tokens.size() < 6)
            continue; // skip invalid lines
        
        AnimationEntry entry;
        entry.name = tokens[0];
        entry.firstFrame = std::stoi(tokens[1]);
        entry.length = std::stoi(tokens[2]);
        entry.looping = std::stoi(tokens[3]);
        entry.fps = std::stoi(tokens[4]);
        entry.moveSpeed = std::stof(tokens[5]);
        
        animations.push_back(entry);
    }
    
    return animations;
}
