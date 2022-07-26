//
//  camera_x.cpp
//  base
//
//  Created by Matthew Guerrette on 7/25/22.
//

#include "camera_x.h"

using namespace DirectX::SimpleMath;

Camera::Camera(Vector3 position,
               Vector3 direction,
               Vector3 up,
               float fov,
               float aspectRatio,
               float nearPlane,
               float farPlane)
{
    Up = up;
    UpdateBasicVectors(direction);
    
    Position = position;
    ViewAngle = fov;
    AspectRatio = aspectRatio;
    NearPlane = nearPlane;
    FarPlane = farPlane;
    
    // Update uniforms
    UpdateUniforms();
}

const CameraUniforms& Camera::GetUniforms() const
{
    return Uniforms;
}

void Camera::UpdateBasicVectors(Vector3 newDirection)
{
    Direction = newDirection;
    Direction.Normalize();
    
    Vector3 right = Direction.Cross(Up);
    Up = right.Cross(Direction);
}

void Camera::UpdateUniforms()
{
    Uniforms.View = Matrix::CreateLookAt(Position, Direction, Up);
    Uniforms.Projection = Matrix::CreatePerspectiveFieldOfView(ViewAngle, AspectRatio, NearPlane, FarPlane);

    
    Uniforms.ViewProjection =  Uniforms.Projection * Uniforms.View;
}
