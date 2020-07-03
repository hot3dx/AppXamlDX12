//--------------------------------------------------------------------------------------
// File: 
//
// Copyright (c) Jeff Kubitz - hot3dx. All rights reserved.
// 
//
//--------------------------------------------------------------------------------------
#pragma once

#include "DirectXHelper.h"
#include <dwrite_3.h>
namespace DX
{



	// Provides an interface for an application that owns DeviceResources to be notified of the device being lost or created.
	interface IDeviceNotify
	{
		virtual void OnDeviceLost() = 0;
		virtual void OnDeviceRestored() = 0;
	};

	//static const UINT c_frameCount = 3;		// Use triple buffering.

	// Controls all the DirectX device resources.
	class DeviceResources
	{
	public:
		static const unsigned int c_AllowTearing = 0x1;
		static const unsigned int c_EnableHDR = 0x2;

		DeviceResources(DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM,
			DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT,
			UINT backBufferCount = 2,
			D3D_FEATURE_LEVEL minFeatureLevel = D3D_FEATURE_LEVEL_11_0,
			unsigned int flags = 0) noexcept(false);
		void CreateDeviceIndependentResources();
		void CreateDeviceResources();
		void CreateWindowSizeDependentResources();

		void SetSwapChainPanel(Windows::UI::Xaml::Controls::SwapChainPanel^ panel);
		bool WindowSizeChanged(int width, int height, DXGI_MODE_ROTATION rotation);
		void SetLogicalSize(Windows::Foundation::Size logicalSize);
		void SetCurrentOrientation(Windows::Graphics::Display::DisplayOrientations currentOrientation);
		void SetDpi(float dpi);
		void ValidateDevice();
		void HandleDeviceLost();
		void Prepare(D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_PRESENT);
		void Present(D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_RENDER_TARGET);
		void WaitForGpu();
		void SetCompositionScale(float compositionScaleX, float compositionScaleY);
		void Trim();
		void RegisterDeviceNotify(IDeviceNotify* deviceNotify);
		DXGI_MODE_ROTATION GetRotation() const { return m_rotation; }

		// The size of the render target, in pixels.
		Windows::Foundation::Size	GetOutputSize() const { return m_outputSize; }

		// The size of the render target, in dips.
		Windows::Foundation::Size	GetLogicalSize() const { return m_logicalSize; }

		float						GetDpi() const { return m_effectiveDpi; }
		bool						IsDeviceRemoved() const { return m_deviceRemoved; }

		// D3D Accessors.
		ID3D12Device* GetD3DDevice() const { return m_d3dDevice.Get(); }
		IDXGISwapChain3* GetSwapChain() const { return m_swapChain.Get(); }
		ID3D12Resource* GetRenderTarget() const { return m_renderTargets[m_backBufferIndex].Get(); }
		ID3D12Resource* GetDepthStencil() const { return m_depthStencil.Get(); }
		ID3D12CommandQueue* GetCommandQueue() const { return m_commandQueue.Get(); }
		ID3D12CommandAllocator* GetCommandAllocator() const { return m_commandAllocators[m_backBufferIndex].Get(); }
		ID3D12GraphicsCommandList* GetCommandList() const { return m_commandList.Get(); }
		DXGI_FORMAT					GetBackBufferFormat() const { return m_backBufferFormat; }
		DXGI_FORMAT					GetDepthBufferFormat() const { return m_depthBufferFormat; }
		D3D12_VIEWPORT				GetScreenViewport() const { return m_screenViewport; }
		DirectX::XMFLOAT4X4         GetOrientationTransform3D() const { return m_orientationTransform3D; }
		UINT						GetCurrentFrameIndex() const { return m_backBufferIndex; }
		UINT                        GetBackBufferCount() const { return m_backBufferCount; }
		Windows::UI::Xaml::Controls::SwapChainPanel^ GetSwapChainPanel() const { return m_swapChainPanel; }
		void                        GetCreateDeviceResources() { CreateDeviceResources(); }
		ID2D1DeviceContext* GetD3DDeviceContext() { return m_d2dContext.Get(); }
		ID3D12DescriptorHeap* GetRtvHeap() { return m_rtvHeap.Get(); }
		IDXGIFactory4* GetDXGIFactory() const { return m_dxgiFactory.Get(); }
		D3D_FEATURE_LEVEL           GetDeviceFeatureLevel() const { return m_d3dFeatureLevel; }
		DXGI_COLOR_SPACE_TYPE       GetColorSpace() const { return m_colorSpace; }
		unsigned int                GetDeviceOptions() const { return m_options; }
		D3D12_RECT                  GetScissorRect() const { return m_scissorRect; }
		Platform::Agile<Windows::UI::Core::CoreWindow^> GetCoreWindow() { return m_window; }
		void                        SetCoreWindow(Windows::UI::Core::CoreWindow^ window){m_window = window; }
		void                        SetScissorRect(LONG x, LONG y, LONG width, LONG height){
			m_scissorRect.left = x; m_scissorRect.top; m_scissorRect.right = width; m_scissorRect.bottom = height;
		}
		CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() const
		{
			return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(m_backBufferIndex), m_rtvDescriptorSize);
		}
		CD3DX12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const
		{
			return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
		}

