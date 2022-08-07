//
//  camera_x.cpp
//  base
//
//  Created by Matthew Guerrette on 7/25/22.
//

#include "camera_x.h"

using namespace DirectX;

Camera::Camera(XMFLOAT3 position,
    XMFLOAT3 direction,
    XMFLOAT3 up,
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

void Camera::UpdateBasicVectors(XMFLOAT3 newDirection)
{
    Direction = newDirection;
    XMVECTOR v = XMLoadFloat3(&Direction);
    v = XMVector3Normalize(v);
    XMStoreFloat3(&Direction, v);

    XMVECTOR upVector = XMLoadFloat3(&Up);
    XMVECTOR right = XMVector3Cross(v, upVector);
    XMVECTOR newUp = XMVector3Cross(right, v);
    XMStoreFloat3(&Up, newUp);
}

void Camera::UpdateUniforms()
{
    XMVECTOR pos = XMLoadFloat3(&Position);
    XMVECTOR dir = XMLoadFloat3(&Direction);
    XMVECTOR up = XMLoadFloat3(&Up);
    Uniforms.View = XMMatrixLookAtRH(pos, dir, up);//Matrix::CreateLookAt(Position, Direction, Up);
    Uniforms.Projection = XMMatrixPerspectiveFovRH(ViewAngle,
        AspectRatio, NearPlane, FarPlane); //Matrix::CreatePerspectiveFieldOfView(ViewAngle, AspectRatio, NearPlane, FarPlane);

    Uniforms.ViewProjection = Uniforms.Projection * Uniforms.View;
}

void Camera::ComputeViewFrustrum()
{
    
}
