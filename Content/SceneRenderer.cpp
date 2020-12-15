//--------------------------------------------------------------------------------------
// File: SceneRenderer.cpp
//
// Copyright (c) Jeff Kubitz - hot3dx. All rights reserved.
// 
//
//--------------------------------------------------------------------------------------
#include "pch.h"
#include "SceneRenderer.h"
#include "..\Common\DirectXHelper.h"
#include <ppltasks.h>
#include <synchapi.h>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <system_error>
#include <thread>
#include <utility>
#include "DirectXPage.xaml.h"
#include <strsafe.h>
#include "..\Graphics\RenderTargetState.h"
#include "..\Graphics\EffectPipelineStateDescription.h"
#include "..\Graphics\CommonStates.h"
#include "..\Graphics\GraphicsMemory.h"
#include "..\Graphics\VertexTypes.h"
#include "..\Graphics\MyResourceUploadBatch.h"
#include "..\Graphics\GeometricPrimitive.h"
#include "..\Graphics\Geometry.h"
#include "..\Graphics\Effects.h"
#include "..\Graphics\EffectCommon.h"
#include "..\Graphics\DDSTextureLoader.h"

using namespace AppXamlDX12;

using namespace DX;
using namespace Concurrency;
using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::Storage;
using namespace Windows::System::Threading;
using namespace Windows::UI::Xaml;


// Loads vertex and pixel shaders from files and instantiates the cube geometry.
SceneRenderer::SceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_EyeX(0.0f), m_EyeY(0.0f), m_EyeZ(20.0f),
	m_LookAtX(0.0f), m_LookAtY(0.01f), m_LookAtZ(0.0f),
	m_UpX(0.0f), m_UpY(1.0f), m_UpZ(0.0f),
	m_posX(0.0f), m_posY(0.0f), m_posZ(0.0f),
	m_fPointSpace(0.3f),
	m_fScrollDist(0.1F),
	m_iScrollPointSetPos(150),
	m_iPointCount(0),
	m_widthRatio(0.0254000f),
    m_heightRatio(0.039000f),
	m_loadingComplete(false),
	m_radiansPerSecond(XM_PIDIV4 / 2),	// rotate 45 degrees per second
	m_angle(0.01f),
	m_tracking(false),
	sceneVertexCount(8),
	m_sceneDeviceResources(deviceResources),
	m_isRightHanded(true)
{
	if (!m_isRightHanded) { m_EyeZ = -20.0f; }

	LoadState();
	ZeroMemory(&m_constantBufferData, sizeof(m_constantBufferData));
	
	m_camera = ref new Hot3dxCamera();

	CreateWindowSizeDependentResources();
	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}

SceneRenderer::~SceneRenderer()
{
	m_loadingComplete = false;
	
}

void SceneRenderer::CreateDeviceDependentResources()
{
	
	if (m_loadingComplete == false)
	{
	
		
			auto device = m_sceneDeviceResources->GetD3DDevice();

			if (!device)
			{

				return;
			}

			m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

			m_states = std::make_unique<CommonStates>(device);

			m_resourceDescriptors = std::make_unique<DescriptorHeap>(device,
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
				Descriptors::Count);

			m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(device);

			m_shape = GeometricPrimitive::CreateTeapot(4.f, 8);

			m_shapeTetra = GeometricPrimitive::CreateTetrahedron(0.5f);
			// SDKMESH has to use clockwise winding with right-handed coordinates, so textures are flipped in U
			//   // C:\\Users\\hot3dx-home\\Source\\Repos\\GameD12UW\\Assets\\...

			m_model = Model::CreateFromSDKMESH(L"Assets\\tiny.sdkmesh");//, device);
			{
				
				        ResourceUploadBatch* m_resourceUpload = new ResourceUploadBatch(device);

				        // Begin Resource Upload
				        m_resourceUpload->BeginXaml();
				        m_model->LoadStaticBuffers(device, *m_resourceUpload);
						// C:\\Users\\hot3dx-home\\Source\\AppXamlDX12\\x64\\Debug\\AppXamlDX12\\AppX\\

						DX::ThrowIfFailed(
							CreateDDSTextureFromFile(device, *m_resourceUpload, L"Assets\\seafloor.dds", &m_texture1)//.ReleaseAndGetAddressOf())
					);

				CreateShaderResourceView(device, m_texture1.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::SeaFloor));

				DX::ThrowIfFailed(
					CreateDDSTextureFromFile(device, *m_resourceUpload, L"Assets\\windowslogo.dds", &m_texture2)//.ReleaseAndGetAddressOf())
					);

				CreateShaderResourceView(device, m_texture2.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::WindowsLogo));

				RenderTargetState rtState(m_sceneDeviceResources->GetBackBufferFormat(), m_sceneDeviceResources->GetDepthBufferFormat());
				// Each effect object must ne proceeded by its own 
				// EffectPipelineStateDescription pd even if the EffectPipelineStateDescription pd is the same
				{
					EffectPipelineStateDescription pd(
						&VertexPositionColor::InputLayout,
						CommonStates::Opaque,
						CommonStates::DepthNone,
						CommonStates::CullNone,
						rtState,
						D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);

					m_lineEffect = std::make_unique<BasicEffect>(device, EffectFlags::VertexColor, pd);
				}

				{
					EffectPipelineStateDescription pd(
						&GeometricPrimitive::VertexType::InputLayout,
						CommonStates::Opaque,
						CommonStates::DepthDefault,
						CommonStates::CullNone,
						rtState);

					m_shapeEffect = std::make_unique<BasicEffect>(device, EffectFlags::PerPixelLighting | EffectFlags::Texture, pd);
					m_shapeEffect->EnableDefaultLighting();
					m_shapeEffect->SetTexture(m_resourceDescriptors->GetGpuHandle(Descriptors::SeaFloor), m_states->AnisotropicWrap());

				}

				{
					EffectPipelineStateDescription pd(
						&GeometricPrimitive::VertexType::InputLayout,
						CommonStates::Opaque,
						CommonStates::DepthDefault,
						CommonStates::CullNone,
						rtState);

					m_shapeTetraEffect = std::make_unique<BasicEffect>(device, EffectFlags::PerPixelLighting | EffectFlags::Texture, pd);
					m_shapeTetraEffect->EnableDefaultLighting();
					m_shapeTetraEffect->SetTexture(m_resourceDescriptors->GetGpuHandle(Descriptors::SeaFloor), m_states->AnisotropicWrap());
				}

				{
					SpriteBatchPipelineStateDescription pd(rtState);

					m_sprites = std::make_unique<SpriteBatch>(device, *m_resourceUpload, pd);
				}

				m_modelResources = m_model->LoadTextures(device, *m_resourceUpload, L"Assets\\");

				{
					EffectPipelineStateDescription psd(
						nullptr,
						CommonStates::Opaque,
						CommonStates::DepthDefault,
						CommonStates::CullClockwise,    // Using RH coordinates, and SDKMESH is in LH coordiantes
						rtState);

					EffectPipelineStateDescription alphapsd(
						nullptr,
						CommonStates::NonPremultiplied, // Using straight alpha
						CommonStates::DepthRead,
						CommonStates::CullClockwise,    // Using RH coordinates, and SDKMESH is in LH coordiantes
						rtState);

					m_modelEffects = m_model->CreateEffects(psd, alphapsd, m_modelResources->Heap(), m_states->Heap());
				}

				m_smallFont = std::make_unique<SpriteFont>(device, *m_resourceUpload,
					L"SegoeUI_18.spritefont", //L"italic.spritefont",
					m_resourceDescriptors->GetCpuHandle(Descriptors::SegoeFont),
					m_resourceDescriptors->GetGpuHandle(Descriptors::SegoeFont));

				m_font = std::make_unique<SpriteFont>(device, *m_resourceUpload,
					L"SegoeUI_18.spritefont",
					m_resourceDescriptors->GetCpuHandle(Descriptors::SegoeFont),
					m_resourceDescriptors->GetGpuHandle(Descriptors::SegoeFont));
				HANDLE completeEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
					auto loaded = m_resourceUpload->EndXaml(m_sceneDeviceResources->GetCommandQueue());
					WaitForSingleObject(m_resourceUpload->GetGPUHandle(), INFINITE);

					m_shapeEffect->SetProjection(XMLoadFloat4x4(&m_projection4x4));
					m_shapeTetraEffect->SetProjection(XMLoadFloat4x4(&m_projection4x4));
					m_lineEffect->SetProjection(XMLoadFloat4x4(&m_projection4x4));

						const D3D12_VIEWPORT viewport = m_sceneDeviceResources->GetScreenViewport();
						
						m_sprites->SetViewport(viewport);

						m_sprites->SetRotation(m_sceneDeviceResources->GetRotation());
						m_sceneDeviceResources->WaitForGpu();
					loaded.then([this]()
					{
						m_loadingComplete = true;
						
					});
					
	    }
	} // eo ! m_loadingComplete = false
}


