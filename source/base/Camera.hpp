
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#import "GraphicsMath.hpp"

XM_ALIGNED_STRUCT(16) CameraUniforms
{
	Matrix View;
	Matrix Projection;
	Matrix ViewProjection;
	Matrix InvProjection;
	Matrix InvView;
	Matrix InvViewProjection;
};

/// @todo Improve this Camera class to use Quaternion rotation
class Camera
{
public:
	Camera(Vector3 position,
		Vector3 direction,
		Vector3 up,
		float fov,
		float aspectRatio,
		float nearPlane,
		float farPlane);

	[[nodiscard]] const CameraUniforms& GetUniforms() const;

private:
	void UpdateBasisVectors(Vector3 direction);

	void UpdateUniforms();

	CameraUniforms Uniforms{};
	Vector3 Position;
	Vector3 Direction;
	Vector3 Up;
	float FieldOfView;
	float AspectRatio;
	float NearPlane;
	float FarPlane;
};

