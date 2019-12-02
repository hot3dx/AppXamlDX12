#pragma once

#include <wrl.h>
#include <wrl/client.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include "Common\d3dx12.h"
#include <d3d11_3.h>
#include <d2d1_3.h>
#include <d2d1effects_2.h>
#include <dwrite_3.h>
#include <wincodec.h>
// Uses NuGet.org package WinPixEventRuntime 1.0.190604001
#include <pix3.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <memory>
#include <agile.h>
#include <concrt.h>
#include <collection.h>
#include "App.xaml.h"

#if defined(_DEBUG)
#include <dxgidebug.h>
#endif