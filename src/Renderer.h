//
//  Renderer.hpp
//  hlmv
//
//  Created by Fedor Artemenkov on 11.11.24.
//

#pragma once

#include <vector>
#include <functional>

#include <glm/glm.hpp>

#include "RenderableModel.h"

struct GLFWwindow;
class MDSModel;
class SkinFile;
class Camera;

struct Renderer
{
    Renderer();
    ~Renderer();
    
    void setModel(const MDSModel& body, const MD3Model& head, const SkinFile &bodySkin, const SkinFile &headSkin);
    void update(float dt);
    void draw(const Camera& camera);
    void imgui_draw();
    
private:
    std::unique_ptr<RenderableModel> m_pmodel;
    
    //ImGui stuff
    std::vector<std::string> sequenceNames;
    
    bool isPlayerView = false;
    glm::vec3 weaponOffset = {0, 0, 0};
    
    void openFile(std::function<void(std::string)> callback, const char* filter);
};
