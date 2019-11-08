/*
*    Metal Examples
*    Copyright(c) 2019 Matt Guerrette
*
*    This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "camera.h"
#include "simd_matrix.h"

simd_float4x4 simd_float4x4_uniform_scale(float scale)
{
    vector_float4 X = { scale, 0, 0, 0 };
    vector_float4 Y = { 0, scale, 0, 0 };
    vector_float4 Z = { 0, 0, scale, 0 };
    vector_float4 W = { 0, 0, 0, 1 };

    simd_float4x4 mat = { X, Y, Z, W };
    return mat;
}

Camera::Camera()
{
}

Camera::~Camera()
{
}

void Camera::updateView()
{
    //    glm::mat4 rotM = glm::mat4(1.0f);
    //    glm::mat4 transM;
    //
    //    rotM = glm::rotate(rotM, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    //    rotM = glm::rotate(rotM, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    //    rotM = glm::rotate(rotM, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    //
    //    transM = glm::translate(glm::mat4(1.0f), position);
    //
    //    if (type == CameraType::firstperson)
    //    {
    //        matrices.view = rotM * transM;
    //    }
    //    else
    //    {
    //        matrices.view = transM * rotM;
    //    }

    simd_float4x4 rot = matrix_identity_float4x4;

    simd_float3 xAxis = simd_make_float3(1.0f, 0.0f, 0.0f);
    simd_float3 yAxis = simd_make_float3(0.0f, 1.0f, 0.0f);
    simd_float3 zAxis = simd_make_float3(0.0f, 0.0f, 1.0f);
    rot               = simd_float4x4_rotate(rot, xAxis, transform_.rotation.x);
    rot               = simd_float4x4_rotate(rot, yAxis, transform_.rotation.y);
    rot               = simd_float4x4_rotate(rot, zAxis, transform_.rotation.z);

    simd_float4x4 trans = simd_float4x4_translation(transform_.translation);

    view_ = simd_mul(trans, rot);
}

void Camera::setTranslation(float x, float y, float z)
{
    transform_.translation = { x, y, z };
    updateView();
}

void Camera::setRotation(float x, float y, float z)
{
    transform_.rotation = { x, y, z };
    updateView();
}

void Camera::setPerspective(float aspect, float fov, float znear, float zfar)
{
    projection_ = simd_float4x4_perspective(aspect, fov, znear, zfar);
}

const simd_float4x4& Camera::getProjection() const
{
    return projection_;
}

const simd_float4x4& Camera::getView() const
{
    return view_;
}
