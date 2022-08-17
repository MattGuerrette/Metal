
#import <Foundation/Foundation.h>

#import "graphics_math.h"

XM_ALIGNED_STRUCT(16) CameraUniforms
{
    DirectX::XMMATRIX view;
    DirectX::XMMATRIX projection;
    DirectX::XMMATRIX viewProjection;
    DirectX::XMMATRIX invViewProjection;
    DirectX::XMMATRIX invProjection;
    DirectX::XMMATRIX invView;
};

@interface Camera : NSObject

- (instancetype)initPerspectiveWithPosition:(DirectX::XMFLOAT3)position
                                           :(DirectX::XMFLOAT3)direction
                                           :(DirectX::XMFLOAT3)up
                                           :(float)fov
                                           :(float)aspectRatio
                                           :(float)nearPlane
                                           :(float)farPlane;

@property (readonly) CameraUniforms uniforms;

@end
