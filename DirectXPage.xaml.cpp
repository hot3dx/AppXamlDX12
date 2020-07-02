//
// DirectXPage.xaml.cpp
// Implementation of the DirectXPage class.
//

#include "pch.h"
#include "DirectXPage.xaml.h"
#include <ppltasks.h>
#include "AppXamlDX12Main.h"

using namespace AppXamlDX12;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
using namespace Windows::System;
using namespace Windows::System::Threading;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;
using namespace Windows::Storage::Provider;
using namespace concurrency;

AppXamlDX12::DirectXPage::DirectXPage():
	m_windowVisible(true),
	m_coreInput(nullptr)
{
	InitializeComponent();

	// Register event handlers for page lifecycle.
	CoreWindow^ window = Window::Current->CoreWindow;

	window->VisibilityChanged +=
		ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &DirectXPage::OnVisibilityChanged);

	DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();

	currentDisplayInformation->DpiChanged +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &DirectXPage::OnDpiChanged);

	currentDisplayInformation->OrientationChanged +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &DirectXPage::OnOrientationChanged);

	DisplayInformation::DisplayContentsInvalidated +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &DirectXPage::OnDisplayContentsInvalidated);

	swapChainPanel->CompositionScaleChanged += 
		ref new TypedEventHandler<SwapChainPanel^, Object^>(this, &DirectXPage::OnCompositionScaleChanged);

	swapChainPanel->SizeChanged +=
		ref new SizeChangedEventHandler(this, &DirectXPage::OnSwapChainPanelSizeChanged);
	
	window->KeyDown += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &DirectXPage::OnKeyDown);

	window->KeyUp += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &DirectXPage::OnKeyUp);

	IDC_NEW_BUTTON->Click += ref new RoutedEventHandler(this, &DirectXPage::IDC_NEW_BUTTON_Click);
	IDC_SAVE_BUTTON->Click += ref new RoutedEventHandler(this, &DirectXPage::IDC_SAVE_BUTTON_Click); 
	IDC_SET_COLORS_BUTTON->Click += ref new RoutedEventHandler(this, &DirectXPage::IDC_SET_COLORS_BUTTON_Click);
	
	// At this point we have access to the device. 
	// We can create the device-dependent resources.
	m_deviceResources = std::make_shared<DX::DeviceResources>();
	m_deviceResources->SetSwapChainPanel(swapChainPanel);

	AppXamlDX12::DirectXPage^ huh = this;
	// Register our SwapChainPanel to get independent input pointer events
	auto workItemHandler = ref new WorkItemHandler([huh] (IAsyncAction^)
	{
		// The CoreIndependentInputSource will raise pointer events for the specified device types on whichever thread it's created on.
		huh->m_coreInput = huh->swapChainPanel->CreateCoreIndependentInputSource(
			Windows::UI::Core::CoreInputDeviceTypes::Mouse |
			Windows::UI::Core::CoreInputDeviceTypes::Touch |
			Windows::UI::Core::CoreInputDeviceTypes::Pen
			);

		// Register for pointer events, which will be raised on the background thread.
		huh->m_coreInput->PointerPressed += ref new TypedEventHandler<Object^, PointerEventArgs^>(huh, &DirectXPage::OnPointerPressed);
		huh->m_coreInput->PointerMoved += ref new TypedEventHandler<Object^, PointerEventArgs^>(huh, &DirectXPage::OnPointerMoved);
		huh->m_coreInput->PointerReleased += ref new TypedEventHandler<Object^, PointerEventArgs^>(huh, &DirectXPage::OnPointerReleased);
		huh->m_coreInput->PointerWheelChanged += ref new TypedEventHandler<Object^, PointerEventArgs^>(huh, &DirectXPage::OnPointerWheelChanged);
		huh->m_coreInput->PointerEntered += ref new TypedEventHandler<Object^, PointerEventArgs^>(huh, &DirectXPage::OnPointerEntered);
		huh->m_coreInput->PointerExited += ref new TypedEventHandler<Object^, PointerEventArgs^>(huh, &DirectXPage::OnPointerExited);
		huh->m_coreInput->PointerCaptureLost += ref new TypedEventHandler<Object^, PointerEventArgs^>(huh, &DirectXPage::OnPointerCaptureLost);
		
		// Begin processing input messages as they're delivered.
		huh->m_coreInput->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessUntilQuit);
	});
	huh = nullptr;
	// Run task on a dedicated high priority background thread.
	m_inputLoopWorker = ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);

	m_main = std::unique_ptr<AppXamlDX12Main>(new AppXamlDX12Main(m_deviceResources));
	m_main->StartRenderLoop();
	
}

