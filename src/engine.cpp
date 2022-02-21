#include "engine.h"

#include <chrono>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <fmt/core.h>

#include <tiny_obj_loader.h>

#include "camera.h"
#include "model.h"
#include "utils.h"
#include "vertex.h"

namespace vker {

constexpr int Width = 1920;
constexpr int Height = 1080;

constexpr float CameraSpeed = 5.0f;
constexpr float LookSpeed = 0.25f;

Engine::Engine() : m_window{ Width, Height, "vker"}, m_renderer{m_window} {}

void Engine::Setup()
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warnings, errors;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warnings, &errors, "../../../asset/model/viking_room.obj")) {
        FatalError("failed to load model: {}\n", errors);
    }

    if (!warnings.empty()) fmt::print("object warnings: {}\n", warnings);

    Model& model = m_renderer.CreateModel();

    for (const auto& shape : shapes) {
        for (const auto& ind : shape.mesh.indices) {
            Vertex vertex{};

            vertex.pos.x = attrib.vertices[ind.vertex_index * 3];
            vertex.pos.y = attrib.vertices[ind.vertex_index * 3 + 1];
            vertex.pos.z = attrib.vertices[ind.vertex_index * 3 + 2];

            vertex.tex.x = attrib.texcoords[ind.texcoord_index * 2];
            vertex.tex.y = 1.0f - attrib.texcoords[ind.texcoord_index * 2 + 1];

            model.indices.push_back(model.indices.size());
            model.vertices.push_back(vertex);
        }
    }

    model.BuildBuffers();
}

void Engine::Run()
{
    bool mouse_focus = false;

    auto last_second = std::chrono::high_resolution_clock::now();
    auto last_time = last_second;

    auto frames = 0;

    Camera cam;
    cam.pos = glm::vec3(-5.0f, -10.0f, 0.0f);
    cam.dir = glm::normalize(glm::vec3(0.0f) - cam.pos);
    cam.up = glm::vec3(0.0f, 0.0f, -1.0f);
    cam.aspect = Width / static_cast<float>(Height);
    cam.fov = 45.0f;

    float yaw = glm::degrees(atan2(cam.dir.y, cam.dir.x));
    float pitch = glm::degrees(asin(-cam.dir.z));

    glm::dvec2 mouse_pos{ Width / 2.0f, Height / 2.0f };

    m_window.SetResizeCallback([=](int w, int h) {
        m_renderer.InvalidateSwapchain();
    });

    while (!m_window.ShouldClose()) {
        const auto current_time = std::chrono::high_resolution_clock::now();
        const auto frame_duration = std::chrono::duration_cast<std::chrono::microseconds>(current_time - last_time).count();
        const auto second_duration = std::chrono::duration_cast<std::chrono::microseconds>(current_time - last_second).count();

        const float fps = (1000000.0f * float(frames)) / float(second_duration);
        const float delta = float(frame_duration) / 1000000.0f;

        //fmt::print("{:.02f} fps ({:.02f} ms / frame)\n", fps, delta * 1000.0f);

        if (second_duration >= 1000000) {
            last_second = current_time;
            frames = 0;
        }

        last_time = current_time;

        const glm::vec3 left = glm::normalize(glm::cross(cam.dir - cam.pos, cam.up));

        if (m_window.GetKeyState(GLFW_KEY_W) == GLFW_PRESS) {
            cam.pos += CameraSpeed * delta * cam.dir;
        }
       
        if (m_window.GetKeyState(GLFW_KEY_A) == GLFW_PRESS) {
            cam.pos -= CameraSpeed * delta * left;
        }
        
        if (m_window.GetKeyState(GLFW_KEY_S) == GLFW_PRESS) {
            cam.pos -= CameraSpeed * delta * cam.dir;
        }
        
        if (m_window.GetKeyState(GLFW_KEY_D) == GLFW_PRESS) {
            cam.pos += CameraSpeed * delta * left;
        }
        
        if (m_window.GetKeyState(GLFW_KEY_I) == GLFW_PRESS) {
            fmt::print("camera pos. x={:.02f}, y={:.02f}, z={:.02f}\n", cam.pos.x, cam.pos.y, cam.pos.z);
            fmt::print("camera dir. x={:.02f}, y={:.02f}, z={:.02f}\n", cam.dir.x, cam.dir.y, cam.dir.z);
        }
        
        if (m_window.GetKeyState(GLFW_KEY_1) == GLFW_PRESS) {
            mouse_focus = false;
            m_window.SetInputMode(GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        
        if (m_window.GetMouseButton(GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
            mouse_focus = true;
            m_window.SetInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            m_window.GetCursor(mouse_pos.x, mouse_pos.y);
        }
        
        if (mouse_focus) {
            double x, y;
            m_window.GetCursor(x, y);
        
            const double dx = x - mouse_pos.x;
            const double dy = y - mouse_pos.y;
        
            mouse_pos.x = x;
            mouse_pos.y = y;
        
            yaw += LookSpeed * dx;
            pitch -= LookSpeed * dy;
        
            yaw = std::fmod(yaw, 360.0f);
            pitch = std::clamp(pitch, -89.0f, 89.0f);
        
            cam.dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            cam.dir.z = sin(glm::radians(pitch));
            cam.dir.y = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        
            cam.dir = glm::normalize(cam.dir);
        }

        m_renderer.Draw(cam);
        m_renderer.Present();
        m_window.Update();

        frames++;
    }
}

} // namespace vker