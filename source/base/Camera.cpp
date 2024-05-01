
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024. Matt Guerrette
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include "Camera.hpp"

Camera::Camera(Vector3 position,
               Vector3 direction,
               Vector3 up,
               float   fov,
               float   aspectRatio,
               float   nearPlane,
               float   farPlane)
    : Position(position), Direction(direction), Up(up), FieldOfView(fov), AspectRatio(aspectRatio),
      NearPlane(nearPlane), FarPlane(farPlane)
{
    UpdateBasisVectors(Direction);
    UpdateUniforms();
}

const CameraUniforms& Camera::GetUniforms() const
{
    return Uniforms;
}

void Camera::UpdateBasisVectors(Vector3 direction)
{
    Direction = direction;
    Direction.Normalize();

    Vector3 up = Up;
    Vector3 right = up.Cross(Direction);
    Vector3 newUp = right.Cross(Direction);
    Up = up;
}

void Camera::UpdateUniforms()
{
    Uniforms.View = Matrix::CreateLookAt(Position, Direction, Up);
    Uniforms.Projection =
        Matrix::CreatePerspectiveFieldOfView(FieldOfView, AspectRatio, NearPlane, FarPlane);
    Uniforms.ViewProjection = Uniforms.Projection * Uniforms.View;
}