DirectXPage::~DirectXPage()
{
	// Stop rendering and processing events on destruction.
	m_main->StopRenderLoop();
	m_coreInput->Dispatcher->StopProcessEvents();
}

// Saves the current state of the app for suspend and terminate events.
void DirectXPage::SaveInternalState(IPropertySet^ state)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	m_deviceResources->Trim();

	// Stop rendering when the app is suspended.
	m_main->StopRenderLoop();

	// Put code to save app state here.
}

// Loads the current state of the app for resume events.
void DirectXPage::LoadInternalState(IPropertySet^ state)
{
	// Put code to load app state here.

	// Start rendering when the app is resumed.
	m_main->StartRenderLoop();
}

// Window event handlers.

void AppXamlDX12::DirectXPage::OnRendering(Platform::Object^ sender, Platform::Object^ args)
{
	//throw ref new Platform::NotImplementedException();
}

void DirectXPage::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	m_windowVisible = args->Visible;
	if (m_windowVisible)
	{
		m_main->StartRenderLoop();
	}
	else
	{
		m_main->StopRenderLoop();
	}
}


// DisplayInformation event handlers.

void DirectXPage::OnDpiChanged(DisplayInformation^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	// Note: The value for LogicalDpi retrieved here may not match the effective DPI of the app
	// if it is being scaled for high resolution devices. Once the DPI is set on DeviceResources,
	// you should always retrieve it using the GetDpi method.
	// See DeviceResources.cpp for more details.
	m_deviceResources->SetDpi(sender->LogicalDpi);
	m_main->CreateWindowSizeDependentResources();
}

void DirectXPage::OnOrientationChanged(DisplayInformation^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	m_deviceResources->SetCurrentOrientation(sender->CurrentOrientation);
	m_main->CreateWindowSizeDependentResources();
}

void DirectXPage::OnDisplayContentsInvalidated(DisplayInformation^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	m_deviceResources->ValidateDevice();
}

void AppXamlDX12::DirectXPage::OnStereoEnabledChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	//m_deviceResources->UpdateStereoState();
	m_main->CreateWindowSizeDependentResources();
}

// Called when the app bar button is clicked.
void DirectXPage::AppBarButton_Click(Object^ sender, RoutedEventArgs^ e)
{
	// Clear previous returned file name, if it exists, between iterations of this scenario
	//OutputTextBlock->Text = "";

	FileOpenPicker^ openPicker = ref new FileOpenPicker();
	openPicker->ViewMode = PickerViewMode::Thumbnail;
	openPicker->SuggestedStartLocation = PickerLocationId::DocumentsLibrary;
	//openPicker->FileTypeFilter->Append(".jpg");
	//openPicker->FileTypeFilter->Append(".jpeg");
	//openPicker->FileTypeFilter->Append(".png");
	openPicker->FileTypeFilter->Append(".txt");
	openPicker->FileTypeFilter->Append(".rtf");
	openPicker->FileTypeFilter->Append(".doc");
	openPicker->FileTypeFilter->Append(".docx");

	DirectXPage^ _this = this;
	create_task(openPicker->PickSingleFileAsync()).then([_this](StorageFile^ file)
	{
		if (file)
		{
			//_this->OutputTextBlock->Text = "Picked document: " + file->Name;
			//_this->theDoc = file->ToString();
			//PlaySound(theDoc);
		}
		else
		{
			//_this->OutputTextBlock->Text = "Operation cancelled.";
		}
	});
	_this = nullptr;
}

void AppXamlDX12::DirectXPage::IDC_NEW_BUTTON_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	
}

void AppXamlDX12::DirectXPage::IDC_WELCOME_STATIC_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	
}

void AppXamlDX12::DirectXPage::IDC_SET_COLORS_BUTTON_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	//	IDC_SET_COLORS_BUTTON
	ColorPicker^ cp = ref new ColorPicker();

	Media::Brush^ brush = IDC_CLIP_STATIC2->Background;
	
	//brush->SetValue(brush->OpacityProperty, cp->Color);
	

}

void AppXamlDX12::DirectXPage::confirmColor_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	// Assign the selected color to a variable to use outside the popup.    
	//myColor = myColorPicker->Color;
	// Close the Flyout.    
	//colorPickerButton.Flyout.Hide(); 
}