// Initializes view parameters when the window size changes.
void SceneRenderer::CreateWindowSizeDependentResources()
{
	//OutputDebugString(L"\nEntered SceneRender: CreateWindowSizeDependentResources\n");
	Size outputSize = m_sceneDeviceResources->GetOutputSize();
	float aspectRatio = outputSize.Width / outputSize.Height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	D3D12_VIEWPORT viewport = m_sceneDeviceResources->GetScreenViewport();
	m_sceneDeviceResources->SetScissorRect(0, 0, 
		static_cast<LONG>(viewport.Width), 
		static_cast<LONG>(viewport.Height));

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}

	// Note that the OrientationTransform3D matrix is post-multiplied here
	// in order to correctly orient the scene to match the display orientation.
	// This post-multiplication step is required for any draw calls that are
	// made to the swap chain render target. For draw calls to other targets,
	// this transform should not be applied.

	// This sample makes use of a right-handed coordinate system using row-major matrices.
	XMMATRIX perspectiveMatrix;
	if (!m_isRightHanded) {
		perspectiveMatrix = XMMatrixPerspectiveFovLH(
			fovAngleY,
			aspectRatio,
			0.01f,
			100.0f
		);
	}
	else {
		perspectiveMatrix = XMMatrixPerspectiveFovRH(
			fovAngleY,
			aspectRatio,
			0.01f,
			100.0f
		);
	}
	XMFLOAT4X4 orientation = m_sceneDeviceResources->GetOrientationTransform3D();
	XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

	DirectX::XMStoreFloat4x4(
		&m_constantBufferData.projection,
		XMMatrixTranspose(perspectiveMatrix * orientationMatrix)
		);

	// Eye is at (0,0.7,10.0), looking at point (0,-0.1,0) with the up-vector along the y-axis.
	XMVECTOR eye = XMVectorSet(m_EyeX, m_EyeY, m_EyeZ, 0.0f);
	XMVECTOR at = XMVectorSet(m_LookAtX, m_LookAtY, m_LookAtZ, 0.0f);
	XMVECTOR up = XMVectorSet(m_UpX, m_UpY, m_UpZ, 0.0f);
	XMFLOAT4 Feye = { m_EyeX, m_EyeY, m_EyeZ, 0.0f };
	XMFLOAT4 Fat = { m_LookAtX, m_LookAtY, m_LookAtZ, 0.0f };

	if (!m_isRightHanded)
	{
		DirectX::XMStoreFloat4x4(&m_constantBufferData.view,
			XMMatrixTranspose(XMMatrixLookAtRH(eye, at, up)));
	}
	else {
		DirectX::XMStoreFloat4x4(&m_constantBufferData.view,
			XMMatrixTranspose(XMMatrixLookAtRH(eye, at, up)));
	}
	XMStoreFloat4(&Feye, XMVector4Transform(XMLoadFloat4(&Feye), XMMatrixTranslation(Fat.x, Fat.y, Fat.z)));

	XMStoreFloat4x4(&m_projection4x4, (perspectiveMatrix * orientationMatrix));
}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void SceneRenderer::Update(DX::StepTimer const& timer)
{
	m_timer = timer;
	
	if (m_loadingComplete == true)
	{
		if (!m_tracking)
		{
			// Rotate the cube a small amount.
			m_angle += static_cast<float>(m_timer.GetElapsedSeconds()) * m_radiansPerSecond;

			Rotate(m_angle);
		}


		// Update the constant buffer resouRender(rce.
		//UINT8* destination = m_mappedConstantBuffer + (m_sceneDeviceResources->GetCurrentFrameIndex() * c_alignedConstantBufferSize);
		//memcpy(destination, &m_constantBufferData, sizeof(m_constantBufferData));

		PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

		m_camera->Eye(XMFLOAT3(m_EyeX, m_EyeY, m_EyeZ));
		m_camera->LookDirection(XMFLOAT3(m_LookAtX, m_LookAtY, m_LookAtZ));
		m_camera->UpDirection(XMFLOAT3(m_UpX, m_UpY, m_UpZ));

		XMVECTOR eye = XMVectorSet(m_EyeX, m_EyeY, m_EyeZ, 0.0f);
		XMVECTOR at = XMVectorSet(m_LookAtX, m_LookAtY, m_LookAtZ, 0.0f);
		XMVECTOR up = XMVectorSet(m_UpX, m_UpY, m_UpZ, 0.0f);
		
		XMFLOAT4 Feye = { m_EyeX, m_EyeY, m_EyeZ, 0.0f };
		XMFLOAT4 Fat = { m_LookAtX, m_LookAtY, m_LookAtZ, 0.0f };
		XMFLOAT4 Fup = {m_UpX, m_UpY, m_UpZ, 0.0f};
		if (!m_isRightHanded)
		{
			DirectX::XMStoreFloat4x4(&m_constantBufferData.view, DirectX::XMMatrixTranspose(DirectX::XMMatrixLookAtLH(eye, at, up)));
			XMStoreFloat4(&Feye, XMVector4Transform(XMLoadFloat4(&Feye), XMMatrixTranslation(Feye.x, Feye.y, Feye.z)));
			XMMatrixTranslation(Feye.x, Feye.y, Feye.z);
			// store m_view
			XMStoreFloat4x4(&m_view4x4, DirectX::XMMatrixLookAtLH(eye, at, up));
			// store m_world 
			XMStoreFloat4x4(&m_world4x4, DirectX::XMMatrixRotationY(float(m_timer.GetTotalSeconds() * XM_PIDIV4)));
		}
		else {

			DirectX::XMStoreFloat4x4(&m_constantBufferData.view,
				XMMatrixTranspose(XMMatrixLookAtRH(eye, at, up)));
			XMStoreFloat4(&Feye, XMVector4Transform(XMLoadFloat4(&Feye), XMMatrixTranslation(Feye.x, Feye.y, Feye.z)));
			XMMatrixTranslation(Feye.x, Feye.y, Feye.z);
			// store m_view
			XMStoreFloat4x4(&m_view4x4, DirectX::XMMatrixLookAtRH(eye, at, up));
			// store m_world 
			XMStoreFloat4x4(&m_world4x4, DirectX::XMMatrixRotationY(float(m_timer.GetTotalSeconds() * XM_PIDIV4)));
		}
		//m_world = DirectX::XMMatrixLookToRH(eye, at, up);
		//eye = XMLoadFloat4(&Feye);
		//at  = XMLoadFloat4(&Fat);
		//XMStoreFloat4(&Feye, XMVector3Normalize(XMVectorSubtract(at, eye)));
		//XMStoreFloat4(&Fat, XMVector4Transform(XMLoadFloat4(&Fat), XMMatrixTranslation(Feye.x, Feye.y, Feye.z)));

		//XMFLOAT3 feye = { m_EyeX, m_EyeY, m_EyeZ };
		//XMFLOAT3 fat = { m_LookAtX, m_LookAtY, m_LookAtZ};
		//XMFLOAT3 fup = { m_UpX, m_UpY, m_UpZ,};
		//m_camera->SetViewParams(feye, fat, fup);
		//DirectX::XMStoreFloat4x4(&m_constantBufferData.view,
		//	XMMatrixTranspose(XMMatrixLookAtRH(eye, at, up)));
		//m_projection = perspectiveMatrix * orientationMatrix;

		m_lineEffect->SetView(XMLoadFloat4x4(&m_view4x4));
		m_lineEffect->SetWorld(DirectX::XMMatrixIdentity());

		m_shapeEffect->SetView(XMLoadFloat4x4(&m_view4x4));
		m_shapeTetraEffect->SetView(XMLoadFloat4x4(&m_view4x4));
		
		m_sprites->SetViewport(m_sceneDeviceResources->GetScreenViewport());
		/*
		m_audioTimerAcc -= (float)timer.GetElapsedSeconds();
		if (m_audioTimerAcc < 0)
		{
			if (m_retryDefault)
			{
				m_retryDefault = false;
				if (m_audEngine->Reset())
				{
					// Restart looping audio
					m_effect1->Play(true);
				}
			}
			else
			{
				m_audioTimerAcc = 4.f;

				m_waveBank->Play(m_audioEvent++);

				if (m_audioEvent >= 11)
					m_audioEvent = 0;
			}
		}

		auto pad = m_gamePad->GetState(0);
		if (pad.IsConnected())
		{
			m_gamePadButtons.Update(pad);

			if (pad.IsViewPressed())
			{
				ExitGameD12UW();
			}
		}
		else
		{
			m_gamePadButtons.Reset();
		}

		auto kb = m_keyboard->GetState();
		m_keyboardButtons.Update(kb);

		if (kb.Escape)
		{
			ExitGameD12UW();
		}

		auto mouse = m_mouse->GetState();
		mouse;
		*/

		PIXEndEvent();
		
	}

}

