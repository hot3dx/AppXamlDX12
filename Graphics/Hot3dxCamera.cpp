//// File: Hot3dxCamera.cpp
//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Jeff Kubitz - hot3dx. All rights reserved

#include "pch.h"
#include "Hot3dxCamera.h"
#include "StereoProjection.h"

using namespace DirectX;

#undef min // Use __min instead.
#undef max // Use __max instead.

//--------------------------------------------------------------------------------------

Hot3dxCamera::Hot3dxCamera()
{
    m_eye=(XMFLOAT3(0.0f, 0.7f, 20.0f));
    m_lookAt = XMFLOAT3(0.0f, -0.1f, 0.0f);
    m_up = XMFLOAT3(0.0f, 1.0f, 0.0f);
    // Setup the view matrix.
    SetViewParams(
        XMFLOAT3(m_eye.x, m_eye.y, m_eye.z),   // Default eye position.
        XMFLOAT3(m_lookAt.x, m_lookAt.y, m_lookAt.z),   // Default look at position.
        XMFLOAT3(m_up.x, m_up.y, m_up.z)    // Default up vector.
        );

    // Setup the projection matrix.
    SetProjParams(70.0f * XM_PI / 180.0f, 1.0f, 0.01f, 1000.0f);
}

//--------------------------------------------------------------------------------------

void Hot3dxCamera::LookDirection(_In_ XMFLOAT3 lookDirection)
{
    XMFLOAT3 lookAt;
    lookAt.x = m_eye.x + lookDirection.x;
    lookAt.y = m_eye.y + lookDirection.y;
    lookAt.z = m_eye.z + lookDirection.z;

    SetViewParams(m_eye, lookAt, m_up);
}

//--------------------------------------------------------------------------------------

void Hot3dxCamera::Eye(_In_ XMFLOAT3 eye)
{
    SetViewParams(eye, m_lookAt, m_up);
}

void Hot3dxCamera::UpDirection(_In_ XMFLOAT3 upDir)
{
    m_up.x = upDir.x;
    m_up.y = upDir.y;
    m_up.z = upDir.z;
    SetViewParams(m_eye, m_lookAt, upDir);
}

//--------------------------------------------------------------------------------------

void Hot3dxCamera::SetViewParams(
    _In_ XMFLOAT3 eye,
    _In_ XMFLOAT3 lookAt,
    _In_ XMFLOAT3 up
    )
{
    m_eye = eye;
    m_lookAt = lookAt;
    m_up = up;

    // Calculate the view matrix.
    XMMATRIX view = XMMatrixLookAtLH(
        XMLoadFloat3(&m_eye),
        XMLoadFloat3(&m_lookAt),
        XMLoadFloat3(&m_up)
        );

    XMVECTOR det;
    XMMATRIX inverseView = XMMatrixInverse(&det, view);
    XMStoreFloat4x4(&m_viewMatrix, view);
    XMStoreFloat4x4(&m_inverseView, inverseView);

    // The axis basis vectors and camera position are stored inside the
    // position matrix in the 4 rows of the camera's world matrix.
    // To figure out the yaw/pitch of the camera, we just need the Z basis vector.
    XMFLOAT3 zBasis;
    XMStoreFloat3(&zBasis, inverseView.r[2]);

    m_cameraYawAngle = atan2f(zBasis.x, zBasis.z);

    float len = sqrtf(zBasis.z * zBasis.z + zBasis.x * zBasis.x);
    m_cameraPitchAngle = atan2f(zBasis.y, len);
}

//--------------------------------------------------------------------------------------

