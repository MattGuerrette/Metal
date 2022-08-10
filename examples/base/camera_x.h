//
//  camera_x.h
//  base
//
//  Created by Matthew Guerrette on 7/25/22.
//

#pragma once

#include "graphics_math.h"
#include "platform.h"

/// @brief Camera uniform values
XM_ALIGNED_STRUCT(16) CameraUniforms
{
    DirectX::XMMATRIX View;
    DirectX::XMMATRIX Projection;
    DirectX::XMMATRIX ViewProjection;
    DirectX::XMMATRIX InvViewProjection;
    DirectX::XMMATRIX InvProjection;
    DirectX::XMMATRIX InvView;
};

class Camera
{
 public:
    /// @brief Default constructor
    Camera() = default;

    /// @brief Constructs a perspective projection camera with position
    /// @param [in] position The camera position
    /// @param [in] direction The camera direction
    /// @param [in] up The camera up direction, perpendicular to direction
    /// @param [in] fov The viewing angle (field of view)
    /// @param [in] aspectRation The horizontal over vertical aspect ratio
    /// @param [in] nearPlane The distance of near plane to position in world
    /// space
    /// @param [in] farPlane The distance of far plane to position in world space
    Camera(DirectX::XMFLOAT3 position,
           DirectX::XMFLOAT3 direction,
           DirectX::XMFLOAT3 up,
           float fov,
           float aspectRatio,
           float nearPlane,
           float farPlane);

    /// @brief Gets the uniforms
    /// @return camera uniforms
    const CameraUniforms& GetUniforms() const;

 private:
    /// @brief Updates basic vectors from new direction
    /// @param [in] newDirection The new direction vector
    /// Computes the new basic vectors for the camera, maintaining
    /// othogonality.
    void UpdateBasicVectors(DirectX::XMFLOAT3 newDirection);

    /// @brief Updates uniforms from basis vectors
    void UpdateUniforms();

    /// @brief Computes the view frustum
    /// @todo Needs implementation
    void ComputeViewFrustum();

    CameraUniforms    Uniforms;
    DirectX::XMFLOAT3 Direction;
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Up;
    float             ViewAngle;
    float             NearPlane;
    float             FarPlane;
    float             AspectRatio;
};