#pragma endregion

#pragma region Frame Render

// Saves the current state of the renderer.
void SceneRenderer::SaveState()
{

	auto state = ApplicationData::Current->LocalSettings->Values;

	if (state->HasKey(AngleKey))
	{
		state->Remove(AngleKey);
	}
	if (state->HasKey(TrackingKey))
	{
		state->Remove(TrackingKey);
	}

	state->Insert(AngleKey, PropertyValue::CreateSingle(m_angle));
	state->Insert(TrackingKey, PropertyValue::CreateBoolean(m_tracking));
}

// Restores the previous state of the renderer.
void SceneRenderer::LoadState()
{
	auto state = ApplicationData::Current->LocalSettings->Values;
	if (state->HasKey(AngleKey))
	{
		m_angle = safe_cast<IPropertyValue^>(state->Lookup(AngleKey))->GetSingle();
		state->Remove(AngleKey);
	}
	if (state->HasKey(TrackingKey))
	{
		m_tracking = safe_cast<IPropertyValue^>(state->Lookup(TrackingKey))->GetBoolean();
		state->Remove(TrackingKey);
	}
}

// Rotate the 3D cube model a set amount of radians.
void SceneRenderer::Rotate(float radians)
{

	// Prepare to pass the updated model matrix to the shader.
	DirectX::XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(XMMatrixRotationY(radians)));
}

void SceneRenderer::StartTracking()
{
	m_tracking = true;
}

BOOL SceneRenderer::checkDistance(float x, float y, float z, float mouseMoveDistDelta)
{
	BOOL tf = false;
	float xx = x - distX;
	if (xx < 0)xx = -xx;
	float yy = y - distY;
	if (yy < 0)yy = -yy;
	float zz = z - distZ;
	if (zz < 0)zz = -zz;

	if ((xx >= mouseMoveDistDelta) ||
		(yy >= mouseMoveDistDelta) ||
		(zz >= mouseMoveDistDelta))
	{

		distX = x; distY = y; distZ = z;
		tf = true;
	}
	return tf;

}

