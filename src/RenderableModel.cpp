//
//  RenderableModel.cpp
//  hlmv
//
//  Created by Fedor Artemenkov on 17.11.24.
//

#include "RenderableModel.h"
#include "Shader.h"
#include <glad/glad.h>

#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/gtx/euler_angles.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "../deps/stb_image.h"

#include <imgui.h>

#include <filesystem>

Shader shader;

RenderableModel::~RenderableModel()
{
//    printf("Delete %s", name.c_str());
//    
//    glDeleteTextures(textures.size(), textures.data());
//    glDeleteVertexArrays(1, &vao);
//    glDeleteBuffers(1, &vbo);
//    glDeleteBuffers(1, &ibo);
}

#define VERT_POSITION_LOC 0
#define VERT_NORMAL_LOC 1
#define VERT_TEX_COORD_LOC 2
#define VERT_BONE_INDEX_LOC 3

std::string resolvePath(const std::string& filename)
{
    namespace fs = std::filesystem;
    
    fs::path originalPath(filename);
    
    // Если файл существует с указанным именем — возвращаем его
    if (fs::exists(originalPath))
        return filename;
    
    // Список допустимых расширений
    static const std::vector<std::string> alternativeExtensions = {
        ".tga", ".jpg"
    };
    
    // Путь без расширения
    fs::path basePath = originalPath;
    basePath.replace_extension(); // удаляет текущее расширение
    
    for (const auto& ext : alternativeExtensions)
    {
        fs::path testPath = basePath;
        testPath.replace_extension(ext);
        
        if (fs::exists(testPath))
            return testPath.string();
    }
    
    // Ничего не найдено — возвращаем пустую строку или исходное имя
    return "";
}

