
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


class Camera
{
public:
    Camera(DirectX::XMFLOAT3 position,
    DirectX::XMFLOAT3 direction,
    DirectX::XMFLOAT3 up,
    float fov,
    float aspectRatio,
    float nearPlane,
    float farPlane);
    
    const CameraUniforms& GetUniforms() const;
    
private:
    void UpdateBasisVectors(DirectX::XMFLOAT3 direction);
    
    void UpdateUniforms();
    
    CameraUniforms Uniforms;
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Direction;
    DirectX::XMFLOAT3 Up;
    float FieldOfView;
    float AspectRatio;
    float NearPlane;
    float FarPlane;
};

