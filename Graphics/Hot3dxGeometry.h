#ifndef _HOT3DXGEOMETRY_H_
#define _HOT3DXGEOMETRY_H_
//--------------------------------------------------------------------------------------
// File: Hot3dxGeometry.h
//
// Copyright (c) Jeff Kubitz - hot3dx. All rights reserved.
// 
// DirectX 12 C++ XMFLOAT and XMVECTOR planar geometry functions
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "..\Common\d3dx12.h"
#include <d3d12.h>
#include <DirectXMath.h>

/////////////////////////////////////////////////////////////////////////////
// Geometry window
using namespace DirectX;

namespace AppXamlDX12
{
	class CHot3dxD3D12Geometry
	{
		// Construction
	public:
		CHot3dxD3D12Geometry();

		// XMFLOAT3 Operations
	public:
		double XM_CALLCONV distanceBetweenPoints(XMFLOAT3 v, XMFLOAT3 pos);
		XMFLOAT3 XM_CALLCONV directionBetweenPoints(XMFLOAT3 _from, XMFLOAT3 _to);
		//XMFLOAT3 CenterOfFace(CD3D9Face face);
		double XM_CALLCONV Magnitude(XMFLOAT3 v);
		double XM_CALLCONV AngleBetweenTwoLines(XMFLOAT3 v, XMFLOAT3 s);
		void XM_CALLCONV InitVector(XMFLOAT3 v);
		double XM_CALLCONV FindPlaneConstant(XMFLOAT3 planeNormal, XMFLOAT3 a);
		XMFLOAT3 XM_CALLCONV FindPlaneNormal(XMFLOAT3 a, XMFLOAT3 b, XMFLOAT3 c);
		XMFLOAT3 XM_CALLCONV FindPointIntersectPlane(XMFLOAT3 planeNormal, XMFLOAT3 lineOrigin, XMFLOAT3 lineDirection, float c);
		//XMFLOAT3 GetFaceVertex(CD3D9Face face, int id);

		// XMVECTOR 3 Operations
		double XM_CALLCONV distanceBetweenPointsVec(XMVECTOR v, XMVECTOR pos);
		XMVECTOR XM_CALLCONV directionBetweenPointsVec(XMVECTOR _from, XMVECTOR _to);
		double XM_CALLCONV MagnitudeVec(XMVECTOR v);
		double XM_CALLCONV AngleBetweenTwoLinesVec(XMVECTOR v, XMVECTOR s);
		void XM_CALLCONV InitVectorVec(XMVECTOR v);
		XMVECTOR XM_CALLCONV FindPlaneNormalVec(XMVECTOR a, XMVECTOR b, XMVECTOR c);
		double XM_CALLCONV FindPlaneConstantVec(XMVECTOR planeNormal, XMVECTOR a);
		XMVECTOR XM_CALLCONV FindPointIntersectPlaneVec(XMVECTOR planeNormal, XMVECTOR lineOrigin, XMVECTOR lineDirection, float c);
		
		// Attributes
	public:

	
	public:
		//virtual ~CHot3dxD3D12Geometry();


	protected:

	};
}
/////////////////////////////////////////////////////////////////////////////


#endif // _HOT3DXGEOMETRY_H_
