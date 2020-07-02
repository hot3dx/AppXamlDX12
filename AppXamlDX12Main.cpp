//--------------------------------------------------------------------------------------
// File: 
//
// Copyright (c) Jeff Kubitz - hot3dx. All rights reserved.
// 
//
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "..\Common\DeviceResources.h"
#include "AppXamlDX12Main.h"
#include "..\Content\SceneRenderer.h"
#include "Common\DirectXHelper.h"
#include "Input\Hot3dxController.h"

using namespace AppXamlDX12;
using namespace Windows::Foundation;
using namespace Windows::System::Threading;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Concurrency;

// Loads and initializes application assets when the application is loaded.
AppXamlDX12Main::AppXamlDX12Main(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_deviceResources(deviceResources),
	m_pointerLocationX(0.0f),
	m_pointerLocationY(0.0f)
{
	// Register to be notified if the Device is lost or recreated
	m_deviceResources->RegisterDeviceNotify(this);

	SetSceneRenderer();
	
		//m_fpsTextRenderer = std::unique_ptr<SampleFpsTextRenderer>(new SampleFpsTextRenderer(m_deviceResources));
	
	
	// TODO: Change the timer settings if you want something other than the default variable timestep mode.
	// e.g. for 60 FPS fixed timestep update logic, call:
	/*
	m_timer.SetFixedTimeStep(true);
	m_timer.SetTargetElapsedSeconds(1.0 / 60);
	*/
	m_controller = ref new Hot3dxController(Window::Current->CoreWindow, m_deviceResources->GetSwapChainPanel()->Dispatcher);

	m_controller->AutoFire(false);
}

AppXamlDX12Main::~AppXamlDX12Main()
{
	// Deregister device notification
	m_deviceResources->RegisterDeviceNotify(nullptr);


}


// Updates application state when the window size changes (e.g. device orientation change)
void AppXamlDX12Main::CreateWindowSizeDependentResources()
{
	// Register event handlers for page lifecycle.
	
		// TODO: Replace this with the size-dependent initialization of your app's content.
	if (m_sceneRenderer->GetLoadingComplete() == false)return;
		m_sceneRenderer->CreateWindowSizeDependentResources();
	
}

void AppXamlDX12::AppXamlDX12Main::CreateDeviceDependentResources()
{
	//m_sceneRenderer = std::unique_ptr<SceneRenderer>(new SceneRenderer(m_deviceResources));
	m_sceneRenderer->CreateDeviceDependentResources();
		
}

void AppXamlDX12Main::StartRenderLoop()
{
	// If the animation render loop is already running then do not start another thread.
	if (m_renderLoopWorker != nullptr && m_renderLoopWorker->Status == Windows::Foundation::AsyncStatus::Started)
	{
		return;
	}

	// Create a task that will be run on a background thread.
	auto workItemHandler = ref new WorkItemHandler([this](IAsyncAction^ action)
	{
		while (action->Status == Windows::Foundation::AsyncStatus::Started)
		{
			critical_section::scoped_lock lock(m_criticalSection);
			////////////////////////////////////////////////////////////////////
			
				auto commandQueue = m_deviceResources->GetCommandQueue();
				PIXBeginEvent(commandQueue, 0, L"Update");
				{
					Update();
				}
				PIXEndEvent(commandQueue);
			
				PIXBeginEvent(commandQueue, 0, L"Render");
				{
					//if (Render() == true)
					//{

						//m_deviceResources->Present();
						//m_sceneRenderer->SetLoadingComplete(false);
						if (m_deviceResources->m_isSwapPanelVisible == true)
						{
							m_sceneRenderer->Render();
						}
						//OutputDebugString(L"\nAppXamlDX12:Rendered\n");

						//	
					//}

				}
				PIXEndEvent(commandQueue);

			
		}
	});
	// Run task on a dedicated high priority background thread.
	m_renderLoopWorker = ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);
}

