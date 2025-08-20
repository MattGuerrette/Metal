
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024. Matt Guerrette
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include "Camera.hpp"

Camera::Camera(glm::vec3 position,
    glm::vec3            direction,
    glm::vec3            up,
    float                fov,
    float                width,
    float                height,
    float                nearPlane,
    float                farPlane)
    : m_position(position)
    , m_direction(direction)
    , m_up(up)
    , m_fieldOfView(fov)
    , m_width(width)
    , m_height(height)
    , m_nearPlane(nearPlane)
    , m_farPlane(farPlane)
{
    updateBasisVectors(m_direction);
    updateUniforms();
}

const CameraUniforms& Camera::uniforms() const
{
    return m_uniforms;
}

void Camera::setProjection(float fov, float width, float height, float zNear, float zFar)
{
    m_fieldOfView = fov;
    m_width = width;
    m_height = height;
    m_nearPlane = zNear;
    m_farPlane = zFar;
    updateUniforms();
}

void Camera::updateBasisVectors(glm::vec3 direction)
{
    m_direction = glm::normalize(direction);

    const glm::vec3 up = m_up;
    const glm::vec3 right = glm::cross(up, m_direction);
    const glm::vec3 newUp = glm::cross(right, m_direction);
    m_up = up;
}

void Camera::updateUniforms()
{
    m_uniforms.view = glm::lookAt(m_position, m_direction, m_up);
    m_uniforms.projection
        = glm::perspectiveFovRH(m_fieldOfView, m_width, m_height, m_nearPlane, m_farPlane);
    m_uniforms.viewProjection = m_uniforms.projection * m_uniforms.view;
}