// When tracking, the 3D cube can be rotated around its Y axis by tracking pointer position relative to the output screen width.
void SceneRenderer::TrackingUpdate(float positionX, float positionY)
{
	//MouseCursorRender(positionX, positionY);

	if (m_tracking)
	{
		D3D12_VIEWPORT rect = m_sceneDeviceResources->GetScreenViewport();
		// convert mouse points to number line 
		// plus/ minus coordinates
		// and convert to float
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
		int centerx;
		int centery;
		// convert mouse points to number line 
		// plus/ minus coordinates
		// and convert to float
		point.x = positionX;
		point.y = positionY;
		if (m_loadingComplete) {

			if (m_bLButtonDown == true)
				if (positionX > 0 || point.x < (rect.TopLeftX - rect.Width)
					|| point.y > 0 || point.y < (rect.TopLeftY - rect.Height))
				{
					centerx = (int)(rect.Width - rect.TopLeftX) / 2;
					centery = (int)(rect.Height - rect.TopLeftY) / 2;
					x = (float)((point.x - centerx) * m_widthRatio);
					y = -(float)((point.y - centery) * m_heightRatio);
				}
		}
		else
		{
			centerx = (int)(rect.Width - rect.TopLeftX) / 2;
			centery = (int)(rect.Height - rect.TopLeftY) / 2;
			x = (float)(((point.x) - centerx) / (centerx / 28.0f));
			y = (float)-(((point.y) - centery) / (centery / 21.0f));
		}
		posX [m_iPointCount] = m_posX = x;
		posY[m_iPointCount] = m_posY = y;
		posZ[m_iPointCount] = m_posZ = z;
		m_iPointCount++;
		float radians = XM_2PI * 2.0f * positionX / m_sceneDeviceResources->GetOutputSize().Width;
		Rotate(radians);

		{
			XMFLOAT3 intersect3 = {};
			CHot3dxD3D12Geometry* geo = new CHot3dxD3D12Geometry();

			XMVECTOR a; XMVECTOR b; XMVECTOR c;
#if defined(_XM_ARM_NEON_INTRINSICS_)
			
			a.n128_f32[0] = 50.0f;
			a.n128_f32[1] = 50.0f;
			a.n128_f32[2] = 0.0f;
			a.n128_f32[3] = 0.0f;
			b.n128_f32[0] = 50.0f;
			b.n128_f32[1] = -50.0f;
			b.n128_f32[2] = 0.0f;
			b.n128_f32[3] = 0.0f;
			c.n128_f32[0] = -50.0f;
			c.n128_f32[1] = -50.0f;
			c.n128_f32[2] = 0.0f;
			c.n128_f32[3] = 0.0f;
#endif
#if defined(_XM_SSE_INTRINSICS_)
			a = { 50.0f, 50.0f, 0.0f, 0.0f };
			b = { 50.0f, -50.0f, 0.0f, 0.0f };
			c = { -50.0f, -50.0f, 0.0f, 0.0f };
#endif
			XMVECTOR eye = XMVectorSet(m_EyeX, m_EyeY, m_EyeZ, 0.0f);
			XMVECTOR lineDirection = XMVectorSet(m_LookAtX, m_LookAtY, m_LookAtZ, 0.0f);
			XMVECTOR planeNormal = geo->FindPlaneNormalVec(a, b, c);
			double planeConstant = geo->FindPlaneConstantVec(planeNormal, a);
			XMVECTOR intersect = geo->FindPointIntersectPlaneVec(planeNormal, eye, lineDirection, 20.0f);

			point.x = x;
			point.y = y;

			if (m_fPointSpace > 0.0F) {

				if (m_fScrollDist > 0.0F) {
					if (checkDistance(XMVectorGetX(intersect), XMVectorGetY(intersect), XMVectorGetZ(intersect), m_fScrollDist))
					{
						intersect3 = { XMVectorGetX(intersect), XMVectorGetY(intersect), XMVectorGetZ(intersect) };
						pos.push_back(intersect3);

						m_iTempGroup[m_iTempGroupCount] = m_iPointCount;
						m_iTempMouseX[m_iTempGroupCount] = point.x;
						m_iTempMouseY[m_iTempGroupCount] = point.y;
						//p.Format(L"%d",m_iPointCount);
						//apoint->m_sName = p;
						m_iTempGroupCount++;
						m_iPointCount++;

					}//eo if(checkDistance(intersect.x, intersect.y, intersect.zz, m_fScrollDist)==true)
				}// eo if(m_fScrollDist>0.0F)
				else
				{

					pos.push_back(intersect3);

					m_iTempGroup[m_iTempGroupCount] = m_iPointCount;
					m_iTempMouseX[m_iTempGroupCount] = point.x;
					m_iTempMouseY[m_iTempGroupCount] = point.y;
					m_iTempGroupCount++;
					m_iPointCount++;
					//p.Format(L"%d",m_iPointCount);
					//apoint->m_sName = p;
					//apoint->~CD3D9Tetra();
				}// eo else if(m_fScrollDist>0.0F)

				//Render();

			//}//eoif(SUCCEEDED(visual->Q

				pSect = intersect3;

			}// eo if(m_bLButtonDown==true){
		}

	}
}

void SceneRenderer::StopTracking()
{
	m_tracking = false;
}

void SceneRenderer::ReleaseDeviceDependentResources()
{

}

void SceneRenderer::OnDeviceLost()
{
		ReleaseDeviceDependentResources();
		
		m_texture1.Reset();
		m_texture2.Reset();

		m_font.reset();
		m_smallFont.reset();
		m_batch.reset();
		m_shape.reset();
		m_shapeTetra.reset();
		m_model.reset();
		m_lineEffect.reset();
		m_shapeEffect.reset();
		m_shapeTetraEffect.reset();
		m_modelEffects.clear();
		m_modelResources.reset();
		m_sprites.reset();
		m_resourceDescriptors.reset();
		m_states.reset();
		m_graphicsMemory.reset();
}

void SceneRenderer::OnDeviceRestored()
{
	//m_cd3d12TetraRenderer->CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
	CreateDeviceDependentResources();
	//m_cd3d12GridRenderer->CreateDeviceDependentResources();
	//m_fpsTextRenderer->CreateDeviceDependentResources();
	
}

void AppXamlDX12::SceneRenderer::MouseCursorRender(float positionX, float positionY)
{
	if (m_tracking)
	{
		if (m_bLButtonDown || m_bRButtonDown)
		{
			D3D12_RECT rect = m_sceneDeviceResources->GetScissorRect();
			// convert mouse points to number line 
			// plus/ minus coordinates
			// and convert to float
			float x = 0.0f;
			float y = 0.0f;
			float z = 0.0f;
			int centerx;
			int centery;
			// convert mouse points to number line 
			// plus/ minus coordinates
			// and convert to float

			if (m_loadingComplete) {
				if (positionX > 0 || point.x < (rect.left - rect.right)
					|| point.y > 0 || point.y < (rect.bottom - rect.top))
				{
					centerx = (rect.right - rect.left) / 2;
					centery = (rect.bottom - rect.top) / 2;
					x = (float)((point.x - centerx) / 7.5f);
					y = (float)-((point.y - centery) / 7.5f);
				}
			}
			/*
			else {
				centerx = (rect.right - rect.left) / 2;
				centery = (rect.bottom - rect.top) / 2;
				x = (float)(((point.x) - centerx) / (centerx / 28.0f));
				y = (float)-(((point.y) - centery) / (centery / 21.0f));
			}
			*/
			XMFLOAT3 vCursor3dPos = XMFLOAT3(x, y, 0.0f);


			XMVECTOR xx, yy, zz;
			XMVECTOR m_vMouse3dPos;
#if defined(_XM_ARM_NEON_INTRINSICS_)
			XMVECTOR m_vXAxis;
			m_vXAxis.n128_f32[0] = 0.0f; m_vXAxis.n128_f32[1] = 0.0f; m_vXAxis.n128_f32[2] = 0.0f;
			xx = XMVectorScale(m_vXAxis, x);
			XMVECTOR m_vYAxis;
			m_vYAxis.n128_f32[0] = 0.0f; m_vYAxis.n128_f32[1] = 0.0f; m_vYAxis.n128_f32[2] = 0.0f;
			yy = XMVectorScale(m_vYAxis, y);
			XMVECTOR m_vZAxis;
			m_vZAxis.n128_f32[0] = 0.0f; m_vZAxis.n128_f32[1] = 0.0f; m_vZAxis.n128_f32[2] = 0.0f;
			zz = XMVectorScale(m_vZAxis, z);
			m_vMouse3dPos.n128_f32[0] = ((XMVectorGetX(yy) - XMVectorGetX(xx)) + m_LookAtX);
			m_vMouse3dPos.n128_f32[1] = ((XMVectorGetY(yy) - XMVectorGetY(xx)) + m_LookAtY);
			m_vMouse3dPos.n128_f32[2] = ((XMVectorGetZ(yy) - XMVectorGetZ(xx)) + m_LookAtZ);
		

#endif
#if defined(_XM_SSE_INTRINSICS_)
			FXMVECTOR m_vXAxis = { 0.0f, 0.0f, 0.0f };
			xx = XMVectorScale(m_vXAxis, x);
			FXMVECTOR m_vYAxis = { 0.0f, 0.0f, 0.0f };
			yy = XMVectorScale(m_vYAxis, y);
			FXMVECTOR m_vZAxis = { 0.0f, 0.0f, 0.0f };
			zz = XMVectorScale(m_vZAxis, z);
			m_vMouse3dPos = { ((XMVectorGetX(yy) - XMVectorGetX(xx)) + m_LookAtX), ((XMVectorGetY(yy) - XMVectorGetY(xx)) + m_LookAtY), ((XMVectorGetZ(yy) - XMVectorGetZ(xx)) + m_LookAtZ) };

#endif
			
			if (m_bLButtonDown == true)
			{
				DrawPointsOne(m_vMouse3dPos, positionX, positionY);
				m_posX = XMVectorGetX(m_vMouse3dPos);
				m_posY = XMVectorGetY(m_vMouse3dPos);
				m_posZ = XMVectorGetZ(m_vMouse3dPos);
				OutputDebugString(L"Inside m_bLButtonDown\n");
			}

			//m_Cursor3d->RenderCursor3d(m_d3dDevice, matView, vCursor3dPos);
			// Turn on mouse cursor.
				// This disables relative mouse movement events.
			//CoreWindow::GetForCurrentThread()->PointerCursor = ref new CoreCursor(CoreCursorType::Cross, 1);

			ViewMatrix(m_view4x4, L"m_view");

			ViewMatrix(m_projection4x4, L"m_projection");
			//XMMatrixTransformation(D3DTS_WORLD, &matWorld);

			ViewMatrix(m_world4x4, L"m_world");

			//m_bRotating = false;
			////////////////////////////////////////..
			//XMMatrixTranslation(vFromPt.x, vFromPt.y, vFromPt.z);
			//m_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);
			//m_Camera->DrawCameraMesh(m_pd3dDevice, matView, vFromPt);
			// End the scene.
			//m_d3dDevice->EndScene();
		} // eo mouseButtonDown if
	}// eo m_tracking if
}