void AppXamlDX12Main::StopRenderLoop()
{
	m_renderLoopWorker->Cancel();
}


// Updates the application state once per frame.
void AppXamlDX12Main::Update()
{
	ProcessInput();
	// Update scene objects.
	m_timer.Tick([&]()
	{
		// TODO: Replace this with your app's content update functions.
		m_sceneRenderer->Update(m_timer);
		//m_cd3d12GridRenderer->Update(m_timer);
		//m_fpsTextRenderer->Update(m_timer);
	});
			
		Render();
	
}


// Renders the current frame according to the current application state.
// Returns true if the frame was rendered and is ready to be displayed.
bool AppXamlDX12Main::Render()
{
	
	// Don't try to render anything before the first Update.
	if (m_timer.GetFrameCount() == 0)
	{
		return false;
	}

	if (!m_sceneRenderer->GetLoadingComplete()) 
	{
		return false;
	}

	
		//auto 
		ID3D12GraphicsCommandList* context = m_deviceResources->GetCommandList();// m_cd3d12GridRenderer->GetComList();// 

		if (context == 0x0000000000000000)
		{
			return m_sceneRenderer->Render();
		}

		// Render the scene objects.
		// TODO: Replace this with your app's content rendering functions.
		if (m_deviceResources->m_isSwapPanelVisible == true)
		{
		      return m_sceneRenderer->Render();
	    }

	return true;
}

void AppXamlDX12::AppXamlDX12Main::Clear()
{
	m_sceneRenderer->Clear();
}

void AppXamlDX12::AppXamlDX12Main::OnWindowSizeChanged()
{
	Size size = m_deviceResources->GetOutputSize();
		return;
	if (!m_deviceResources->WindowSizeChanged(
		(int)size.Width,
		(int)size.Height,
		m_deviceResources->GetRotation()))
		return;
	// TODO: Replace this with the size-dependent initialization of your app's content.
	m_sceneRenderer->CreateWindowSizeDependentResources();
		
}

void AppXamlDX12::AppXamlDX12Main::ValidateDevice()
{
	m_deviceResources->ValidateDevice();
}

void AppXamlDX12::AppXamlDX12Main::OnSuspending()
{
	// TODO: Replace this with your app's suspending logic.

	// Process lifetime management may terminate suspended apps at any time, so it is
	// good practice to save any state that will allow the app to restart where it left off.

	m_sceneRenderer->SaveState();


}

void AppXamlDX12::AppXamlDX12Main::OnResuming()
{
	// TODO: Replace this with your app's resuming logic.
	m_timer.ResetElapsedTime();
	/*m_gamePadButtons.Reset();
	m_keyboardButtons.Reset();
	m_audEngine->Resume();*/
}

void AppXamlDX12::AppXamlDX12Main::OnDeviceRemoved()
{
	// TODO: Save any necessary application or renderer state and release the renderer
	// and its resources which are no longer valid.
	m_sceneRenderer->SaveState();
	m_sceneRenderer = nullptr;
	
}

// Notifies renderers that device resources need to be released.
void AppXamlDX12Main::OnDeviceLost()
{
	m_sceneRenderer->OnDeviceLost();
}

// Notifies renderers that device resources may now be recreated.
void AppXamlDX12Main::OnDeviceRestored()
{
	m_sceneRenderer->OnDeviceRestored();
}
	

void AppXamlDX12::AppXamlDX12Main::WindowActivationChanged(Windows::UI::Core::CoreWindowActivationState activationState)
{
}

void AppXamlDX12::AppXamlDX12Main::KeyDown(Windows::System::VirtualKey key)
{
}

void AppXamlDX12::AppXamlDX12Main::KeyUp(Windows::System::VirtualKey key)
{
}

void AppXamlDX12::AppXamlDX12Main::ProcessInput()
{
	// TODO: Add per frame input handling here.
	m_sceneRenderer->TrackingUpdate(m_pointerLocationX, m_pointerLocationY);
	
}

