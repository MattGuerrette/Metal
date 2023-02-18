
#include "Camera.hpp"

using namespace DirectX;

Camera::Camera(XMFLOAT3 position,
               XMFLOAT3 direction,
               XMFLOAT3 up,
               float fov,
               float aspectRatio,
               float nearPlane,
               float farPlane)
    : Position(position), Direction(direction), Up(up), FieldOfView(fov),
    AspectRatio(aspectRatio), NearPlane(nearPlane), FarPlane(farPlane)
{
    UpdateBasisVectors(Direction);
    UpdateUniforms();
}

const CameraUniforms& Camera::GetUniforms() const
{
    return Uniforms;
}

void Camera::UpdateBasisVectors(DirectX::XMFLOAT3 direction)
{
    Direction = direction;
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
    Uniforms.view = XMMatrixLookAtRH(pos, dir, up);
    Uniforms.projection = XMMatrixPerspectiveFovRH(FieldOfView,
        AspectRatio, NearPlane, FarPlane);
    Uniforms.viewProjection = Uniforms.projection * Uniforms.view;
}
