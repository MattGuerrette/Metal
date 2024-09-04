
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024. Matt Guerrette
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#import "GraphicsMath.hpp"

XM_ALIGNED_STRUCT(16) CameraUniforms
{
    Matrix view;
    Matrix projection;
    Matrix viewProjection;
    Matrix invProjection;
    Matrix invView;
    Matrix invViewProjection;
};

/// @todo Improve this Camera class to use Quaternion rotation
class Camera
{
public:
    Camera(Vector3 position,
        Vector3    direction,
        Vector3    up,
        float      fov,
        float      aspectRatio,
        float      nearPlane,
        float      farPlane);

    [[nodiscard]] const CameraUniforms& uniforms() const;

    void setProjection(float fov, float aspect, float zNear, float zFar);

private:
    void updateBasisVectors(Vector3 direction);

    void updateUniforms();

    CameraUniforms m_uniforms {};
    Vector3        m_position;
    Vector3        m_direction;
    Vector3        m_up;
    float          m_fieldOfView;
    float          m_aspectRatio;
    float          m_nearPlane;
    float          m_farPlane;
};