void AppXamlDX12::DirectXPage::cancelColor_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	// Close the Flyout.    
	//colorPickerButton.Flyout.Hide(); 
}

void AppXamlDX12::DirectXPage::IDC_CLEAR_BUTTON_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	m_deviceResources->m_isSwapPanelVisible = true;
	

}

void AppXamlDX12::DirectXPage::IDC_SET_POINTS_BUTTON_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	throw ref new Platform::NotImplementedException();
}

void AppXamlDX12::DirectXPage::IDC_SIZE_OBJECT_BUTTON_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	throw ref new Platform::NotImplementedException();
}

void AppXamlDX12::DirectXPage::IDC_CLEAR_CLEAR_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	throw ref new Platform::NotImplementedException();
}

void AppXamlDX12::DirectXPage::IDC_SAVE_BUTTON_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	// Clear previous returned file name, if it exists, between iterations of this scenario
	//IDC_WELCOME_STATIC->Text = "";//was used as a sample but needs to be changed; A writer function needs to be called with
	//the file data to be saved.

	FileSavePicker^ savePicker = ref new FileSavePicker();
	savePicker->SuggestedStartLocation = PickerLocationId::DocumentsLibrary;

	auto plainTextExtensions = ref new Platform::Collections::Vector<String^>();
	plainTextExtensions->Append(".txt");
	savePicker->FileTypeChoices->Insert("Plain Text", plainTextExtensions);
	savePicker->SuggestedFileName = "New Document";

	DirectXPage^ _this = this;
	create_task(savePicker->PickSaveFileAsync()).then([_this](StorageFile^ file)
	{
		if (file != nullptr)
		{
			// Prevent updates to the remote version of the file until we finish making changes and call CompleteUpdatesAsync.
			CachedFileManager::DeferUpdates(file);
			// write to file
			create_task(FileIO::WriteTextAsync(file, file->Name)).then([_this, file]()
			{
				// Let Windows know that we're finished changing the file so the other app can update the remote version of the file.
				// Completing updates may require Windows to ask for user input.
				create_task(CachedFileManager::CompleteUpdatesAsync(file)).then([_this, file](FileUpdateStatus status)
				{
					if (status == FileUpdateStatus::Complete)
					{
						_this->IDC_WELCOME_STATIC->Text = "File " + file->Name + " was saved.";
					}
					else
					{
						_this->IDC_WELCOME_STATIC->Text = "File " + file->Name + " couldn't be saved.";
					}
				});
			});
		}
		else
		{
			_this->IDC_WELCOME_STATIC->Text = "Operation cancelled.";
		}
	});
	_this = nullptr;
}

void AppXamlDX12::DirectXPage::IDC_ROTO_HELP_BUTTON_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	throw ref new Platform::NotImplementedException();
}

void DirectXPage::OnPointerPressed(Object^ sender, PointerEventArgs^ e)
{
	if (e->CurrentPoint->Properties->IsLeftButtonPressed) { m_main->GetSceneRenderer()->m_bLButtonDown = true; }
	if (e->CurrentPoint->Properties->IsMiddleButtonPressed) {}
	if (e->CurrentPoint->Properties->IsRightButtonPressed) { m_main->GetSceneRenderer()->m_bRButtonDown = true; }
	// When the pointer is pressed begin tracking the pointer movement.
	m_main->StartTracking();
}

void DirectXPage::OnPointerMoved(Object^ sender, PointerEventArgs^ e)
{
	// Update the pointer tracking code.
	if (e->CurrentPoint->Properties->IsLeftButtonPressed) {}
	if (e->CurrentPoint->Properties->IsMiddleButtonPressed) {}
	if (e->CurrentPoint->Properties->IsRightButtonPressed) {}
	if (m_main->IsTracking())
	{
		m_main->TrackingUpdate(e->CurrentPoint->Position.X, e->CurrentPoint->Position.Y);
	}
}

void DirectXPage::OnPointerReleased(Object^ sender, PointerEventArgs^ e)
{
	// Stop tracking pointer movement when the pointer is released.
	if (!e->CurrentPoint->Properties->IsLeftButtonPressed) { m_main->GetSceneRenderer()->m_bLButtonDown = false; }
	if (!e->CurrentPoint->Properties->IsMiddleButtonPressed) {}
	if (!e->CurrentPoint->Properties->IsRightButtonPressed) { m_main->GetSceneRenderer()->m_bRButtonDown = false; }

	m_main->StopTracking();
}

