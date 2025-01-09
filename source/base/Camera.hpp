

#pragma once

#include "GraphicsMath.hpp"

class Camera
{
public:
    Camera(Vector3 position,
        Vector3    direction,
        Vector3    up,
        float      fov,
        float      aspectRatio,
        float      nearPlane,
        float      farPlane,
        float      viewWidth,
        float      viewHeight);

    [[nodiscard]] float viewWidth() const;

    [[nodiscard]] float viewHeight() const;

    [[nodiscard]] Matrix viewProjection() const;

    [[nodiscard]] Vector3 direction() const;

    [[nodiscard]] Vector3 up() const;

    [[nodiscard]] Vector3 right() const;

    void moveForward(float dt);

    void moveBackward(float dt);

    void strafeLeft(float dt);

    void strafeRight(float dt);

    void rotate(float pitch, float yaw);

    void setPosition(const Vector3& position);

    void setRotation(const Vector3& rotation);

    void setProjection(float fov,
        float                aspectRatio,
        float                nearPlane,
        float                farPlane,
        float                viewWidth,
        float                viewHeight);

private:
    void updateUniforms();

    Quaternion m_orientation;
    Matrix     m_viewProjection;
    Matrix     m_projection;
    Matrix     m_view;
    Vector3    m_position;
    Vector3    m_direction;
    Vector3    m_rotation;
    float      m_fieldOfView;
    float      m_aspectRatio;
    float      m_nearPlane;
    float      m_farPlane;
    float      m_speed = 10.0f;
    float      m_viewWidth = 800.0f;
    float      m_viewHeight = 600.0f;
};
