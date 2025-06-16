//
//  Utils.cpp
//  wolfmv
//
//  Created by Fedor Artemenkov on 16.06.25.
//

#include "Utils.h"

#include <filesystem>
#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../deps/stb_image.h"

std::string resolvePath(const std::string& filename, const std::vector<std::string>& extensions)
{
    namespace fs = std::filesystem;
    
    fs::path originalPath(filename);
    
    if (originalPath.has_extension() && fs::exists(originalPath)) return filename;
    
    fs::path basePath = originalPath;
    basePath.replace_extension();
    
    for (const auto& ext : extensions)
    {
        fs::path testPath = basePath;
        testPath.replace_extension(ext);
        
        if (fs::exists(testPath)) return testPath.string();
    }
    
    return "";
}

GLuint loadTexture(std::string filename)
{
    filename = resolvePath(filename, {".tga", ".jpg"});
    
    if (filename.empty()) return 0;
    
    GLuint id;
    glGenTextures(1, &id);
    
    int width, height;
    int num_channels = 3;
    unsigned char* image = stbi_load(filename.c_str(), &width, &height, &num_channels, 3);
    
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