void AppXamlDX12::DirectXPage::OnPointerWheelChanged(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ e)
{
	// negative delta wheel rolled backward - positive delta wheel rolled forward..
	int delta = e->CurrentPoint->Properties->MouseWheelDelta;
}

void AppXamlDX12::DirectXPage::OnPointerEntered(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ e)
{
	//throw ref new Platform::NotImplementedException();
}

void AppXamlDX12::DirectXPage::OnPointerExited(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ e)
{
	//throw ref new Platform::NotImplementedException();
}

void AppXamlDX12::DirectXPage::OnPointerCaptureLost(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ e)
{
	//throw ref new Platform::NotImplementedException();
}

void AppXamlDX12::DirectXPage::OnKeyDown(Windows::UI::Core::CoreWindow^ /*window*/, Windows::UI::Core::KeyEventArgs^ args)
{
	auto Key = args->VirtualKey;
	switch (Key)
	{
	case VirtualKey::F4:
	{
		//m_windowClosed = true;
		OutputDebugString(L"F4 Pressed\n");
	}break;
	case VirtualKey::F5:
	{
		//m_windowClosed = true;
		OutputDebugString(L"F5 Pressed\n");
	}
	case VirtualKey::P:       // Pause
	{
		OutputDebugString(L"Pause\n");
		//m_pauseKeyActive = true;
	}break;
	case VirtualKey::Home:
	{
		OutputDebugString(L"Home\n");
		//m_homeKeyActive = true;
	}break;
	case VirtualKey::Q:
	{
		m_main->GetSceneRenderer()->m_EyeZ += 1.0f;
		m_main->GetSceneRenderer()->m_LookAtZ += 1.0f;
		OutputDebugString(L"Q Pressed\n");
	}break;
	case VirtualKey::W:
	{
		m_main->GetSceneRenderer()->m_EyeY += 1.0f;
		m_main->GetSceneRenderer()->m_LookAtY += 1.0f;
		OutputDebugString(L"W Pressed\n");
	}break;
	case VirtualKey::E:
	{
		m_main->GetSceneRenderer()->m_EyeZ -= 1.0f;
		m_main->GetSceneRenderer()->m_LookAtZ -= 1.0f;
		OutputDebugString(L"E Pressed\n");
	}break;
	case VirtualKey::A:
	{
		m_main->GetSceneRenderer()->m_EyeX += 1.0f;
		m_main->GetSceneRenderer()->m_LookAtX += 1.0f;
		OutputDebugString(L"A Pressed\n");
	}break;
	case VirtualKey::S:
	{
		m_main->GetSceneRenderer()->m_EyeY -= 1.0f;
		m_main->GetSceneRenderer()->m_LookAtY -= 1.0f;
		OutputDebugString(L"S Pressed\n");
	}break;
	case VirtualKey::D:
	{
		m_main->GetSceneRenderer()->m_EyeX -= 1.0f;
		m_main->GetSceneRenderer()->m_LookAtX -= 1.0f;
		OutputDebugString(L"D Pressed\n");
	}break;
	case VirtualKey::Z:
	{

		//m_main->GetSceneRenderer()->m_widthRatio += 0.0001f;

	}break;
	case VirtualKey::X:
	{
		//m_main->GetSceneRenderer()->m_widthRatio -= 0.0001f;


	}break;
	case VirtualKey::C:
	{
		//m_main->GetSceneRenderer()->m_heightRatio += 0.0001f;


	}break;
	case VirtualKey::V:
	{
		//m_main->GetSceneRenderer()->m_heightRatio -= 0.0001f;


	}break;
	case VirtualKey::Left:
	{
	}break;
	case VirtualKey::Right:
	{
	}break;
	case VirtualKey::Up:
	{
	}break;
	case VirtualKey::Down:
	{
	}break;
	}
}

void AppXamlDX12::DirectXPage::OnKeyUp(Windows::UI::Core::CoreWindow^ /*window*/, Windows::UI::Core::KeyEventArgs^ args)
{
	if (static_cast<UINT>(args->VirtualKey) < 256)
	{
		//m_pSample->OnKeyUp(static_cast<UINT8>(args->VirtualKey));
	}
}

void AppXamlDX12::DirectXPage::IDC_EXTERIOR_FACES_CHECKBOX_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	throw ref new Platform::NotImplementedException();
}

void AppXamlDX12::DirectXPage::IDC_INTERIOR_FACES_CHECKBOX_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	throw ref new Platform::NotImplementedException();
}