GLuint loadTexture(std::string filename)
{
    filename = resolvePath(filename);
    
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

Entity entity;

const MDSModel * pbody;
const MD3Model * phead;

int startFrame = 0;
int numFrames = 1;
int fps = 15;

auto g_textures = std::unordered_map<std::string, GLuint>();

void RenderableModel::init(const MDSModel& mds, const MD3Model& md3, const SkinFile &bodySkin, const SkinFile &headSkin)
{
    shader.init("assets/shaders/md3.glsl");
    
    for (const auto& [mesh, texture] : bodySkin.textures)
    {
        g_textures[mesh] = loadTexture(texture.c_str());
    }
    
    for (const auto& [mesh, texture] : headSkin.textures)
    {
        g_textures[mesh] = loadTexture(texture.c_str());
    }
    
    this->name = mds.name_;
    
    pbody = &mds;
    
    drawCallList.resize(mds.numSurfaces());
    
    for (int i = 0; i < drawCallList.size(); ++i)
    {
        auto& drawCall = drawCallList[i];
        
        int numVertices = mds.surfaceNumVertices(i);
        int numIndices = mds.surfaceNumTriangles(i) * 3;
        
        drawCall.numVertices = numVertices;
        drawCall.numIndices = numIndices;
        
        glGenBuffers(1, &drawCall.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, drawCall.vbo);
        glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(Vertex), nullptr, GL_STREAM_DRAW);
        drawCall.verticesPtr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        

        glGenVertexArrays(1, &drawCall.vao);
        glBindVertexArray(drawCall.vao);
        
        glEnableVertexAttribArray(VERT_POSITION_LOC);
        glVertexAttribPointer(VERT_POSITION_LOC, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
        
        glEnableVertexAttribArray(VERT_NORMAL_LOC);
        glVertexAttribPointer(VERT_NORMAL_LOC, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        
        glEnableVertexAttribArray(VERT_TEX_COORD_LOC);
        glVertexAttribPointer(VERT_TEX_COORD_LOC, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
        
        
        glGenBuffers(1, &drawCall.ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawCall.ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * numIndices, nullptr, GL_STREAM_DRAW);
        drawCall.indicesPtr = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
    }
    
    phead = &md3;
    
    drawCallList2.resize(md3.numSurfaces());
    
    for (int i = 0; i < drawCallList2.size(); ++i)
    {
        auto& drawCall = drawCallList2[i];
        
        int numVertices = md3.surfaceNumVertices(i);
        int numIndices = md3.surfaceNumIndices(i);
        
        drawCall.numVertices = numVertices;
        drawCall.numIndices = numIndices;
        
        glGenBuffers(1, &drawCall.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, drawCall.vbo);
        glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(Vertex), nullptr, GL_STREAM_DRAW);
        drawCall.verticesPtr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        
        
        glGenVertexArrays(1, &drawCall.vao);
        glBindVertexArray(drawCall.vao);
        
        glEnableVertexAttribArray(VERT_POSITION_LOC);
        glVertexAttribPointer(VERT_POSITION_LOC, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
        
        glEnableVertexAttribArray(VERT_NORMAL_LOC);
        glVertexAttribPointer(VERT_NORMAL_LOC, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        
        glEnableVertexAttribArray(VERT_TEX_COORD_LOC);
        glVertexAttribPointer(VERT_TEX_COORD_LOC, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
        
        
        glGenBuffers(1, &drawCall.ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawCall.ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * numIndices, nullptr, GL_STREAM_DRAW);
        drawCall.indicesPtr = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
    }
}

void RenderableModel::updatePose()
{
    int currIndex = int(cur_frame);
    int nextIndex = (currIndex + 1) % numFrames;
    float factor = cur_frame - floor(cur_frame);
    
    entity.frame = startFrame + nextIndex;
    entity.torsoFrame = startFrame + nextIndex;
    entity.oldFrame = startFrame + currIndex;
    entity.oldTorsoFrame = startFrame + currIndex;
    entity.lerp = factor;
    entity.torsoLerp = factor;
}

void RenderableModel::update(float dt)
{
    cur_anim_duration = (float)numFrames / fps;
    
    updatePose();
    
    cur_frame_time += dt;
    
    if (cur_frame_time >= cur_anim_duration)
    {
        cur_frame_time = 0;
    }
    
    cur_frame = (float)numFrames * (cur_frame_time / cur_anim_duration);
}

void RenderableModel::draw(glm::mat4 &mvp)
{
    pbody->render(drawCallList, &entity);
    
    shader.bind();
    shader.setUniform("uMVP", mvp);
    
    for (int i = 0; i < drawCallList.size(); ++i)
    {
        auto& drawCall = drawCallList[i];
        
        if (g_textures.contains(drawCall.name))
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, g_textures[drawCall.name]);
        }
        
        glBindVertexArray(drawCall.vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawCall.ibo);
        
        glDrawElements(GL_TRIANGLES, drawCall.numIndices, GL_UNSIGNED_SHORT, 0);
    }
    
    Transform headTransform;
    pbody->lerpTag("tag_head", entity, 0, &headTransform);
    
    glm::mat4 model(1.0f);
    
    model[0][0] = headTransform.rotation[0][0];
    model[0][1] = headTransform.rotation[0][1];
    model[0][2] = headTransform.rotation[0][2];
    
    model[1][0] = headTransform.rotation[1][0];
    model[1][1] = headTransform.rotation[1][1];
    model[1][2] = headTransform.rotation[1][2];
    
    model[2][0] = headTransform.rotation[2][0];
    model[2][1] = headTransform.rotation[2][1];
    model[2][2] = headTransform.rotation[2][2];
    
    model[3][0] = headTransform.position.x;
    model[3][1] = headTransform.position.y;
    model[3][2] = headTransform.position.z;
    
    shader.setUniform("uMVP", mvp * model);
    
    phead->render(drawCallList2, &entity);
    
    for (int i = 0; i < drawCallList2.size(); ++i)
    {
        auto& drawCall = drawCallList2[i];
        
        if (drawCall.name == "h_blink") continue;
        
        if (g_textures.contains(drawCall.name))
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, g_textures[drawCall.name]);
        }
        
        glBindVertexArray(drawCall.vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawCall.ibo);
        
        glDrawElements(GL_TRIANGLES, drawCall.numIndices, GL_UNSIGNED_SHORT, 0);
    }
}

void RenderableModel::imguiDraw()
{
    ImGui::Text("entity.oldFrame = %i", entity.oldFrame);
    ImGui::Text("entity.frame = %i", entity.frame);
    
    float f = cur_frame_time / cur_anim_duration;
    ImGui::SliderFloat("Frame", &f, 0.0f, 1.0f);
}

void RenderableModel::setAnimation(const AnimationEntry &entry)
{
    startFrame = entry.firstFrame;
    numFrames = entry.length;
    fps = entry.fps;
    
    cur_frame = 0;
}

