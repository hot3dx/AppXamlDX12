#pragma once

#include <ppltasks.h>

#include <wrl.h>

#include <wrl/client.h>
#include <dxgi1_4.h>
#include <dxgi1_5.h>
#include <dxgi1_6.h>
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
#include <DirectXPackedVector.h>
#include <DirectXCollision.h>
#include <agile.h>
#include <concrt.h>
#include <collection.h>
#include "App.xaml.h"
#include <synchapi.h>
#include <strsafe.h>
#if defined(_DEBUG)
#include <dxgidebug.h>
#endif

#include <algorithm>
#include <atomic>
#include <array>
#include <exception>
#include <initializer_list>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <malloc.h>
#include <stddef.h>
#include <stdint.h>

#define XAUDIO2_HELPER_FUNCTIONS 1
#include <xaudio2.h>
#include <xaudio2fx.h>

#include <mmreg.h>
#include <mfidl.h>
#include <mfapi.h>
#include <mfreadwrite.h>
#include <mfmediaengine.h>
#include <mferror.h>

struct RectF
{
	float Left;
	float Top;
	float Right;
	float Bottom;
};

struct PointF
{
	float X;
	float Y;
};

struct ColorF
{
	float R;
	float G;
	float B;
	float A;
};

#define SINGLETHREADED FALSE