void AppXamlDX12::SceneRenderer::DrawPointsOne(XMVECTOR intersect, float positiontX, float positionY)
{
	if (m_bLButtonDown == true)
	{
		if (m_fPointSpace > 0.0F) {

			if (m_fScrollDist > 0.0F) {
				if (checkDistance(XMVectorGetX(intersect), XMVectorGetY(intersect), XMVectorGetZ(intersect), m_fScrollDist))
				{
					
					posX[m_iPointCount] = XMVectorGetX(intersect);
					posY[m_iPointCount] = XMVectorGetY(intersect);
					posZ[m_iPointCount] = XMVectorGetZ(intersect);

					m_iTempGroup[m_iTempGroupCount] = m_iPointCount;
					m_iTempMouseX[m_iTempGroupCount] = point.x;
					m_iTempMouseY[m_iTempGroupCount] = point.y;
					
					m_iTempGroupCount++;
					m_iPointCount++;
					
				}//eo if(checkDistance(intersect.x, intersect.y, intersect.zz, m_fScrollDist)==true)
			}// eo if(m_fScrollDist>0.0F)
			else
			{
				posX[m_iPointCount] = XMVectorGetX(intersect);
				posY[m_iPointCount] = XMVectorGetY(intersect);
				posZ[m_iPointCount] = XMVectorGetZ(intersect);

				m_iTempGroup[m_iTempGroupCount] = m_iPointCount;
				m_iTempMouseX[m_iTempGroupCount] = point.x;
				m_iTempMouseY[m_iTempGroupCount] = point.y;
				m_iTempGroupCount++;
				m_iPointCount++;
				
			}// eo else if(m_fScrollDist>0.0F)

			//Render();

		//}//eoif(SUCCEEDED(visual->Q

			pSect = XMFLOAT3(XMVectorGetX(intersect), XMVectorGetY(intersect), XMVectorGetZ(intersect));

		}// eo if(m_bLButtonDown==true){		
	}
}


