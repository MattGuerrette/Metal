/*
*    Metal Examples
*    Copyright(c) 2019 Matt Guerrette
*
*    This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include "platform.h"
#import <simd/simd.h>

struct Transform {
    simd_float3 translation = simd_float3(0.0f);
    simd_float3 rotation = simd_float3(0.0f);
    simd_float3 scale = simd_float3(0.0f);
};

class Camera {
public:
    Camera();

    ~Camera();

    void setTranslation(float x, float y, float z);

    void setRotation(float x, float y, float z);

    void setPerspective(float aspect, float fov, float znear, float zfar);

    const simd_float4x4& getProjection() const;
    const simd_float4x4& getView() const;

private:
    simd_float4x4 projection_ = matrix_identity_float4x4;
    simd_float4x4 view_       = matrix_identity_float4x4;
    Transform     transform_;

    void updateView();
};
