
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include "Camera.hpp"

Camera::Camera(Vector3 position,
	Vector3 direction,
	Vector3 up,
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

void Camera::MoveForward(float dt)
{
	Position += Direction * dt * Speed;
	UpdateUniforms();
}

void Camera::MoveBackward(float dt)
{
	Position -= Direction * dt * Speed;
	UpdateUniforms();
}

void Camera::StrafeLeft(float dt)
{
	Position += Right * dt * Speed;
	UpdateUniforms();
}

void Camera::StrafeRight(float dt)
{
	Position -= Right * dt * Speed;
	UpdateUniforms();
}

void Camera::RotateY(float dt)
{
	//Rotation.y += dt * Speed * 10.0f;
	Quaternion rotation = Quaternion::CreateFromYawPitchRoll(dt, 0, 0);
	Direction = Vector3::Transform(Direction, rotation);
	//Vector3 newDir = Vector3::Transform(Direction, rotation);
	UpdateBasisVectors(Direction);
	UpdateUniforms();
}

void Camera::UpdateBasisVectors(Vector3 direction)
{
	Direction = direction;
	Direction.Normalize();

	Vector3 up = Up;
	Right = up.Cross(Direction);
	Vector3 newUp = Right.Cross(Direction);
	Up = up;
}

void Camera::UpdateUniforms()
{
	Rotation = Quaternion::LookRotation(Direction, Vector3::Up);
	Uniforms.View = Matrix::CreateLookAt(Position, Position + Direction, Up);
	Uniforms.Projection = Matrix::CreatePerspectiveFieldOfView(FieldOfView,
		AspectRatio, NearPlane, FarPlane);
	Uniforms.ViewProjection = Uniforms.View * Uniforms.Projection;
}
