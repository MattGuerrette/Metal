

#include "Camera.hpp"

Camera::Camera(Vector3 position,
    Vector3            direction,
    Vector3            up,
    float              fov,
    float              aspectRatio,
    float              nearPlane,
    float              farPlane,
    float              viewWidth,
    float              viewHeight)
    : m_position(position)
    , m_direction(direction)
    , m_fieldOfView(fov)
    , m_aspectRatio(aspectRatio)
    , m_nearPlane(nearPlane)
    , m_farPlane(farPlane)
    , m_viewWidth(viewWidth)
    , m_viewHeight(viewHeight)
{
    updateUniforms();
}

void Camera::setProjection(float fov,
    float                        aspectRatio,
    float                        nearPlane,
    float                        farPlane,
    float                        viewWidth,
    float                        viewHeight)
{
    m_fieldOfView = fov;
    m_aspectRatio = aspectRatio;
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
    m_viewWidth = viewWidth;
    m_viewHeight = viewHeight;
    updateUniforms();
}

Matrix Camera::viewProjection() const
{
    return m_view * m_projection;
}

float Camera::viewWidth() const
{
    return m_viewWidth;
}

float Camera::viewHeight() const
{
    return m_viewHeight;
}

void Camera::moveForward(float dt)
{
    m_position += direction() * dt * m_speed;
    updateUniforms();
}

void Camera::moveBackward(float dt)
{
    m_position -= direction() * dt * m_speed;
    updateUniforms();
}

void Camera::strafeLeft(float dt)
{
    m_position -= right() * dt * m_speed;
    updateUniforms();
}

void Camera::strafeRight(float dt)
{
    m_position += right() * dt * m_speed;
    updateUniforms();
}

void Camera::setPosition(const Vector3& position)
{
    m_position = position;
    updateUniforms();
}

void Camera::setRotation(const Vector3& rotation)
{
    m_rotation = rotation;
    updateUniforms();
}

void Camera::rotate(float pitch, float yaw)
{
    DirectX::XMVECTOR pitchAxis
        = DirectX::XMVector3Rotate(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), m_orientation);

    // Create quaternions for the pitch and yaw rotations
    DirectX::XMVECTOR pitchQuat = DirectX::XMQuaternionRotationAxis(pitchAxis, pitch);
    DirectX::XMVECTOR yawQuat
        = DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), yaw);

    // Combine the pitch and yaw rotations with the current orientation
    m_orientation = DirectX::XMQuaternionMultiply(yawQuat, m_orientation);
    m_orientation = DirectX::XMQuaternionMultiply(m_orientation, pitchQuat);

    // Normalize the quaternion to avoid floating-point drift
    m_orientation = DirectX::XMQuaternionNormalize(m_orientation);

    updateUniforms();
}

Vector3 Camera::direction() const
{
    auto dir = Vector3::Transform(Vector3::Forward, m_orientation);
    dir.Normalize();
    return dir;
}

Vector3 Camera::right() const
{
    auto right = Vector3::Transform(Vector3::Right, m_orientation);
    right.Normalize();
    return right;
}

Vector3 Camera::up() const
{
    auto up = Vector3::Transform(Vector3::Up, m_orientation);
    up.Normalize();
    return up;
}

void Camera::updateUniforms()
{
    auto rotation = Quaternion::CreateFromYawPitchRoll(m_rotation);

    m_view = Matrix::CreateLookAt(m_position, m_position + direction(), up());
    m_projection = Matrix::CreatePerspectiveFieldOfView(
        m_fieldOfView, m_aspectRatio, m_nearPlane, m_farPlane);
    m_viewProjection = m_view * m_projection;
}