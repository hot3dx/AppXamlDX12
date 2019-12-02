//
// DirectXPage.xaml.h
// Declaration of the DirectXPage class.
//

#pragma once

#include "pch.h"
#include "DirectXPage.g.h"

#include "Common/DeviceResources.h"
#include "AppXamlDX12Main.h"



namespace AppXamlDX12
{
	using namespace DX;
	/// <summary>
	/// A page that hosts a DirectX SwapChainPanel.
	/// </summary>
	public ref class DirectXPage sealed
	{
	public:
		DirectXPage();
		virtual ~DirectXPage();

		void SaveInternalState(Windows::Foundation::Collections::IPropertySet^ state);
		void LoadInternalState(Windows::Foundation::Collections::IPropertySet^ state);

		// Accessors
		DirectXPage^ GetThisDirectXPage() { return this; }
		Windows::UI::Xaml::Controls::TextBox^ GetSphereRadiusTextBox() { return m_SphereRadiusTextBox; }
		Windows::UI::Xaml::Controls::TextBox^ GetPointSpaceTextBox(){ return m_PointSpaceTextBox;}



	private:
		// XAML low-level rendering event handler.
		void OnRendering(Platform::Object^ sender, Platform::Object^ args);

		// Window event handlers.
		void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args);

		// DisplayInformation event handlers.
		void OnDpiChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
		void OnOrientationChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
		void OnDisplayContentsInvalidated(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);

		// Other event handlers.
		// Buttons
		void AppBarButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void IDC_SET_COLORS_BUTTON_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void IDC_CLEAR_BUTTON_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void IDC_SET_POINTS_BUTTON_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void IDC_SIZE_OBJECT_BUTTON_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void IDC_CLEAR_CLEAR_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void IDC_SAVE_BUTTON_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void IDC_ROTO_HELP_BUTTON_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		//void IDC__BUTTON_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);


		// CheckBoxes
		void IDC_EXTERIOR_FACES_CHECKBOX_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void IDC_INTERIOR_FACES_CHECKBOX_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void IDC_FIRST_TO_LAST_CHECKBOX_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void IDC_AXIS_CHECKBOX_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void IDC_TOP_OR_LEFT_CHECKBOX_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void IDC_BOTTOM_OR_RIGHT_CHECKBOX_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

		// TextBoxes
		VOID IDC_ROTATION_EDIT_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

		// Slider
		void IDC_SLIDER_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);

		void OnCompositionScaleChanged(Windows::UI::Xaml::Controls::SwapChainPanel^ sender, Object^ args);
		void OnSwapChainPanelSizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e);

		// Track our independent input on a background worker thread.
		Windows::Foundation::IAsyncAction^ m_inputLoopWorker;
		Windows::UI::Core::CoreIndependentInputSource^ m_coreInput;

		// Independent input handling functions.
		void OnPointerPressed(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ e);
		void OnPointerMoved(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ e);
		void OnPointerReleased(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ e);

		// Resources used to render the DirectX content in the XAML page background.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;
		std::unique_ptr<AppXamlDX12Main> m_main;

		bool m_windowVisible;
		void SphereRadiusTextBox_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e);
		void PointSpacingTextBox_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e);
		void DrawSphereButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

		float m_cameraradius;
		float m_camerarotation;
		float m_anglerotation;
		int m_iCount;
	};
}

