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
#include "WolfAnim.h"

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

std::vector<AnimationEntry> wolfanim;
int seqIndex = 0;

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
                    std::filesystem::path wolfanim_path = mds_path.parent_path() / "wolfanim.cfg";
                    
                    wolfanim = parseWolfAnimFile(wolfanim_path);
                    
                    mds.loadFromFile(filename);
                    md3.loadFromFile(head_path);
                    
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
    
    if (m_pmodel == nullptr) return;
    
    ImGui::SetNextWindowSizeConstraints(ImVec2(250, 250), ImVec2(FLT_MAX, FLT_MAX));
    
    if (ImGui::Begin("Model Info###model"))
    {
        ImGui::Text(("Name: " + m_pmodel->name).c_str());
        
        ImGuiStyle& style = ImGui::GetStyle();
        float w = ImGui::CalcItemWidth();
        float spacing = style.ItemInnerSpacing.x;
        float button_sz = ImGui::GetFrameHeight();
        
        ImGui::Text("Sequence");
        ImGui::SameLine(0, 10);
        
        ImGui::PushItemWidth(w - spacing * 2.0f - button_sz * 2.0f);
        
        if (ImGui::BeginCombo("##sequence combo", wolfanim[seqIndex].name.c_str(), ImGuiComboFlags_None))
        {
            for (int i = 0; i < wolfanim.size(); ++i)
            {
                bool is_selected = (seqIndex == i);
                
                if (ImGui::Selectable(wolfanim[i].name.c_str(), is_selected))
                {
                    seqIndex = i;
                    m_pmodel->setAnimation(wolfanim[i]);
                }
                
                if (is_selected) ImGui::SetItemDefaultFocus();
            }
            
            ImGui::EndCombo();
        }
        
        ImGui::PopItemWidth();
        
        ImGui::End();
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
