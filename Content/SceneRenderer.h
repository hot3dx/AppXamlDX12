//--------------------------------------------------------------------------------------
// File: SceneRenderer.h
//
// Copyright (c) Jeff Kubitz - hot3dx. All rights reserved.
// 
//
//--------------------------------------------------------------------------------------
#pragma once

#include "..\Common\DeviceResources.h"
#include "..\Content\ShaderStructures.h"
#include "..\Common\StepTimer.h"
#include "..\Graphics\AlignedNew.h"
#include "..\Graphics\RenderTargetState.h"
#include "..\Graphics\EffectPipelineStateDescription.h"
#include "..\Graphics\CommonStates.h"
#include "..\Graphics\GraphicsMemory.h"
#include "..\Graphics\DescriptorHeap.h"
#include "..\Graphics\EffectCommon.h"
#include "..\Graphics\VertexTypes.h"
#include <SimpleMath.h>
#include "..\Graphics\Model.h"
#include "..\Graphics\PrimitiveBatch.h"
#include "..\Graphics\GeometricPrimitive.h"
#include "..\Graphics\SpriteBatch.h"
#include "..\Graphics\SpriteFont.h"
#include "..\Graphics\Hot3dxCamera.h"
#include "..\Audio\Audio.h"
#include "..\Audio\MediaReader.h"
#include "..\Graphics\Hot3dxGeometry.h"
#include <thread>
#include <chrono>
#include <assert.h>






namespace AppXamlDX12
{
	// This sample renderer instantiates a basic rendering pipeline.
	class SceneRenderer
	{
	public:
		SceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		~SceneRenderer();
		void CreateDeviceDependentResources();
		void CreateWindowSizeDependentResources();
		void Update(DX::StepTimer const& timer);
		bool Render();
		void Clear();

		void SaveState();

		void StartTracking();
		BOOL checkDistance(float x, float y, float z, float mouseMoveDistDelta);
		void TrackingUpdate(float positionX, float positionY);
		void StopTracking();
		bool IsTracking() { return m_tracking; }
		void ReleaseDeviceDependentResources();
		void OnDeviceLost();
		void OnDeviceRestored();
		//CCameraXYMoveRotation          m_CamXYMoveRotate;
		void MouseCursorRender(float positionX, float positionY);
		void DrawPointsOne(XMVECTOR intersect, float positiontX, float positionY);
		bool                           is3DVisible;

		bool GetLoadingComplete() { return m_loadingComplete; }
		void SetLoadingComplete(bool complete){ m_loadingComplete = complete; }
		// Indices into the application state map.
		Platform::String^ AngleKey = "Angle";
		Platform::String^ TrackingKey = "Tracking";
		

		XMFLOAT3 pSect;
		bool m_bFaceSelected;
		int m_iV;
		bool m_bLButtonDown;
		bool m_bRButtonDown;
		void Initialize();
		void OnLButtonDown(UINT nFlags, XMFLOAT2 point);
		void OnRightButtonDown(UINT nFlags, XMFLOAT2 point);
		bool m_bMouseMove;
		void OnMouseMove(UINT nFlags, XMFLOAT2 point);

		void ViewMatrix(XMMATRIX m, TCHAR* str);

		unsigned int m_iPointCount;
		unsigned int m_iTotalPointCount;
		unsigned int m_iLastPoint;
		XMFLOAT2 point;
		unsigned int m_iTempGroup[10000];
		XMFLOAT2 m_iTempMouse[10000];

		unsigned int m_iTempGroupCount;
		unsigned int m_iGroupCount;
		std::vector<XMFLOAT3> pos;

		float m_fScrollDist;
		float m_fPointSpace;
		int m_iScrollPointSetPos;
		float distX;
		float distY;
		float distZ;

		float Setrotx;
		float Setroty;
		float Setrotz;

		float m_widthRatio;
		float m_heightRatio;

	private:
		void LoadState();
		void Rotate(float radians);
		void XM_CALLCONV DrawGrid(FXMVECTOR xAxis, FXMVECTOR yAxis, FXMVECTOR origin, size_t xdivs, size_t ydivs, GXMVECTOR color);
		void ScreenMouse3DWorldAlignment();

	private:
		// Constant buffers must be 256-byte aligned.
		static const UINT c_alignedConstantBufferSize = (sizeof(ModelViewProjectionConstantBuffer) + 255) & ~255;

		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_sceneDeviceResources;

