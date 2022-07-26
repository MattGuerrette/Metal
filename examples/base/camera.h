///*
//*    Metal Examples
//*    Copyright(c) 2019 Matt Guerrette
//*
//*    This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
//*/
//
//#pragma once
//
//#include "platform.h"
//#import <simd/simd.h>
//
//struct Transform {
//    simd::float3 translation = 0.0f;
//    simd::float3 rotation = 0.0f;
//    simd::float3 scale = 0.0f;
//};
//
//class Camera {
//public:
//    Camera();
//
//    ~Camera();
//
//    void setTranslation(float x, float y, float z);
//
//    void setRotation(float x, float y, float z);
//
//    void setPerspective(float aspect, float fov, float znear, float zfar);
//
//    const simd_float4x4& getProjection() const;
//    const simd_float4x4& getView() const;
//
//
//    Transform     transform_;
//private:
//    simd::float4x4 projection_ = matrix_identity_float4x4;
//    simd::float4x4 view_       = matrix_identity_float4x4;
//
//    void updateView();
//};

/*
See LICENSE folder for this sample’s licensing information.

Abstract:
Declaration of the AAPLCamera object.
*/


#import <Foundation/Foundation.h>
#import <simd/simd.h>

#define NUM_CASCADES (3)

// Matrices that are stored and generated internally within the camera object
struct AAPLCameraUniforms {
    simd::float4x4 viewMatrix;
    simd::float4x4 projectionMatrix;
    simd::float4x4 viewProjectionMatrix;
    simd::float4x4 invOrientationProjectionMatrix;
    simd::float4x4 invViewProjectionMatrix;
    simd::float4x4 invProjectionMatrix;
    simd::float4x4 invViewMatrix;
    simd::float4 frustumPlanes[6];
};

struct AAPLUniforms {
    AAPLCameraUniforms cameraUniforms;
    AAPLCameraUniforms shadowCameraUniforms[3];

    // Mouse state: x,y = position in pixels; z = buttons
    simd::float3 mouseState;
    simd::float2 invScreenSize;
    float projectionYScale;
    float brushSize;

    float ambientOcclusionContrast;
    float ambientOcclusionScale;
    float ambientLightScale;
    float gameTime;
    float frameTime;
};

struct AAPLDebugVertex {
    simd::float4 position;
    simd::float4 color;
};

// Describes our standardized OBJ format geometry vertex format
struct AAPLObjVertex {
    simd::float3 position;
    simd::float3 normal;
    simd::float3 color;
#if !METAL
    bool operator==(const AAPLObjVertex& o) const
    {
        return simd::all(o.position == position) && simd::all(o.normal == normal) && simd::all(o.color == color);
    }
#endif
};

// The camera object has only six writable properties:
// - position, direction, up define the orientation and position of the camera
// - nearPlane and farPlane define the projection planes
// - viewAngle defines the view angle in radians
//  All other properties are generated from these values.
//  - Note: the AAPLCameraUniforms struct is populated lazily on demand to reduce CPU overhead

@interface AAPLCamera : NSObject {
    // Internally generated camera uniforms used/defined by the renderer
    AAPLCameraUniforms _uniforms;

    // A boolean value that denotes if the intenral uniforms structure needs rebuilding
    bool _uniformsDirty;

    // - Note: The camera can be either perspective or parallel, depending on a defined angle OR a defined width

    // Full view angle inradians for perspective view; 0 for parallel view
    float _viewAngle;

    // Width of back plane for parallel view; 0 for perspective view
    float _width;

    // Direction of the camera; is normalized
    simd::float3 _direction;

    // Position of the camera/observer point
    simd::float3 _position;

    // Up direction of the camera; perpendicular to _direction
    simd::float3 _up;

    // Distance of the near plane to _position in world space
    float _nearPlane;

    // Distance of the far plane to _position in world space
    float _farPlane;

    // Aspect ratio of the horizontal against the vertical (widescreen gives < 1.0 value)
    float _aspectRatio;
}

// Updates internal uniforms from the various properties
- (void)updateUniforms;

// Rotates camera around axis; updating many properties at once
- (void)rotateOnAxis:(simd::float3)inAxis radians:(float)inRadians;
- (instancetype)initPerspectiveWithPosition:(simd::float3)position
                                  direction:(simd::float3)direction
                                         up:(simd::float3)up
                                  viewAngle:(float)viewAngle
                                aspectRatio:(float)aspectRatio
                                  nearPlane:(float)nearPlane
                                   farPlane:(float)farPlane;

- (instancetype)initParallelWithPosition:(simd::float3)position
                               direction:(simd::float3)direction
                                      up:(simd::float3)up
                                   width:(float)width
                                  height:(float)height
                               nearPlane:(float)nearPlane
                                farPlane:(float)farPlane;

// Internally generated uniform; maps to _uniforms
@property (readonly) AAPLCameraUniforms uniforms;

// Left of the camera
@property (readonly) simd::float3 left;

// Right of the camera
@property (readonly) simd::float3 right;

// Down direction of the camera
@property (readonly) simd::float3 down;

// Facing direction of the camera (alias of direction)
@property (readonly) simd::float3 forward;

// Backwards direction of the camera
@property (readonly) simd::float3 backward;

// Returns true if perspective (viewAngle != 0, width == 0)
@property (readonly) bool isPerspective;

// Returns true if perspective (width != 0, viewAngle == 0)
@property (readonly) bool isParallel;

// Position/observer point of the camera
@property simd::float3 position;

// Facing direction of the camera
@property simd::float3 direction;

// Up direction of the camera; perpendicular to direction
@property simd::float3 up;

// Full viewing angle in radians
@property float viewAngle;

// Aspect ratio in width / height
@property float aspectRatio;

// Distance from near plane to observer point (position)
@property float nearPlane;

// Distance from far plane to observer point (position)
@property float farPlane;

// Accessors for position, direction, angle
- (float)viewAngle;
- (float)width;
- (simd::float3)position;
- (simd::float3)direction;
- (float)nearPlane;
- (float)farPlane;
- (float)aspectRatio;
- (AAPLCameraUniforms)uniforms;

- (bool)isPerspective;
- (bool)isParallel;

// Accessors for read-only derivitives
- (simd::float3)left;
- (simd::float3)right;
- (simd::float3)down;
- (simd::float3)forward;
- (simd::float3)backward;

// Setters for posing properties
- (void)setViewAngle:(float)newAngle;
- (void)setWidth:(float)newWidth;
- (void)setPosition:(simd::float3)newPosition;
- (void)setDirection:(simd::float3)newDirection;
- (void)setNearPlane:(float)newNearPlane;
- (void)setFarPlane:(float)newFarPlane;
- (void)setAspectRatio:(float)newAspectRatio;
@end