void AppXamlDX12::DirectXPage::IDC_FIRST_TO_LAST_CHECKBOX_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	throw ref new Platform::NotImplementedException();
}

void AppXamlDX12::DirectXPage::IDC_AXIS_CHECKBOX_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	throw ref new Platform::NotImplementedException();
}

void AppXamlDX12::DirectXPage::IDC_TOP_OR_LEFT_CHECKBOX_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	throw ref new Platform::NotImplementedException();
}

void AppXamlDX12::DirectXPage::IDC_BOTTOM_OR_RIGHT_CHECKBOX_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	throw ref new Platform::NotImplementedException();
}

VOID AppXamlDX12::DirectXPage::IDC_ROTATION_EDIT_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	return VOID();
}

void DirectXPage::OnCompositionScaleChanged(Windows::UI::Xaml::Controls::SwapChainPanel^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	m_deviceResources->SetCompositionScale(sender->CompositionScaleX, sender->CompositionScaleY);
	m_main->CreateWindowSizeDependentResources();
}

void DirectXPage::OnSwapChainPanelSizeChanged(Object^ sender, SizeChangedEventArgs^ e)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	m_deviceResources->SetLogicalSize(e->NewSize);
	m_main->CreateWindowSizeDependentResources();
}

void AppXamlDX12::DirectXPage::IDC_SLIDER_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e)
{

}


void AppXamlDX12::DirectXPage::SphereRadiusTextBox_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e)
{

}


void AppXamlDX12::DirectXPage::PointSpacingTextBox_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e)
{

}


void AppXamlDX12::DirectXPage::DrawSphereButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	
	Platform::String^ str;
	auto sceneRenderer = m_main->GetSceneRenderer();

	m_deviceResources->WaitForGpu();
	m_main->StopRenderLoop();

	str = AppXamlDX12::DirectXPage::m_SphereRadiusTextBox->Text;
	m_cameraradius = std::wcstof(str->Data(), nullptr);

	str = AppXamlDX12::DirectXPage::m_PointSpaceTextBox->Text;
	m_camerarotation = std::wcstof(str->Data(), nullptr);

	/*
	//m_anglerotation = m_camerarotation * m_degreeradian;
	sceneRenderer->m_CamXYMoveRotate.m_iCount = 0;

	sceneRenderer->OnDrawSphereButton(this);

	
	sceneRenderer->m_CamXYMoveRotate.m_fCamMove_cameraradius = m_cameraradius;
		str = AppXamlDX12::DirectXPage::m_PointSpaceTextBox->Text;
		sceneRenderer->m_CamXYMoveRotate.m_fCamMove_camerarotation = m_camerarotation;
		//std::wcstof(str->Data(), nullptr); //(float)atof((char*)str->Data());  //str.GetBuffer(str.GetLength()));
		sceneRenderer->m_CamXYMoveRotate.m_fCamMove_anglerotation = sphereRenderer->m_CamXYMoveRotate.m_fCamMove_camerarotation * sphereRenderer->m_CamXYMoveRotate.m_fCamMove_degreeradian;
		sceneRenderer->m_CamXYMoveRotate.m_iCount = 0;
*/
	/*
	//sphereRenderer->ReleaseDeviceDependentResources();
	str = AppXamlDX12::DirectXPage::m_SphereRadiusTextBox->Text;
	sphereRenderer->m_CamXYMoveRotate.m_fCamMove_cameraradius = std::wcstof(str->Data(), nullptr); //(float)atof((char*)str->Data());// (str->Length()));
	str = AppXamlDX12::DirectXPage::m_PointSpaceTextBox->Text;
	sphereRenderer->m_CamXYMoveRotate.m_fCamMove_camerarotation = std::wcstof(str->Data(), nullptr); //(float)atof((char*)str->Data());  //str.GetBuffer(str.GetLength()));
	sphereRenderer->m_CamXYMoveRotate.m_fCamMove_anglerotation = sphereRenderer->m_CamXYMoveRotate.m_fCamMove_camerarotation * sphereRenderer->m_CamXYMoveRotate.m_fCamMove_degreeradian;
	sphereRenderer->m_CamXYMoveRotate.m_iCount = 0;
	//SendMessage(514, 0, 0);
	sphereRenderer->is3DVisible = false;
	sphereRenderer->CreateWindowSizeDependentResources();
	sphereRenderer->CreateDeviceDependentResources();
	sphereRenderer->Render();
	*/
	//sphereRenderer = nullptr;

}
