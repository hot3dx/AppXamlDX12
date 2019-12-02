﻿#pragma once

#include "Common\StepTimer.h"
#include "Common\DeviceResources.h"
#include "Content\Sample3DSceneRenderer.h"
//#include "Content\SampleFpsTextRenderer.h"

// Renders Direct2D and 3D content on the screen.
namespace AppXamlDX12
{
	class AppXamlDX12Main : public DX::IDeviceNotify
	{
	public:
		AppXamlDX12Main(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		~AppXamlDX12Main();
		void CreateWindowSizeDependentResources();
		void Update();
		bool Render();

		void OnWindowSizeChanged();
		void OnSuspending();
		void OnResuming();
		void OnDeviceRemoved();
		void StartTracking() { m_sceneRenderer->StartTracking(); }
		void TrackingUpdate(float positionX) { m_pointerLocationX = positionX; }
		void StopTracking() { m_sceneRenderer->StopTracking(); }
		bool IsTracking() { return m_sceneRenderer->IsTracking(); }
		void StartRenderLoop();
		void StopRenderLoop();
		Concurrency::critical_section& GetCriticalSection() { return m_criticalSection; }

		// IDeviceNotify
		virtual void OnDeviceLost();
		virtual void OnDeviceRestored();

		// Accessors

		Sample3DSceneRenderer* GetSceneRenderer(){ return m_sceneRenderer.get();}


	private:
		

		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		// TODO: Replace with your own content renderers.
		std::unique_ptr<Sample3DSceneRenderer> m_sceneRenderer;
		//std::unique_ptr<SampleFpsTextRenderer> m_fpsTextRenderer;

		Windows::Foundation::IAsyncAction^ m_renderLoopWorker;
		Concurrency::critical_section m_criticalSection;

		// Rendering loop timer.
		DX::StepTimer m_timer;

		// Track current input pointer position.
		float m_pointerLocationX;
	};
}