		bool                                        m_isSwapPanelVisible;


	private:

		void UpdateRenderTargetSize();
		void MoveToNextFrame();
		DXGI_MODE_ROTATION ComputeDisplayRotation();
		void GetHardwareAdapter(IDXGIAdapter1** ppAdapter);
		void UpdateColorSpace();


		//UINT											m_currentFrame;
		static const size_t MAX_BACK_BUFFER_COUNT = 3;

		UINT                                                m_backBufferIndex;

		// Direct3D objects.
		Microsoft::WRL::ComPtr<ID3D12Device>			m_d3dDevice;
		Microsoft::WRL::ComPtr<IDXGIFactory4>			m_dxgiFactory;
		Microsoft::WRL::ComPtr<IDXGIFactory5>			m_dxgiFactory5;
		Microsoft::WRL::ComPtr<IDXGISwapChain3>			m_swapChain;

		Microsoft::WRL::ComPtr<ID3D12Resource>			m_renderTargets[MAX_BACK_BUFFER_COUNT];

		Microsoft::WRL::ComPtr<ID3D12Resource>			m_depthStencil;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>	m_rtvHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>	m_dsvHeap;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue>		m_commandQueue;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator>	m_commandAllocators[MAX_BACK_BUFFER_COUNT];
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   m_commandList;
		DXGI_FORMAT										m_backBufferFormat;
		DXGI_FORMAT										m_depthBufferFormat;
		UINT                                            m_backBufferCount;
		D3D12_VIEWPORT									m_screenViewport;
		D3D12_RECT                                      m_scissorRect;
		UINT											m_rtvDescriptorSize;
		bool											m_deviceRemoved;
		DWORD                                           m_dxgiFactoryFlags;

		// CPU/GPU Synchronization.
		Microsoft::WRL::ComPtr<ID3D12Fence>				m_fence;
		UINT64											m_fenceValues[MAX_BACK_BUFFER_COUNT];
		//UINT64											m_fenceValue;
		HANDLE											m_fenceEvent;
		//Microsoft::WRL::Wrappers::Event                 m_fenceEvent;
		
		// Cached reference to the XAML panel.
		Windows::UI::Xaml::Controls::SwapChainPanel^ m_swapChainPanel;

		// Cached reference to the Window.
		Platform::Agile<Windows::UI::Core::CoreWindow^>	m_window;

		Microsoft::WRL::ComPtr<ID2D1Factory3>           m_d2dFactory;
		Microsoft::WRL::ComPtr<ID2D1Device1>            m_d2dDevice1;
		Microsoft::WRL::ComPtr<ID2D1Device2>		    m_d2dDevice;
		Microsoft::WRL::ComPtr<ID2D1Bitmap1>            m_d2dTargetBitmap;
		Microsoft::WRL::ComPtr<ID2D1Bitmap1>            m_d2dTargetBitmapRight;
		Microsoft::WRL::ComPtr<ID2D1DeviceContext>      m_d2dContext;

		// DirectWrite drawing components.
		Microsoft::WRL::ComPtr<IDWriteFactory2>         m_dwriteFactory2;
		Microsoft::WRL::ComPtr<IDWriteFactory3>         m_dwriteFactory;
		Microsoft::WRL::ComPtr<IWICImagingFactory2>     m_wicFactory;

		// Cached device properties.
		Windows::Foundation::Size						m_d3dRenderTargetSize;
		Windows::Foundation::Size						m_outputSize;
		Windows::Foundation::Size						m_logicalSize;
		Windows::Graphics::Display::DisplayOrientations	m_nativeOrientation;
		Windows::Graphics::Display::DisplayOrientations	m_currentOrientation;
		float											m_dpi;
		float											m_compositionScaleX;
		float											m_compositionScaleY;
		float											m_effectiveCompositionScaleX;
		float											m_effectiveCompositionScaleY;
		bool                                            m_stereoEnabled;
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>    m_textBrush;
		Microsoft::WRL::ComPtr<IDWriteTextFormat>       m_textFormat;
		Microsoft::WRL::ComPtr<IDWriteTextFormat>       m_smallTextFormat;

		D3D_FEATURE_LEVEL                                   m_d3dFeatureLevel;
		D3D_FEATURE_LEVEL                                   m_d3dMinFeatureLevel;
		DXGI_MODE_ROTATION                                  m_rotation;
		RECT                                                m_outputSizeRect;

		// HDR Support
		DXGI_COLOR_SPACE_TYPE                               m_colorSpace;

		// DeviceResources options (see flags above)
		unsigned int                                        m_options;

		// This is the DPI that will be reported back to the app. It takes into account whether the app supports high resolution screens or not.
		float											m_effectiveDpi;

		// Transforms used for display orientation.
		D2D1::Matrix3x2F                                m_orientationTransform2D;
		DirectX::XMFLOAT4X4								m_orientationTransform3D;

		// The IDeviceNotify can be held directly as it owns the DeviceResources.
		IDeviceNotify* m_deviceNotify;

		

};
}
