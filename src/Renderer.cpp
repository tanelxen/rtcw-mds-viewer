//
//  Renderer.cpp
//  hlmv
//
//  Created by Fedor Artemenkov on 11.11.24.
//

#include <thread>
#include <filesystem>

#include "Renderer.h"

#include "MDSModel.h"
#include "MD3Model.h"

#include "Camera.h"
#include "MainQueue.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "../deps/tinyfiledialogs.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <imgui.h>

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
}

void Renderer::setModel(const MDSModel& body, const MD3Model& head)
{
    m_pmodel = std::make_unique<RenderableModel>();
    m_pmodel->init(body, head);
}

void Renderer::update(float dt)
{
    MainQueue::instance().poll();
    
    if (m_pmodel) {
        m_pmodel->update(dt);
    }
}

void Renderer::draw(const Camera& camera)
{
    glm::mat4 quakeToGL = {
        {  0,  0, -1,  0 },
        { -1,  0,  0,  0 },
        {  0,  1,  0,  0 },
        {  0,  0,  0,  1 }
    };
    
    glm::mat4 mvp = camera.projection * camera.view * quakeToGL;
    
    if (m_pmodel) {
        m_pmodel->draw(mvp);
    }
}

MDSModel mds;
MD3Model md3;

void Renderer::imgui_draw()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open", "Ctrl+O"))
            {
                openFile([this](std::string filename) {

                    std::filesystem::path mds_path(filename);
                    std::filesystem::path head_path = mds_path.parent_path() / "head3.mdc";
                    
                    mds.loadFromFile(filename);
                    md3.loadFromFile(head_path.string());
                    
                    setModel(mds, md3);
                    
                }, "*.mds");
            }

            if (ImGui::MenuItem("Exit")) {
                exit(1);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    
    if (m_pmodel) {
        m_pmodel->imguiDraw();
    }
}

void Renderer::openFile(std::function<void (std::string)> callback, const char* filter)
{
    std::thread([callback, filter]() {
        
        const char* filename = tinyfd_openFileDialog(nullptr, nullptr, 1, &filter, nullptr, 0);
        
        if (filename != nullptr)
        {
            MainQueue::instance().async([callback, filename] () {
                callback(filename);
            });
        }
        
    }).detach();
}
