#pragma once

#include "pch.h"

using namespace DirectX;

#ifndef RGBA_FRACTAL
#define RGBA_FRACTAL 0.00390625F
#endif

/* Format of RGBA colors is
* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*| alpha | red | green | blue |
*+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
#define RGBA_GETALPHA(rgb)      ((rgb) >> 24)
#define RGBA_GETRED(rgb)        (((rgb) >> 16) & 0xff)
#define RGBA_GETGREEN(rgb)      (((rgb) >> 8) & 0xff)
#define RGBA_GETBLUE(rgb)       ((rgb) & 0xff)
#define RGBA_MAKE(r, g, b, a)   ((DWORD) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))
#define RGBA_MAKE_XMVECTOR32(r, g, b, a) ((XMVECTOR32) (((w) << 24) | ((x) << 16) | ((y) << 8) | (z)))
#define RGBA_MAKE_XMVECTOR(r, g, b, a) ((XMVECTOR) (((w) << 24) | ((x) << 16) | ((y) << 8) | (z)))
#define RGBA_MAKE_GXMVECTOR(r, g, b, a) ((GXMVECTOR) (((w) << 24) | ((x) << 16) | ((y) << 8) | (z)))
#define RGBA_MAKE_FXMVECTOR(r, g, b, a) ((FXMVECTOR) (((w) << 24) | ((x) << 16) | ((y) << 8) | (z)))
#define RGBA_MAKE_CXMVECTOR(r, g, b, a) ((CXMVECTOR) (((w) << 24) | ((x) << 16) | ((y) << 8) | (z)))
#define RGBA_MAKE_XMFLOAT3(r, g, b)   ((XMFLOAT3) (((x) << 16) | ((y) << 8) | (z)))
#define RGBA_MAKE_XMFLOAT4(r, g, b, a)   ((XMFLOAT4) (((w) << 24) | ((x) << 16) | ((y) << 8) | (z)))

typedef struct _D3D12HOTBOX
{
	XMVECTOR min, max;
} D3D12HOTBOX, * LPD3D12HOTBOX;

typedef struct _D3D12HOTPICKDESC
{
	ULONG       ulFaceIdx;
	LONG        lGroupIdx;
	XMVECTOR   vPosition;

} D3D12HOTPICKDESC, * LPD3D12HOTPICKDESC;

typedef struct _D3D12HOTPICKDESC2
{
	ULONG       ulFaceIdx;
	LONG        lGroupIdx;
	XMVECTOR    dvPosition;
	float       tu;
	float       tv;
	XMVECTOR    dvNormal;
	XMFLOAT4    dcColor;

} D3D12HOTPICKDESC2, * LPD3D12HOTPICKDESC2;
