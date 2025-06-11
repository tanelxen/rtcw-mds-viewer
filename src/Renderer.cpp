//
//  Renderer.cpp
//  hlmv
//
//  Created by Fedor Artemenkov on 11.11.24.
//

#include <thread>

#include "Renderer.h"
#include "MDSModel.h"
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
    uploadShader();
}

Renderer::~Renderer()
{
    glDeleteProgram(program);
}

void Renderer::setModel(const Model& model)
{
    m_pmodel = std::make_unique<RenderableModel>();
    m_pmodel->init(model);
}

void Renderer::update(float dt)
{
    if (m_pmodel) {
        m_pmodel->update(dt);
    }
    
    MainQueue::instance().poll();
}

void Renderer::draw(const Camera& camera)
{
    glUseProgram(program);
    
    glm::mat4 quakeToGL = {
        {  0,  0, -1,  0 },
        { -1,  0,  0,  0 },
        {  0,  1,  0,  0 },
        {  0,  0,  0,  1 }
    };
    
    glm::mat4 mvp = camera.projection * camera.view * quakeToGL;
    glUniformMatrix4fv(u_MVP_loc, 1, GL_FALSE, (const float*) &mvp);
    
    if (m_pmodel) {
//        glUniformMatrix4fv(u_boneTransforms_loc, (GLsizei)(m_pmodel->transforms.size()), GL_FALSE, &(m_pmodel->transforms[0][0][0]));
        m_pmodel->draw();
    }
}

unsigned int compile_shader(unsigned int type, const char* source);

void Renderer::uploadShader()
{
    const char* vert = R"(
        #version 410 core
        layout (location = 0) in vec4 position;
        layout (location = 1) in vec3 normal;
        layout (location = 2) in vec2 texCoord;
    
        uniform mat4 uMVP;
    
        out vec2 uv;

        void main()
        {
            gl_Position = uMVP * position;
            uv = texCoord;
        }
    )";
    
    const char* frag = R"(
        #version 410 core
    
        in vec2 uv;
    
        uniform sampler2D s_texture;
    
        //final color
        out vec4 FragColor;
    
        void main()
        {
            FragColor = texture(s_texture, uv);
        }
    )";
    
    program = glCreateProgram();

    unsigned int vs = compile_shader(GL_VERTEX_SHADER, vert);
    unsigned int fs = compile_shader(GL_FRAGMENT_SHADER, frag);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);
    
    glUniform1i(glGetUniformLocation(program, "s_texture"), 0);
    
    glUseProgram(program);
    
    u_MVP_loc = glGetUniformLocation(program, "uMVP");

    if (u_MVP_loc == -1)
    {
        printf("Shader have no uniform %s\n", "uMVP");
    }
    
    u_boneTransforms_loc = glGetUniformLocation(program, "uBoneTransforms");

    if (u_boneTransforms_loc == -1)
    {
        printf("Shader have no uniform %s\n", "uBoneTransforms");
    }
}

void Renderer::imgui_draw()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open", "Ctrl+O"))
            {
                openFile([this](std::string filename) {

                    Model mdl;
                    mdl.loadFromFile(filename);
                    
                    setModel(mdl);
                    
                }, "*.mds");
            }

            if (ImGui::MenuItem("Exit")) {
                exit(1);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
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


unsigned int compile_shader(unsigned int type, const char* source)
{
    unsigned int id = glCreateShader(type);

    glShaderSource(id, 1, &source, nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);

    if (result == GL_FALSE)
    {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);

        char message[1024];
        glGetShaderInfoLog(id, length, &length, message);

        printf("Failed to compile %s shader:\n", (type == GL_VERTEX_SHADER) ? "vertex" : "fragment");
        printf("%s\n", message);
        return 0;
    }

    return id;
}