void AppXamlDX12::SceneRenderer::Initialize()
{
	//----------------------------------------------------------------------

		//_In_ MoveLookController ^ controller,
		//_In_ GraphicRenderer ^ renderer
		
	
		// This method is expected to be called as an asynchronous task.
		// Care should be taken to not call rendering methods on the
		// m_renderer as this would result in the D3D Context being
		// used in multiple threads, which is not allowed.

		//m_controller = controller;
		//m_renderer = renderer;

		m_audioController = ref new Audio;
		m_audioController->CreateDeviceIndependentResources();
		Tetras tetras[1000] = {};
		for (int i = 0; i < 1000; i++)
		{
			tetras[i].m_shapeTetra = GeometricPrimitive::CreateTetrahedron(1.F);
		};
		//m_objects = std::vector<GraphicObject^>();
		//m_renderObjects = std::vector<GraphicObject^>();
		//m_level = std::vector<Level^>();

		//m_savedState = ref new PersistentState();
		//m_savedState->Initialize(ApplicationData::Current->LocalSettings->Values, "Graphic");

		StepTimer m_timer;// = new StepTimer();

		// Create a sphere primitive to represent the player.
		// The sphere will be used to handle collisions and constrain the player in the world.
		// It is not rendered so it is not added to the list of render objects.
		// It is added to the object list so it will be included in intersection calculations.
		//m_artistCamera = GeometricPrimitive::CreateSphere(1.0f);// XMFLOAT3(0.0f, -1.3f, 4.0f), 0.2f);
		//m_objects.push_back(m_artistDraw);
		//m_artistCamera->Active(true);

		m_camera = ref new Hot3dxCamera();
		m_camera->SetProjParams(XM_PI / 2, 1.0f, 0.01f, 100.0f);
		m_camera->SetViewParams(
			//m_artistCamera->Position(),            // Eye point in world coordinates.
			XMFLOAT3(m_EyeX, m_EyeY, m_EyeZ),
			XMFLOAT3(m_LookAtX, m_LookAtY, m_LookAtZ),     // Look at point in world coordinates.
			XMFLOAT3(m_UpX, m_UpY, m_UpZ)      // The Up vector for the camera.
			);

		//m_controller->Pitch(m_camera->Pitch());
		//m_controller->Yaw(m_camera->Yaw());

		// Instantiate a set of primitives to represent the containing world. These objects
		// maintain the geometry and material properties of the walls, floor and ceiling.
		// The TargetId is used to identify the world objects so that the right geometry
		// and textures can be associated with them later after those resources have
		// been created.
		//GraphicObject^ world = ref new GraphicObject();
		//world->TargetId(GraphicConstants::WorldFloorId);
		//world->Active(true);
		//m_renderObjects.push_back(world);

		//world = ref new GraphicObject();
		//world->TargetId(GraphicConstants::WorldCeilingId);
		//world->Active(true);
		//m_renderObjects.push_back(world);

		//world = ref new GraphicObject();
		//world->TargetId(GraphicConstants::WorldWallsId);
		//world->Active(true);
		//m_renderObjects.push_back(world);

		// Min and max Bound are defining the world space of the Graphic.
		// All camera motion and dynamics are confined to this space.
		//m_minBound = XMFLOAT3(-4.0f, -3.0f, -6.0f);
		//m_maxBound = XMFLOAT3(4.0f, 3.0f, 6.0f);

		/* Instantiate the Cylinders for use in the various Graphic levels.
		// Each cylinder has a different initial position, radius and direction vector,
		// but share a common set of material properties.
		for (int a = 0; a < GraphicConstants::MaxCylinders; a++)
		{
			Cylinder^ cylinder;
			switch (a)
			{
			case 0:
				cylinder = ref new Cylinder(XMFLOAT3(-2.0f, -3.0f, 0.0f), 0.25f, XMFLOAT3(0.0f, 6.0f, 0.0f));
				break;
			case 1:
				cylinder = ref new Cylinder(XMFLOAT3(2.0f, -3.0f, 0.0f), 0.25f, XMFLOAT3(0.0f, 6.0f, 0.0f));
				break;
			case 2:
				cylinder = ref new Cylinder(XMFLOAT3(0.0f, -3.0f, -2.0f), 0.25f, XMFLOAT3(0.0f, 6.0f, 0.0f));
				break;
			case 3:
				cylinder = ref new Cylinder(XMFLOAT3(-1.5f, -3.0f, -4.0f), 0.25f, XMFLOAT3(0.0f, 6.0f, 0.0f));
				break;
			case 4:
				cylinder = ref new Cylinder(XMFLOAT3(1.5f, -3.0f, -4.0f), 0.50f, XMFLOAT3(0.0f, 6.0f, 0.0f));
				break;
			}
			cylinder->Active(true);
			m_objects.push_back(cylinder);
			m_renderObjects.push_back(cylinder);
		}

		MediaReader^ mediaReader = ref new MediaReader;
		auto targetHitSound = mediaReader->LoadMedia("Assets\\hit.wav");
		*/
		// Instantiate the targets for use in the Graphic.
		// Each target has a different initial position, size and orientation,
		// but share a common set of material properties.
		// The target is defined by a position and two vectors that define both
		// the plane of the target in world space and the size of the parallelogram
		// based on the lengths of the vectors.
		// Each target is assigned a number for identification purposes.
		// The Target ID number is 1 based.
		/* All targets have the same material properties.
		for (int a = 1; a < GraphicConstants::MaxTargets; a++)
		{
			m_^ target;
			switch (a)
			{
			case 1:
				target = ref new Face(XMFLOAT3(-2.5f, -1.0f, -1.5f), XMFLOAT3(-1.5f, -1.0f, -2.0f), XMFLOAT3(-2.5f, 1.0f, -1.5f));
				break;
			
		}
		
			target->Target(true);
			target->TargetId(a);
			target->Active(true);
			target->HitSound(ref new SoundEffect());
			target->HitSound()->Initialize(
				m_audioController->SoundEffectEngine(),
				mediaReader->GetOutputWaveFormatEx(),
				targetHitSound
				);

			m_objects.push_back(target);
			m_renderObjects.push_back(target);
		}
		*/
		// Instantiate a set of spheres to be used as ammunition for the Graphic
		// and set the material properties of the spheres.
		auto ammoHitSound = mediaReader->LoadMedia("Assets\\bounce.wav");

		for (unsigned int a = 0; a < m_iPointCount; a++)
		{
			m_tetraPoints[a].m_shapeTetra = GeometricPrimitive::CreateTetrahedron(0.1f);//ref new Sphere;
			//m_tetraPoints[a]->Radius(1.0f);
			//m_tetraPoints[a]->HitSound(ref new SoundEffect());
			//m_tetraPoints[a]->HitSound()->Initialize(
			//	m_audioController->SoundEffectEngine(),
			//	mediaReader->GetOutputWaveFormatEx(),
			//	ammoHitSound
			//	);
			//m_ammo[a]->Active(false);
			//m_renderObjects.push_back(m_ammo[a]);
		}
		
		

		// Load the currentScore for saved state if it exists.
		LoadState();

		//m_controller->Active(false);
	
}

void AppXamlDX12::SceneRenderer::OnLButtonDown(UINT nFlags, XMFLOAT2 point)
{
	/*
	if (point.x > 3 && point.x < 345 && point.y>3 && point.y < 345)
	{
		SetCursor(AfxGetApp()->LoadCursor(IDC_CURSOR1));
		if (m_pSceneObjects->m_bLinearAcross == true)
			curposxy.y = point.y;
		if (m_pSceneObjects->m_bLinearUp == true)
			curposxy.x = point.x;

		m_bLButtonDown = true;
		if (m_iDrawMode == 2)
		{
			point.y -= 2;
			//SelectVertex(nFlags, point);
			//XMFLOAT intersect = SelectGridFace(nFlags, point);
			//DrawPointsOne(intersect, point);
			//DrawPointsOne(SelectGridFace(nFlags, point), point);
		}
		else
		{
			//Select(nFlags, point);

		}
	}
	*/
}

void AppXamlDX12::SceneRenderer::OnRightButtonDown(UINT nFlags, XMFLOAT2 point)
{
	/*
	if (point.x > 3 && point.x < 345 && point.y>3 && point.y < 345)
	{
		SetCursor(AfxGetApp()->LoadCursor(IDC_CURSOR1));
		if (m_pSceneObjects->m_bLinearAcross == true)
			curposxy.y = point.y;
		if (m_pSceneObjects->m_bLinearUp == true)
			curposxy.x = point.x;

		m_bLButtonDown = true;
		if (m_iDrawMode == 2)
		{
			point.y -= 2;
			//SelectVertex(nFlags, point);
			//XMFLOAT intersect = SelectGridFace(nFlags, point);
			//DrawPointsOne(intersect, point);
			//DrawPointsOne(SelectGridFace(nFlags, point), point);
		}
		else
		{
			//Select(nFlags, point);

		}
	}
	*/

}

