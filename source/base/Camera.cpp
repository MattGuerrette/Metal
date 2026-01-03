
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024. Matt Guerrette
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include "Camera.hpp"

Camera::Camera(Vector3 position,
    Vector3            direction,
    Vector3            up,
    float              fov,
    float              aspectRatio,
    float              nearPlane,
    float              farPlane)
    : m_position(position)
    , m_direction(direction)
    , m_up(up)
    , m_fieldOfView(fov)
    , m_aspectRatio(aspectRatio)
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

void Camera::setProjection(float fov, float aspect, float zNear, float zFar)
{
    m_fieldOfView = fov;
    m_aspectRatio = aspect;
    m_nearPlane = zNear;
    m_farPlane = zFar;
    updateUniforms();
}

void Camera::updateBasisVectors(Vector3 direction)
{
    m_direction = direction;
    m_direction.Normalize();
}

void Camera::updateUniforms()
{
    m_uniforms.view = Matrix::CreateLookAt(m_position, m_direction, m_up);
    m_uniforms.projection = Matrix::CreatePerspectiveFieldOfView(
        m_fieldOfView, m_aspectRatio, m_nearPlane, m_farPlane);
    m_uniforms.viewProjection = m_uniforms.projection * m_uniforms.view;
}
