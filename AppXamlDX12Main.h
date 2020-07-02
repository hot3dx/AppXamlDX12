//--------------------------------------------------------------------------------------
// File: 
//
// Copyright (c) Jeff Kubitz - hot3dx. All rights reserved.
// 
//
//--------------------------------------------------------------------------------------

#pragma once

#include "Common\StepTimer.h"
#include "Common\DeviceResources.h"
#include "Content\SceneRenderer.h"
#include "Input\Hot3dxController.h"

// Renders Direct2D and 3D content on the screen.
namespace AppXamlDX12
{
	class AppXamlDX12Main : public DX::IDeviceNotify
	{
	public:
		AppXamlDX12Main(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		~AppXamlDX12Main();
		void CreateWindowSizeDependentResources();
		void CreateDeviceDependentResources();
		void Update();
		bool Render();
		void Clear();
		void OnSuspending();
		void OnWindowSizeChanged();
		void ValidateDevice();
		void OnResuming();
		void OnDeviceRemoved();
		void StartTracking() { m_sceneRenderer->StartTracking();}
		void TrackingUpdate(float positionX, float positionY) { m_pointerLocationX = positionX; m_pointerLocationY = positionY; }
		void StopTracking() { m_sceneRenderer->StopTracking();}
        bool IsTracking() { return m_sceneRenderer->IsTracking();}
		void StartRenderLoop();
		void StopRenderLoop();
		Concurrency::critical_section& GetCriticalSection() { return m_criticalSection; }

		// IDeviceNotify
		virtual void OnDeviceLost();
		virtual void OnDeviceRestored();

		// Accessors

		SceneRenderer* GetSceneRenderer(){ return m_sceneRenderer.get();}
		void SetSceneRenderer() { m_sceneRenderer = std::unique_ptr<SceneRenderer>(new SceneRenderer(m_deviceResources)); }
		//CD3D12GridXaml* GetCD3D12GridRenderer() { return m_cd3d12GridRenderer.get(); }

		void PauseRequested() {
			m_timer.Stop();
			m_pauseRequested = true; };
		void PauseEnded() {// if (m_updateState == AppXamlDX12::UpdateEngineState::WaitingForPress) 
			m_pressComplete = true; };

		void WindowActivationChanged(Windows::UI::Core::CoreWindowActivationState activationState);

		void KeyDown(Windows::System::VirtualKey key);
		void KeyUp(Windows::System::VirtualKey key);

	private:
		// Process all input from the user before updating game state
		void ProcessInput();
		
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		// TODO: Replace with your own content renderers.
		std::unique_ptr<SceneRenderer> m_sceneRenderer;
		
		Windows::Foundation::IAsyncAction^ m_renderLoopWorker;
		Concurrency::critical_section m_criticalSection;

		// Rendering loop timer.
		DX::StepTimer m_timer;

		///////////////////////////////////////////////////
		bool                                                m_pauseRequested;
		bool                                                m_pressComplete;
		bool                                                m_renderNeeded;
		bool                                                m_haveFocus;
		
		Hot3dxController^                                   m_controller;

		///////////////////////////////////////////////////
		
		// Track current input pointer position.
		float m_pointerLocationX;
		float m_pointerLocationY;
	};
}