void AppXamlDX12::SceneRenderer::OnMouseMove(UINT nFlags, XMFLOAT2 point)
{
	/*
	if (point.x>3 && point.x<345 && point.y>3 && point.y<345)
	{
		//SetCursor(AfxGetApp()->LoadCursor(IDC_CURSOR1));

		XMFLOAT3 intersect = SelectGridFace(nFlags, point);


		if (m_pSceneObjects->m_bLinearAcross == true)
			point.y = curposxy.y;
		if (m_pSceneObjects->m_bLinearUp == true)
			point.x = curposxy.x;
		if (m_iDrawMode == 2)
		{
			//DrawPointsOne(intersect, point);
		}
		else if (m_iDrawMode == 3)
		{
			if (m_bLButtonDown == true)
			{
				//CD3DFrame* pScene;
				CD3D9Camera* pCamera;

				HOTD3D9RAY rmRay;
				XMFLOAT4 v4Src;

				pCamera = m_Camera;// -> > GetCamera(&pCamera);
				rmRay.dvPos = m_pSceneObjects->m_CamPos;
				//pCamera->GetScene(&pScene);
				//pCamera->Release();

				v4Src.x = (float)(point.x);
				v4Src.y = (float)(point.y);
				v4Src.z = 0.0f;
				v4Src.w = 1.0f;



				XMMatrix matrixOut;
				FLOAT scaling = 1.0F;
				XMFLOAT3 centerRotate;
				D3DXQUATERNION quat;
				XMFLOAT3 pv;
				pv.x = quat.w = v4Src.w;
				pv.y = quat.x = v4Src.x;
				pv.z = quat.y = v4Src.y;
				quat.z = v4Src.z;
				//XMFLOAT3 translation;
				// InverseTransform is XMMatrixAffineTransformation(
				// XMMatrix* out, float scaling, XMFLOAT3* centerofrotation, D3DXQuaternion* rotation, XMFLOAT3* Translation)
				//XMMatrix* matrixOut1 = XMMatrixAffineTransformation(&matrixOut, scaling, &rmRay.dvDir, &quat, &translation);
				//XMFLOAT3* translation;
				XMMatrix outInverse;
				FLOAT pDeterminant;
				XMMatrixInverse(&outInverse, &pDeterminant, &matWorld);
				D3DXVec3TransformCoord(&rmRay.dvDir, &pv, &outInverse);

				XMFLOAT3 dvCamPos = rmRay.dvPos;

				CD3DFrame* pFrame3 = m_pSceneObjects->gridFrame;

				CD3DMesh* mesh = m_pSceneObjects->grid;

				XMFLOAT3 pos = rmRay.dvPos;
				//m_pSceneObjects->m_view->InverseTransform(&pos, &v4Src);
				//matrixOut1 = XMMatrixAffineTransformation(&matrixOut, scaling, &pos, &quat, &translation);
				//XMFLOAT3* translation;
				//XMMatrix outInverse;
				//FLOAT pDeterminant;
				XMMatrixInverse(&outInverse, &pDeterminant, &matWorld);
				D3DXVec3TransformCoord(&rmRay.dvDir, &pos, &outInverse);

				if (GetD3D9Device()) {

				}
				else {
					//SetD3D9Device(GetD3DD3vice9());
				}
				//m_pd3dDevice->CreateFace(&fc);
				CD3D9Face fc;
				XMFLOAT3 v1, v2, v3;

				v1 = m_pSceneObjects->grid->pVerts[0].p;
				v2 = m_pSceneObjects->grid->pVerts[8].p;
				v3 = m_pSceneObjects->grid->pVerts[80].p;
				fc.p1 = v1;
				fc.p2 = v2;
				fc.p3 = v3;

				int fvCnt = 3;
				double distxyz = 0.0;
				CHot3dxD3D12Geometry geo;
				XMFLOAT3 g = geo.CenterOfFace(fc);
				XMFLOAT4 g4;
				g4.x = g.x;
				g4.y = g.y;
				g4.z = g.z;
				g4.w = 0.0F;
				XMFLOAT3 faceCenter;
				//transformtoworldcoord
				//pFrame3->Transform(&faceCenter, &g);
				D3DXVec3Transform(&g4, &faceCenter, NULL);
				XMFLOAT3 faceCenterDir = geo.directionBetweenPoints(pos, faceCenter);

				double camToFaceCenterDist = geo.distanceBetweenPoints(dvCamPos, faceCenter);
				XMFLOAT camToMouseDir = geo.directionBetweenPoints(dvCamPos, pos);
				double camToMouseDist = geo.distanceBetweenPoints(dvCamPos, pos);
				double faceDirDist = geo.distanceBetweenPoints(faceCenter, faceCenterDir);

				XMFLOAT camToMouseDir2;
				camToMouseDir2 = camToMouseDir;
				camToMouseDir2.x *= 1000.F;
				camToMouseDir2.y *= 1000.F;
				camToMouseDir2.z *= 1000.F;
				XMFLOAT3 intersect, f1, f2, f3;
				f1 = geo.GetFaceVertex(fc, 0);
				//pFrame3->SetPosition(Transform(&f1, &f1);
				f2 = geo.GetFaceVertex(fc, 1);
				//pFrame3->Transform(&f2, &f2);
				f3 = geo.GetFaceVertex(fc, 2);
				//pFrame3->Transform(&f3, &f3);
				XMFLOAT pn;
				pn = geo.FindPlaneNormal(f1, f2, f3);
				double t = geo.FindPlaneConstant(pn, f2);
				intersect = geo.FindPointIntersectPlane(pn, dvCamPos, camToMouseDir2,
					(float)t);

				geo.~CHot3dxD3D12Geometry();
				if (m_fPointSpace>0.0F) {
					if(checkDistance(intersect.x, intersect.y, intersect.z, m_fPointSpace))
					{
					DWORD n = tempMeshBld->GetLocalMesh()->GetNumVertices();
					XMFLOAT3 p;
					D3DXVec3Subtract(&p, &pSect, &intersect);
					for (unsigned int i = 0; i<n; i++)
					{
						XMFLOAT3 s, v;
						s = tempMeshBld->pVerts[i].p;
						//m_pSceneObjects.gridFrame->Transform(&v, &s);
						tempMeshBld->pVerts[i].p.x = v.x - p.x;
						tempMeshBld->pVerts[i].p.y = v.y - p.y;
						tempMeshBld->pVerts[i].p.z = v.z - p.z;

					}
					pSect = intersect;
				}
			}
		}
	}
*/
}

// Renders one frame using the vertex and pixel shaders.
bool SceneRenderer::Render()
{
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!m_loadingComplete)
	{
		return false;
	}

	// Prepare the command list to render a new frame.
	m_sceneDeviceResources->Prepare();
	Clear();
	auto commandList = m_sceneDeviceResources->GetCommandList();
	PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");
	// Draw procedurally generated XZ dynamic grid
	const XMVECTORF32 xaxis = { 20.f, 0.f, 0.f };
	const XMVECTORF32 yaxis = { 0.f, 0.f, 20.f };
	DrawGrid(xaxis, yaxis, g_XMZero, 20, 20, Colors::Gray);
	// Draw procedurally generated XY dynamic grid
	const XMVECTORF32 xaxis1 = { 10.f, 0.f, 0.f };
	const XMVECTORF32 yaxis1 = { 0.f, 10.f, 0.f };
	const XMVECTORF32 zaxis1 = { 0.f, 0.f, 0.f };
	DrawGrid(xaxis1, yaxis1, zaxis1, 10, 10, Colors::Crimson);

	// Set the descriptor heaps
	ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptors->Heap(), m_states->Heap() };
	commandList->SetDescriptorHeaps(_countof(heaps), heaps);

	// Draw sprite
	PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Draw sprite");
	m_sprites->Begin(commandList);
	m_sprites->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::WindowsLogo), GetTextureSize(m_texture2.Get()),
		XMFLOAT2(1, 1));// 10, 75));
	m_font->DrawString(m_sprites.get(), L"Hot3dx AppXamlDX12 Dev Res", XMFLOAT2(100, 370), Colors::Yellow);
	m_smallFont->DrawString(m_sprites.get(), L"Hi from the redneck hovel", XMFLOAT2(100, 400), Colors::DarkSeaGreen);
	m_sprites->End();

	PIXEndEvent(commandList);
	// Draw 3D object
	PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Draw teapot");

	DirectX::XMMATRIX local = (XMLoadFloat4x4(&m_world4x4) * DirectX::XMMatrixTranslation(-2.f, -2.f, -10.f));
	//DirectX::XMMATRIX local = m_world * DirectX::XMMatrixTranslation(-2.f, -2.f, -10.f);
	m_shapeEffect->SetWorld(local);
	m_shapeEffect->Apply(commandList);
	m_shape->Draw(commandList);
	for (unsigned int i = 0; i < m_iPointCount; i++)
	{
		DirectX::XMMATRIX localTetra = (XMLoadFloat4x4(&m_world4x4) * DirectX::XMMatrixTranslation(posX[i], posY[i], posZ[i]));
		m_shapeTetraEffect->SetWorld(localTetra);
		m_shapeTetraEffect->Apply(commandList);
		m_shapeTetra->Draw(commandList);
	}
	PIXEndEvent(commandList);

	// Draw model
	PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Draw model");
	const XMVECTORF32 scale = { 0.01f, 0.01f, 0.01f };
	const XMVECTORF32 translate = { 3.f, -2.f, -10.f };
	// Old DirectX::SimpleMath::Quaternion::CreateFromYawPitchRoll(XM_PI / 2.f, 0.f, -XM_PI / 2.f);

	XMVECTOR angles;
