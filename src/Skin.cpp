//
//  Skin.cpp
//  wolfmv
//
//  Created by Fedor Artemenkov on 13.06.25.
//

#include "Skin.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

#include <filesystem>

inline std::string trim(const std::string& str)
{
    const char* whitespace = " \t\r\n";
    const auto start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) return "";
    const auto end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

SkinFile parseSkinFile(const std::string& filepath)
{
    std::filesystem::path file_path(filepath);
    auto folder = file_path.parent_path();
    
    SkinFile result;
    std::ifstream file(filepath);
    
    if (!file) {
        std::cerr << "Cannot open file: " << filepath << std::endl;
        return result;
    }
    
    std::string line;
    
    while (std::getline(file, line))
    {
        line = trim(line);
        if (line.empty() || line.starts_with("//")) continue;
        
        size_t commaPos = line.find(',');
        if (commaPos == std::string::npos) continue;
        
        std::string key = trim(line.substr(0, commaPos));
        std::string value = trim(line.substr(commaPos + 1));
        
        // Удалить кавычки, если они есть
        if (!value.empty() && value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.size() - 2);
        }
        
        if (value.empty()) {
            result.tags.push_back(key);
        } else if (key.starts_with("md3_")) {
            result.attachments[key] = value;
        } else {
            std::filesystem::path texture_path(value);
            result.textures[key] = (folder / texture_path.filename()).string();
        }
    }
    
    return result;
}


void SkinFile::print()
{
    std::cout << "Tags:\n";
    for (const auto& tag : tags) {
        std::cout << "  " << tag << '\n';
    }
    
    std::cout << "\nSkins:\n";
    for (const auto& [mesh, texture] : textures) {
        std::cout << "  " << mesh << " -> " << texture << '\n';
    }
    
    std::cout << "\nAttachments:\n";
    for (const auto& [key, model] : attachments) {
        std::cout << "  " << key << " -> " << model << '\n';
    }
}
