//
//  Renderer.cpp
//  hlmv
//
//  Created by Fedor Artemenkov on 11.11.24.
//

#include <thread>
#include <filesystem>

#include "Renderer.h"

#include "WolfCharacter.h"
#include "WolfAnim.h"
#include "Skin.h"

#include "Camera.h"
#include "MainQueue.h"
#include "Utils.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "../deps/tinyfiledialogs.h"

#include <imgui.h>

#include <filesystem>
#include <unordered_set>

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
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

std::vector<AnimationEntry> wolfanim;
int seqIndex = 0;

namespace fs = std::filesystem;

std::string selectedFolder;
std::vector<std::string> skinNames;
std::string currentSelectedSkin;
bool showMissingMdsMessage = false;

void ScanSkinFolder(const std::string& folderPath)
{
    skinNames.clear();
    
    bool folderHasBodyMds = fs::exists(folderPath + "/body.mds");
    showMissingMdsMessage = !folderHasBodyMds;
    
    if (!folderHasBodyMds) return;
    
    std::unordered_set<std::string> bodySkins;
    std::unordered_set<std::string> headSkins;
    
    for (const auto& entry : fs::directory_iterator(folderPath)) {
        if (!entry.is_regular_file()) continue;
        std::string filename = entry.path().filename().string();
        
        if (filename.starts_with("body_") && filename.ends_with(".skin")) {
            std::string skin = filename.substr(5, filename.size() - 10); // 5 = strlen("body_"), 10 = strlen("body_") + strlen(".skin")
            bodySkins.insert(skin);
        } else if (filename.starts_with("head_") && filename.ends_with(".skin")) {
            std::string skin = filename.substr(5, filename.size() - 10);
            headSkins.insert(skin);
        }
    }
    
    // Найти общие имена скинов
    for (const auto& skin : bodySkins) {
        if (headSkins.count(skin)) {
            skinNames.push_back(skin);
        }
    }
}

void Renderer::LoadSkinPair(const std::string& folder, const std::string& skinName)
{
    std::string animPath = folder + "/wolfanim.cfg";
    wolfanim = parseWolfAnimFile(animPath);
    
    m_pmodel = std::make_unique<WolfCharacter>();
    m_pmodel->m_name = (fs::path(folder).parent_path().filename() / skinName).string();
    m_pmodel->init(folder, skinName);
}

void selectFolder(std::function<void (std::string)> callback);

void Renderer::imgui_draw()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open", "Ctrl+O"))
            {
                selectFolder([this](std::string path) {
                    selectedFolder = path;
                    ScanSkinFolder(path);
                });
            }

            if (ImGui::MenuItem("Exit")) {
                exit(1);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    
    if (showMissingMdsMessage)
    {
        ImGui::OpenPopup("###alert");
        showMissingMdsMessage = false;
    }
    
    if (ImGui::BeginPopupModal("ERROR###alert", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Missing 'body.mds' in selected folder!");
        ImGui::Spacing();
        
        if (ImGui::Button("OK", ImVec2(60, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    
    if (!selectedFolder.empty())
    {
        if (ImGui::Begin("Available skins###skins"))
        {
            for (const auto& skin : skinNames)
            {
                if (ImGui::Selectable(skin.c_str(), currentSelectedSkin == skin))
                {
                    currentSelectedSkin = skin;
                    LoadSkinPair(selectedFolder, skin);
                }
            }
            
            ImGui::End();
        }
    }
    
    if (m_pmodel == nullptr) return;
    
    ImGui::SetNextWindowSizeConstraints(ImVec2(250, 250), ImVec2(FLT_MAX, FLT_MAX));
    
    if (ImGui::Begin("Model Info###model"))
    {
        ImGui::Text(("Name: " + m_pmodel->m_name).c_str());
        
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

void selectFolder(std::function<void (std::string)> callback)
{
    std::thread([callback]() {
        
        const char* path = tinyfd_selectFolderDialog(nullptr, nullptr);
        
        if (path != nullptr)
        {
            MainQueue::instance().async([callback, path] () {
                callback(path);
            });
        }
        
    }).detach();
}
