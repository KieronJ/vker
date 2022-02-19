#pragma once

#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace vker {

struct Camera {
    const glm::mat4 GetViewMatrix() const
    {
        return glm::lookAt(pos, pos + dir, up);
    }

    const glm::mat4 GetProjectionMatrix() const
    {
        return glm::perspective(glm::radians(fov), aspect, 0.1f, 10000.0f);
    }

    glm::vec3 pos, dir, up;
    float fov, aspect;
};


} // namespace