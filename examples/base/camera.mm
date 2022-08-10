/*
 *    Metal Examples
 *    Copyright(c) 2019 Matt Guerrette
 *
 *    This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#include "camera.h"

using namespace simd;

// Generate look-at matrix
// First generate the full matrix basis, then write out an inverse(!) transform matrix
static float4x4 sInvMatrixLookat(float3 inEye, float3 inTo, float3 inUp)
{
    float3 z = normalize(inTo - inEye);
    float3 x = normalize(cross(inUp, z));
    float3 y = cross(z, x);
    float3 t = (float3) { -dot(x, inEye), -dot(y, inEye), -dot(z, inEye) };
    return float4x4(float4 { x.x, y.x, z.x, 0 },
        float4 { x.y, y.y, z.y, 0 },
        float4 { x.z, y.z, z.z, 0 },
        float4 { t.x, t.y, t.z, 1 });
}

// Helper function to normalize a plane equation so the plane direction is normalized to 1.
// This will result in dot(x, plane.xyz)+plane.w giving the actual distance to the plane
static simd::float4 sPlaneNormalize(const simd::float4& inPlane)
{
    return inPlane / simd::length(inPlane.xyz);
}

@implementation AAPLCamera : NSObject

// Helper function that is called after up is updated; will adjust forward/direction to stay orthogonal
// when creating a more defined basis, do not set axis independently, but use rotate() or setBasis() functions to update all at once
- (void)orthogonalizeFromNewUp:(float3)newUp
{
    _up = normalize(newUp);
    float3 right = normalize(cross(_direction, _up));
    _direction = (cross(_up, right));
}

// Helper function that is called after forward is updated; will adjust up to stay orthogonal
// when creating a more defined basis, do not set axis independently, but use rotate() or setBasis() functions to update all at once
- (void)orthogonalizeFromNewForward:(float3)newForward
{
    _direction = normalize(newForward);
    float3 right = normalize(cross(_direction, _up));
    _up = cross(right, _direction);
}

- (instancetype)initPerspectiveWithPosition:(simd::float3)position
                                  direction:(simd::float3)direction
                                         up:(simd::float3)up
                                  viewAngle:(float)viewAngle
                                aspectRatio:(float)aspectRatio
                                  nearPlane:(float)nearPlane
                                   farPlane:(float)farPlane
{
    self = [super init];
    _up = up;
    [self orthogonalizeFromNewForward:direction];

    _position = position;
    _width = 0;
    _viewAngle = viewAngle;
    _aspectRatio = aspectRatio;
    _nearPlane = nearPlane;
    _farPlane = farPlane;

    _uniformsDirty = true;
    return self;
}

- (instancetype)initParallelWithPosition:(simd::float3)position
                               direction:(simd::float3)direction
                                      up:(simd::float3)up
                                   width:(float)width
                                  height:(float)height
                               nearPlane:(float)nearPlane
                                farPlane:(float)farPlane
{
    self = [super init];
    _up = up;
    [self orthogonalizeFromNewForward:direction];
    _position = position;
    _width = width;
    _viewAngle = 0;
    _aspectRatio = width / height;
    _nearPlane = nearPlane;
    _farPlane = farPlane;
    _uniformsDirty = true;
    return self;
}

- (bool)isPerspective
{
    return _viewAngle != 0.0f;
}

- (bool)isParallel
{
    return _viewAngle == 0.0f;
}

// Updates internal uniforms to reflect new direction, up and position properties of the object
- (void)updateUniforms
{
    // Generate the view matrix from a matrix lookat
    _uniforms.viewMatrix = sInvMatrixLookat(_position, _position + _direction, _up);

    // Generate projection matrix from viewing angle and plane distances
    if (_viewAngle != 0) {
        float va_tan = 1.0f / tanf(_viewAngle * 0.5);
        float ys = va_tan;
        float xs = ys / _aspectRatio;
        float zs = _farPlane / (_farPlane - _nearPlane);
        _uniforms.projectionMatrix = float4x4((float4) { xs, 0, 0, 0 },
            (float4) { 0, ys, 0, 0 },
            (float4) { 0, 0, zs, 1 },
            (float4) { 0, 0, -_nearPlane * zs, 0 });

    } else {
        // Generate projection matrix from width, height and plane distances
        float ys = 2.0f / _width;
        float xs = ys / _aspectRatio;
        float zs = 1.0f / (_farPlane - _nearPlane);
        _uniforms.projectionMatrix = float4x4((float4) { xs, 0, 0, 0 },
            (float4) { 0, ys, 0, 0 },
            (float4) { 0, 0, zs, 0 },
            (float4) { 0, 0, -_nearPlane * zs, 1 });
    }

    // Derived matrices
    _uniforms.viewProjectionMatrix = _uniforms.projectionMatrix * _uniforms.viewMatrix;
    _uniforms.invProjectionMatrix = simd_inverse(_uniforms.projectionMatrix);
    _uniforms.invOrientationProjectionMatrix = simd_inverse(_uniforms.projectionMatrix * sInvMatrixLookat((float3) { 0, 0, 0 }, _direction, _up));
    _uniforms.invViewProjectionMatrix = simd_inverse(_uniforms.viewProjectionMatrix);
    _uniforms.invViewMatrix = simd_inverse(_uniforms.viewMatrix);
    
    

    float4x4 transp_vpm = simd::transpose(_uniforms.viewProjectionMatrix);
    _uniforms.frustumPlanes[0] = sPlaneNormalize(transp_vpm.columns[3] + transp_vpm.columns[0]); // left plane eq
    _uniforms.frustumPlanes[1] = sPlaneNormalize(transp_vpm.columns[3] - transp_vpm.columns[0]); // right plane eq
    _uniforms.frustumPlanes[2] = sPlaneNormalize(transp_vpm.columns[3] + transp_vpm.columns[1]); // up plane eq
    _uniforms.frustumPlanes[3] = sPlaneNormalize(transp_vpm.columns[3] - transp_vpm.columns[1]); // down plane eq
    _uniforms.frustumPlanes[4] = sPlaneNormalize(transp_vpm.columns[3] + transp_vpm.columns[2]); // near plane eq
    _uniforms.frustumPlanes[5] = sPlaneNormalize(transp_vpm.columns[3] - transp_vpm.columns[2]); // far plane eq

    // Uniforms are updated and no longer dirty
    _uniformsDirty = false;
}

// All other directions are derived from the -up and _direction instance variables
- (float3)left
{
    return simd_cross(_direction, _up);
}
- (float3)right
{
    return -self.left;
}
- (float3)down
{
    return -self.up;
}
- (float3)forward
{
    return self.direction;
}
- (float3)backward
{
    return -self.direction;
}

- (float)nearPlane
{
    return _nearPlane;
}
- (float)farPlane
{
    return _farPlane;
}
- (float)aspectRatio
{
    return _aspectRatio;
}
- (float)viewAngle
{
    return _viewAngle;
}
- (float)width
{
    return _width;
}
- (float3)up
{
    return _up;
}
- (float3)position
{
    return _position;
}
- (float3)direction
{
    return _direction;
}

// For the uniforms getter, we first check the dirty flag and re-calculate the values if needed
- (AAPLCameraUniforms)uniforms
{
    if (_uniformsDirty)
        [self updateUniforms];
    return _uniforms;
}

// For all the setter functions, we update the instance variables and set the dirty flag
- (void)setNearPlane:(float)newNearPlane
{
    _nearPlane = newNearPlane;
    _uniformsDirty = true;
}
- (void)setFarPlane:(float)newFarPlane
{
    _farPlane = newFarPlane;
    _uniformsDirty = true;
}
- (void)setAspectRatio:(float)newAspectRatio
{
    _aspectRatio = newAspectRatio;
    _uniformsDirty = true;
}
- (void)setViewAngle:(float)newAngle
{
    assert(_width == 0);
    _viewAngle = newAngle;
    _uniformsDirty = true;
}
- (void)setWidth:(float)newWidth
{
    assert(_viewAngle == 0);
    _width = newWidth;
    _uniformsDirty = true;
}
- (void)setPosition:(float3)newPosition
{
    _position = newPosition;
    _uniformsDirty = true;
}
- (void)setUp:(float3)newUp
{
    [self orthogonalizeFromNewUp:newUp];
    _uniformsDirty = true;
}
- (void)setDirection:(float3)newDirection
{
    [self orthogonalizeFromNewForward:newDirection];
    _uniformsDirty = true;
}

- (float4x4)ViewMatrix
{
    return _uniforms.viewMatrix;
}
- (float4x4)ProjectionMatrix
{
    return _uniforms.projectionMatrix;
}
- (float4x4)ViewProjectionMatrix
{
    return _uniforms.viewProjectionMatrix;
}
- (float4x4)InvOrientationProjectionMatrix
{
    return _uniforms.invOrientationProjectionMatrix;
}
- (float4x4)InvViewProjectionMatrix
{
    return _uniforms.invViewProjectionMatrix;
}
- (float4x4)InvProjectionMatrix
{
    return _uniforms.invProjectionMatrix;
}
- (float4x4)InvViewMatrix
{
    return _uniforms.invViewMatrix;
}

// Rotates the camera on an axis; it rotates both direction and up to keep the orthogonal base intact
- (void)rotateOnAxis:(float3)inAxis radians:(float)inRadians
{
    // generate rotation matrix along inAxis
    float3 axis = normalize(inAxis);
    float ct = cosf(inRadians);
    float st = sinf(inRadians);
    float ci = 1 - ct;
    float x = axis.x;
    float y = axis.y;
    float z = axis.z;
    float3x3 mat((float3) { ct + x * x * ci, y * x * ci + z * st, z * x * ci - y * st },
        (float3) { x * y * ci - z * st, ct + y * y * ci, z * y * ci + x * st },
        (float3) { x * z * ci + y * st, y * z * ci - x * st, ct + z * z * ci });

    // apply to basis vectors
    _direction = simd_mul(_direction, mat);
    _up = simd_mul(_up, mat);
    _uniformsDirty = true;
}

@end

// Camera::Camera()
//{
// }
//
// Camera::~Camera()
//{
// }
//
// void Camera::updateView()
//{
//     simd_float4x4 rot = matrix_identity_float4x4;
//
//     simd_float3 xAxis = simd_make_float3(1.0f, 0.0f, 0.0f);
//     simd_float3 yAxis = simd_make_float3(0.0f, 1.0f, 0.0f);
//     simd_float3 zAxis = simd_make_float3(0.0f, 0.0f, 1.0f);
//     rot               = simd_float4x4_rotate(rot, xAxis, transform_.rotation.x);
//     rot               = simd_float4x4_rotate(rot, yAxis, transform_.rotation.y);
//     rot               = simd_float4x4_rotate(rot, zAxis, transform_.rotation.z);
//
//     simd_float4x4 trans = simd_float4x4_translation(transform_.translation);
//
//     view_ = simd_mul(trans, rot);
// }
//
// void Camera::setTranslation(float x, float y, float z)
//{
//     transform_.translation = { x, y, z };
//     updateView();
// }
//
// void Camera::setRotation(float x, float y, float z)
//{
//     transform_.rotation = { x, y, z };
//     updateView();
// }
//
// void Camera::setPerspective(float aspect, float fov, float znear, float zfar)
//{
//     projection_ = simd_float4x4_perspective(aspect, fov, znear, zfar);
// }
//
// const simd_float4x4& Camera::getProjection() const
//{
//     return projection_;
// }
//
// const simd_float4x4& Camera::getView() const
//{
//     return view_;
// }
