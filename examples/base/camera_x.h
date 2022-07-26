//
//  camera_x.h
//  base
//
//  Created by Matthew Guerrette on 7/25/22.
//

#pragma once

#include "platform.h"
#include "graphics_math.h"


struct CameraUniforms
{
    DirectX::SimpleMath::Matrix View;
    DirectX::SimpleMath::Matrix Projection;
    DirectX::SimpleMath::Matrix ViewProjection;
    DirectX::SimpleMath::Matrix InvViewProjection;
    DirectX::SimpleMath::Matrix InvProjection;
    DirectX::SimpleMath::Matrix InvView;
};

class Camera {
public:
    Camera() = default;
    
    /// @brief Constructs a perspective projection camera with position
    /// @param [in] position The camera position
    /// @param [in] direction The camera direction
    /// @param [in] up The camera up direction, perpendicular to direction
    /// @param [in] fov The viewing angle (field of view)
    /// @param [in] aspectRation The horizontal over vertical aspect ratio
    /// @param [in] nearPlane The distance of near plane to position in world space
    /// @param [in] farPlane The distance of far plane to position in world space
    Camera(const DirectX::SimpleMath::Vector3 position,
           const DirectX::SimpleMath::Vector3 direction,
           const DirectX::SimpleMath::Vector3 up,
           float fov,
           float aspectRatio,
           float nearPlane,
           float farPlane);
    
    
    const CameraUniforms& GetUniforms() const;
    
private:
    void UpdateBasicVectors(DirectX::SimpleMath::Vector3 newDirection);
    
    void UpdateUniforms();
    
    CameraUniforms Uniforms;
    DirectX::SimpleMath::Vector3 Direction;
    DirectX::SimpleMath::Vector3 Position;
    DirectX::SimpleMath::Vector3 Up;
    float ViewAngle;
    float NearPlane;
    float FarPlane;
    float AspectRatio;
};