#if defined(_XM_ARM_NEON_INTRINSICS_)
	angles.n128_f32[0] = XM_PI / 2.f; angles.n128_f32[0] = 0.f; angles.n128_f32[0] = -XM_PI / 2.f;
#endif
#if defined(_XM_SSE_INTRINSICS_)
	angles = { XM_PI / 2.f, 0.f, -XM_PI / 2.f };
#endif
	XMVECTOR rotate = DirectX::XMQuaternionRotationRollPitchYawFromVector(angles);
	// Old local = m_world * XMMatrixTransformation(g_XMZero, DirectX::SimpleMath::Quaternion::Identity, scale, g_XMZero, rotate, translate);
	local = XMLoadFloat4x4(&m_world4x4) * XMMatrixTransformation(g_XMZero, DirectX::XMQuaternionIdentity(), scale, g_XMZero, rotate, translate);
	Model::UpdateEffectMatrices(m_modelEffects, local, XMLoadFloat4x4(&m_view4x4), XMLoadFloat4x4(&m_projection4x4));
	heaps[0] = m_modelResources->Heap();
	commandList->SetDescriptorHeaps(_countof(heaps), heaps);
	m_model->Draw(commandList, m_modelEffects.begin());
	
	PIXEndEvent(commandList);
	
	PIXEndEvent(commandList);
	
	// Show the new frame.
	PIXBeginEvent(m_sceneDeviceResources->GetCommandQueue(), PIX_COLOR_DEFAULT, L"Present");
	
	m_sceneDeviceResources->Present();
	
	m_graphicsMemory->Commit(m_sceneDeviceResources->GetCommandQueue());
	
	PIXEndEvent(m_sceneDeviceResources->GetCommandQueue());
	
	return true;
}

// Helper method to clear the back buffers.
void SceneRenderer::Clear()
{
	auto commandList = m_sceneDeviceResources->GetCommandList();
	PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

	// Clear the views.
	auto rtvDescriptor = m_sceneDeviceResources->GetRenderTargetView();
	auto dsvDescriptor = m_sceneDeviceResources->GetDepthStencilView();

	commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
	commandList->ClearRenderTargetView(rtvDescriptor, Colors::CornflowerBlue, 0, nullptr);
	commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// Set the viewport and scissor rect.
	auto viewport = m_sceneDeviceResources->GetScreenViewport();
	auto scissorRect = m_sceneDeviceResources->GetScissorRect();
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);

	PIXEndEvent(commandList);
}


void XM_CALLCONV SceneRenderer::DrawGrid(FXMVECTOR xAxis, FXMVECTOR yAxis, FXMVECTOR origin, size_t xdivs, size_t ydivs, GXMVECTOR color)
{
	auto commandList = m_sceneDeviceResources->GetCommandList();
	PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Draw grid");

	m_lineEffect->Apply(commandList);

	m_batch->Begin(commandList);

	xdivs = std::max<size_t>(1, xdivs);
	ydivs = std::max<size_t>(1, ydivs);

	for (size_t i = 0; i <= xdivs; ++i)
	{
		float fPercent = float(i) / float(xdivs);
		fPercent = (fPercent * 2.0f) - 1.0f;
		XMVECTOR vScale = XMVectorScale(xAxis, fPercent);
		vScale = XMVectorAdd(vScale, origin);

		VertexPositionColor v1(XMVectorSubtract(vScale, yAxis), color);
		VertexPositionColor v2(XMVectorAdd(vScale, yAxis), color);
		m_batch->DrawLine(v1, v2);
	}

	for (size_t i = 0; i <= ydivs; i++)
	{
		float fPercent = float(i) / float(ydivs);
		fPercent = (fPercent * 2.0f) - 1.0f;
		XMVECTOR vScale = XMVectorScale(yAxis, fPercent);
		vScale = XMVectorAdd(vScale, origin);

		VertexPositionColor v1(XMVectorSubtract(vScale, xAxis), color);
		VertexPositionColor v2(XMVectorAdd(vScale, xAxis), color);
		m_batch->DrawLine(v1, v2);
	}

	m_batch->End();

	PIXEndEvent(commandList);
}

void AppXamlDX12::SceneRenderer::ViewMatrix(XMFLOAT4X4 m, TCHAR* str)
{
	
    XMMatrixSet(m._11, m._12, m._13, m._14,
	    m._21, m._22, m._23, m._24,
		m._31, m._32, m._33, m._34,
		m._41, m._42, m._43, m._44
		);


    wchar_t* t = {};
    //OutputDebugString(t);
    //OutputDebugString(str);

    wchar_t* s = { L"\nThe Matrix values: \n%s\n%.6f  %.6f  %.6f %.6f\n%.6f  %.6f  %.6f %.6f\n%.6f  %.6f  %.6f %.6f\n%.6f  %.6f  %.6f %.6f\n" };
    swprintf_s(t, 1000, s, str, m._11, m._12, m._13, m._14,
	     m._21, m._22, m._23, m._24,
	     m._31, m._32, m._33, m._34,
	     m._41, m._42, m._43, m._44);


     OutputDebugString(t);
}

void AppXamlDX12::SceneRenderer::RotatePitch(float degree)
{
	//m_camera->RotatePitch(degree);
}

void AppXamlDX12::SceneRenderer::RotateYaw(float degree)
{
	// m_camera->RotateYaw(degree);
}

void AppXamlDX12::SceneRenderer::ScreenMouse3DWorldAlignment()
{
	TCHAR dest[500] = {};;
	TCHAR* s = TEXT("%s\nW: %.6f H: %.6f \n");
	TCHAR* t = TEXT("The ratios:");

	StringCbPrintf(dest, 500, s, t, m_widthRatio, m_heightRatio);
	OutputDebugString(dest);
}
XMMATRIX XM_CALLCONV AppXamlDX12::SceneRenderer::SetXMMatrix(DirectX::XMFLOAT4X4 m, XMMATRIX xm)
{
	xm = XMMatrixSet(m._11, m._12, m._13, m._14,
		m._21, m._22, m._23, m._24,
		m._31, m._32, m._33, m._34,
		m._41, m._42, m._43, m._44
	);
	return xm;
}
XMMATRIX XM_CALLCONV AppXamlDX12::SceneRenderer::GetXMMatrix(DirectX::XMFLOAT4X4 m)
{
	return XMMatrixSet(m._11, m._12, m._13, m._14,
		m._21, m._22, m._23, m._24,
		m._31, m._32, m._33, m._34,
		m._41, m._42, m._43, m._44
	);
}
#pragma endregion

#pragma region Message Handlers
#pragma endregion

