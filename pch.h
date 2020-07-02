#pragma once

#pragma warning(disable : 4619 4061 4265 4355 4365 4571 4623 4625 4626 4628 4668 4710 4711 4746 4774 4820 4987 5026 5027 5031 5032 5039 5045)


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

#pragma warning(push)
#pragma warning(disable : 4702)
#include <functional>
#pragma warning(pop)

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
