//
//  Utils.h
//  wolfmv
//
//  Created by Fedor Artemenkov on 16.06.25.
//

#pragma once

#include <string>
#include <vector>

std::string resolvePath(const std::string& filename, const std::vector<std::string>& extensions);
unsigned int loadTexture(std::string filename);
