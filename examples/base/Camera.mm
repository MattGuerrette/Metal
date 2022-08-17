
#import "Camera.h"

using namespace DirectX;

@interface Camera ()
{
    XMFLOAT3 _position;
    XMFLOAT3 _direction;
    XMFLOAT3 _up;
    CameraUniforms _uniforms;
    float _fov;
    float _aspectRatio;
    float _nearPlane;
    float _farPlane;
}

- (void)updateUniforms;

- (void)updateBasisVectors:(XMFLOAT3)newDirection;

@end

@implementation Camera

- (instancetype)initPerspectiveWithPosition:(DirectX::XMFLOAT3)position :(DirectX::XMFLOAT3)direction :(DirectX::XMFLOAT3)up :(float)fov :(float)aspectRatio :(float)nearPlane :(float)farPlane {
    self = [super init];
    
    _direction = direction;
    _position = position;
    _up = up;
    _fov = fov;
    _aspectRatio = aspectRatio;
    _nearPlane = nearPlane;
    _farPlane = farPlane;
    [self updateBasisVectors:_direction];
    [self updateUniforms];
    
    return self;
}

- (CameraUniforms)uniforms {
    return _uniforms;
}

- (void)updateBasisVectors:(XMFLOAT3)newDirection {
    _direction = newDirection;
    XMVECTOR v = XMLoadFloat3(&_direction);
    v = XMVector3Normalize(v);
    XMStoreFloat3(&_direction, v);

    XMVECTOR upVector = XMLoadFloat3(&_up);
    XMVECTOR right = XMVector3Cross(v, upVector);
    XMVECTOR newUp = XMVector3Cross(right, v);
    XMStoreFloat3(&_up, newUp);
}

- (void)updateUniforms {
    XMVECTOR pos = XMLoadFloat3(&_position);
    XMVECTOR dir = XMLoadFloat3(&_direction);
    XMVECTOR up = XMLoadFloat3(&_up);
    _uniforms.view = XMMatrixLookAtRH(pos, dir, up);
    _uniforms.projection = XMMatrixPerspectiveFovRH(_fov,
        _aspectRatio, _nearPlane, _farPlane);
    _uniforms.viewProjection = _uniforms.projection * _uniforms.view;
}

@end
