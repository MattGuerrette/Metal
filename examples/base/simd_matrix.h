/*
*    Metal Examples
*    Copyright(c) 2019 Matt Guerrette
*
*    This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#import <simd/simd.h>

inline simd_float4x4 simd_float4x4_perspective(float aspect, float fovy, float znear, float zfar)
{
    float yScale = 1 / tan(fovy * 0.5);
    float xScale = yScale / aspect;
    float zRange = zfar - znear;
    float zScale = -(zfar + znear) / zRange;
    float wzScale = -2 * zfar * znear / zRange;

    simd_float4 P = { xScale, 0, 0, 0 };
    simd_float4 Q = { 0, yScale, 0, 0 };
    simd_float4 R = { 0, 0, zScale, -1 };
    simd_float4 S = { 0, 0, wzScale, 0 };

    simd_float4x4 mat = { P, Q, R, S };
    return mat;
}

inline simd_float4x4 simd_float4x4_rotation(simd_float3 axis, float angle)
{
    float c = cos(angle);
    float s = sin(angle);

    simd_float4 X;
    X.x = axis.x * axis.x + (1 - axis.x * axis.x) * c;
    X.y = axis.x * axis.y * (1 - c) - axis.z * s;
    X.z = axis.x * axis.z * (1 - c) + axis.y * s;
    X.w = 0.0;

    simd_float4 Y;
    Y.x = axis.x * axis.y * (1 - c) + axis.z * s;
    Y.y = axis.y * axis.y + (1 - axis.y * axis.y) * c;
    Y.z = axis.y * axis.z * (1 - c) - axis.x * s;
    Y.w = 0.0;

    simd_float4 Z;
    Z.x = axis.x * axis.z * (1 - c) - axis.y * s;
    Z.y = axis.y * axis.z * (1 - c) + axis.x * s;
    Z.z = axis.z * axis.z + (1 - axis.z * axis.z) * c;
    Z.w = 0.0;

    simd_float4 W;
    W.x = 0.0;
    W.y = 0.0;
    W.z = 0.0;
    W.w = 1.0;

    simd_float4x4 mat = { X, Y, Z, W };
    return mat;
}

inline simd_float4x4 simd_float4x4_translation(simd_float3 t)
{
    simd_float4 X = { 1, 0, 0, 0 };
    simd_float4 Y = { 0, 1, 0, 0 };
    simd_float4 Z = { 0, 0, 1, 0 };
    simd_float4 W = { t.x, t.y, t.z, 1 };

    simd_float4x4 mat = { X, Y, Z, W };
    return mat;
}

inline simd_float4x4 simd_float4x4_rotate(simd_float4x4 m, simd_float3 axis, float angle)
{
    simd_float4x4 rot = simd_float4x4_rotation(axis, angle);

    return simd_mul(m, rot);
}