		// Rendering loop timer.
		DX::StepTimer                               m_timer;


		ModelViewProjectionConstantBuffer					m_constantBufferData;
		UINT8*                                              m_mappedConstantBuffer;
		UINT												m_cbvDescriptorSize;
		
		// Variables used with the rendering loop.
		bool	m_loadingComplete;
		float	m_radiansPerSecond;
		float	m_angle;
		bool	m_tracking;
		UINT    sceneVertexCount;
		// Track our independent input on a background worker thread.
		Windows::Foundation::IAsyncAction^ m_inputLoopWorker;
		//Windows::UI::Core::CoreIndependentInputSource^ m_coreInput;
		/////////////
		// DirectXTK Test objects.
		std::shared_ptr<DirectX::ResourceUploadBatch>                           mesourceUpload;
		std::unique_ptr<DirectX::GraphicsMemory>                                m_graphicsMemory;
		std::unique_ptr<DirectX::DescriptorHeap>                                m_resourceDescriptors;
		std::unique_ptr<DirectX::CommonStates>                                  m_states;
		std::unique_ptr<DirectX::BasicEffect>                                   m_lineEffect;
		std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>  m_batch;
		std::unique_ptr<DirectX::BasicEffect>                                   m_shapeEffect;
		std::unique_ptr<DirectX::BasicEffect>                                   m_artistCameraEffect;
		std::unique_ptr<DirectX::BasicEffect>                                   m_shapeTetraEffect;
		std::unique_ptr<DirectX::Model>                                         m_model;
		std::vector<std::shared_ptr<DirectX::IEffect>>                          m_modelEffects;
		std::unique_ptr<DirectX::EffectTextureFactory>                          m_modelResources;
		std::unique_ptr<DirectX::GeometricPrimitive>                            m_shape;
		std::unique_ptr<DirectX::GeometricPrimitive>                            m_artistCamera;
		std::unique_ptr<DirectX::GeometricPrimitive>                            m_shapeTetra;
		std::unique_ptr<DirectX::SpriteBatch>                                   m_sprites;
		std::unique_ptr<DirectX::SpriteFont>                                    m_smallFont;
		std::unique_ptr<DirectX::SpriteFont>                                    m_font;

		//std::unique_ptr<DirectX::AudioEngine>                                   m_audEngine;
		//std::unique_ptr<DirectX::WaveBank>                                      m_waveBank;
		//std::unique_ptr<DirectX::SoundEffect>                                   m_soundEffect;
		//std::unique_ptr<DirectX::SoundEffectInstance>                           m_effect1;
		//std::unique_ptr<DirectX::SoundEffectInstance>                           m_effect2;

		Microsoft::WRL::ComPtr<ID3D12Resource>                                  m_texture1;
		Microsoft::WRL::ComPtr<ID3D12Resource>                                  m_texture2; 
		
		
		//DirectX::SimpleMath::Matrix
		DirectX::XMMATRIX                                                        m_world;
		//DirectX::SimpleMath::Matrix  
		DirectX::XMMATRIX                                                        m_view;
		//DirectX::SimpleMath::Matrix   
			DirectX::XMMATRIX 	                                                 m_projection;

		// Descriptors
			enum Descriptors
			{
				WindowsLogo,
				SeaFloor,
				SegoeFont,
				Count = 256
     		};

			
			Hot3dxCamera^ m_camera;
			public:float m_EyeX;
			public:float m_EyeY;
			public:float m_EyeZ;
			public:float m_LookAtX;
			public:float m_LookAtY;
			public:float m_LookAtZ;
			public:float m_UpX;
			public:float m_UpY;
			public:float m_UpZ;
			Audio^ m_audioController;
			MediaReader^ mediaReader = ref new MediaReader();
			// Screen Draw Variables
			std::unique_ptr<CHot3dxD3D12Geometry> m_geo;
			struct Tetras
			{
				std::unique_ptr<DirectX::GeometricPrimitive>                   m_shapeTetra;
			};
			UINT32                                      m_tetraCount;
			UINT32                                      m_tetraNext;
			std::vector<Tetras>                                                m_tetraPoints;
			float m_posX;
			float m_posY;
			float m_posZ;

			float posX[10000];
			float posY[10000];
			float posZ[10000];

			
	};
}


