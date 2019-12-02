//
// App.xaml.h
// Declaration of the App class.
//

#pragma once

#include "..\Generated Files\App.g.h"
#include "DirectXPage.xaml.h"
#include "AppXamlDX12Main.h"

namespace AppXamlDX12
{
		/// <summary>
	/// Provides application-specific behavior to supplement the default Application class.
	/// </summary>
	ref class App sealed
	{
	public:
		App();
		virtual void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ e) override;

	private:
		void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ e);
		void OnResuming(Platform::Object ^sender, Platform::Object ^args);
		void OnNavigationFailed(Platform::Object ^sender, Windows::UI::Xaml::Navigation::NavigationFailedEventArgs ^e);
		DirectXPage^ m_directXPage;

	
		// Private accessor for m_deviceResources, protects against device removed errors.
		std::shared_ptr<DX::DeviceResources> GetDeviceResources();


		std::shared_ptr<DX::DeviceResources> m_deviceResources;
		std::unique_ptr<AppXamlDX12Main> m_main;
	};
}
