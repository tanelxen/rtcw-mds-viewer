//
//  Renderer.hpp
//  hlmv
//
//  Created by Fedor Artemenkov on 11.11.24.
//

#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>

struct WolfCharacter;
struct GLFWwindow;
class Camera;

struct Renderer
{
    Renderer();
    ~Renderer();
    
    void update(float dt);
    void draw(const Camera& camera);
    void imgui_draw();
    
private:
    void LoadSkinPair(const std::string& folder, const std::string& skinName);
    std::unique_ptr<WolfCharacter> m_pmodel;
};
