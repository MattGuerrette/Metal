
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024. Matt Guerrette
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct alignas(16) CameraUniforms
{
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 viewProjection;
    glm::mat4 invProjection;
    glm::mat4 invView;
    glm::mat4 invViewProjection;
};

/// @todo Improve this Camera class to use Quaternion rotation
class Camera
{
public:
    Camera(glm::vec3 position,
        glm::vec3    direction,
        glm::vec3    up,
        float        fov,
        float        width,
        float        height,
        float        nearPlane,
        float        farPlane);

    [[nodiscard]] const CameraUniforms& uniforms() const;

    void setProjection(float fov, float width, float height, float zNear, float zFar);

private:
    void updateBasisVectors(glm::vec3 direction);

    void updateUniforms();

    CameraUniforms m_uniforms {};
    glm::vec3      m_position;
    glm::vec3      m_direction;
    glm::vec3      m_up;
    float          m_fieldOfView;
    float          m_width;
    float          m_height;
    float          m_nearPlane;
    float          m_farPlane;
};