void Hot3dxCamera::SetProjParams(
    _In_ float fieldOfView,
    _In_ float aspectRatio,
    _In_ float nearPlane,
    _In_ float farPlane
    )
{
    // Set attributes for the projection matrix.
    m_fieldOfView = fieldOfView;
    m_aspectRatio = aspectRatio;
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
    XMStoreFloat4x4(
        &m_projectionMatrix,
        XMMatrixPerspectiveFovLH(
            m_fieldOfView,
            m_aspectRatio,
            m_nearPlane,
            m_farPlane
            )
        );

    STEREO_PARAMETERS* stereoParams = nullptr;
    // Update the projection matrix.
    XMStoreFloat4x4(
        &m_projectionMatrixLeft,
        MatrixStereoProjectionFovLH(
            stereoParams,
            STEREO_CHANNEL::LEFT,
            m_fieldOfView,
            m_aspectRatio,
            m_nearPlane,
            m_farPlane,
            STEREO_MODE::NORMAL
            )
        );

    XMStoreFloat4x4(
        &m_projectionMatrixRight,
        MatrixStereoProjectionFovLH(
            stereoParams,
            STEREO_CHANNEL::RIGHT,
            m_fieldOfView,
            m_aspectRatio,
            m_nearPlane,
            m_farPlane,
            STEREO_MODE::NORMAL
            )
        );
}

//--------------------------------------------------------------------------------------

DirectX::XMMATRIX Hot3dxCamera::View()
{
    return XMLoadFloat4x4(&m_viewMatrix);
}

//--------------------------------------------------------------------------------------

DirectX::XMMATRIX Hot3dxCamera::Projection()
{
    return XMLoadFloat4x4(&m_projectionMatrix);
}

//--------------------------------------------------------------------------------------

DirectX::XMMATRIX Hot3dxCamera::LeftEyeProjection()
{
    return XMLoadFloat4x4(&m_projectionMatrixLeft);
}

//--------------------------------------------------------------------------------------

DirectX::XMMATRIX Hot3dxCamera::RightEyeProjection()
{
    return XMLoadFloat4x4(&m_projectionMatrixRight);
}

//--------------------------------------------------------------------------------------

DirectX::XMMATRIX Hot3dxCamera::World()
{
    return XMLoadFloat4x4(&m_inverseView);
}

//--------------------------------------------------------------------------------------

DirectX::XMFLOAT3 Hot3dxCamera::Eye()
{
    return m_eye;
}

//--------------------------------------------------------------------------------------

DirectX::XMFLOAT3 Hot3dxCamera::LookAt()
{
    return m_lookAt;
}

DirectX::XMFLOAT3 Hot3dxCamera::Up()
{
    return m_up;
}

//--------------------------------------------------------------------------------------

float Hot3dxCamera::NearClipPlane()
{
    return m_nearPlane;
}

//--------------------------------------------------------------------------------------

float Hot3dxCamera::FarClipPlane()
{
    return m_farPlane;
}

//--------------------------------------------------------------------------------------

float Hot3dxCamera::Pitch()
{
    return m_cameraPitchAngle;
}

//--------------------------------------------------------------------------------------

float Hot3dxCamera::Yaw()
{
    return m_cameraYawAngle;
}

void Hot3dxCamera::RotateYaw(float deg)
{
    XMMATRIX rotation = XMMatrixRotationAxis(XMVectorSet(m_up.x, m_up.y, m_up.z, 0.0f), deg);

    XMVECTOR eye = XMVector3TransformCoord(XMVectorSet(m_eye.x, m_eye.y, m_eye.z, 0.0f), rotation);
    m_eye = { XMVectorGetX(eye), XMVectorGetY(eye), XMVectorGetZ(eye) };
}

void Hot3dxCamera::RotatePitch(float deg)
{
    XMVECTOR right = XMVector3Normalize(XMVector3Cross(XMVectorSet(m_eye.x, m_eye.y, m_eye.z, 0.0f), 
        XMVectorSet(m_up.x, m_up.y, m_up.z, 0.0f)));
    XMMATRIX rotation = XMMatrixRotationAxis(right, deg);
    
    XMVECTOR eye = XMVector3TransformCoord(XMVectorSet(m_eye.x, m_eye.y, m_eye.z, 0.0f), rotation);
    m_eye = { XMVectorGetX(eye), XMVectorGetY(eye), XMVectorGetZ(eye) };
}

void Hot3dxCamera::Reset()
{
    m_eye = { 0.0f, 0.0f, -20.0f};
    m_lookAt = { 0.0f, 0.01f, 0.0f};
    m_up = { 0.0f, 1.0f, 0.0f};
}

void Hot3dxCamera::Set(XMFLOAT3 eye, XMFLOAT3 at, XMFLOAT3 up)
{
    m_eye = eye;
    m_lookAt = at;
    m_up = up;
}
//--------------------------------------------------------------------------------------