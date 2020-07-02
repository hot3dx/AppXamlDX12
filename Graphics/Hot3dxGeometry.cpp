//--------------------------------------------------------------------------------------
// File: Hot3dxGeometry.cpp
//
// Copyright (c) Jeff Kubitz - hot3dx. All rights reserved.
// 
// DirectX 12 C++ XMFLOAT and XMVECTOR planar geometry functions
//--------------------------------------------------------------------------------------
#include "pch.h"
#include "Hot3dxGeometry.h"
#include <DirectXMath.h>
////////////////////////////////////////////////////////////////////////////
// CHot3dxD3D12Geometry
using namespace DirectX;
using namespace AppXamlDX12;

CHot3dxD3D12Geometry::CHot3dxD3D12Geometry()
{
}



double XM_CALLCONV AppXamlDX12::CHot3dxD3D12Geometry::distanceBetweenPoints(XMFLOAT3 v, XMFLOAT3 pos)
{
	return sqrt((pow((v.x - pos.x), 2.0)) + (pow((v.y - pos.y), 2.0)) + (pow((v.z - pos.x), 2.0)));
}

XMFLOAT3 XM_CALLCONV AppXamlDX12::CHot3dxD3D12Geometry::directionBetweenPoints(XMFLOAT3 _from, XMFLOAT3 _to)
{
	XMFLOAT3 v;
	v.x = (_to.x - _from.x);
	v.y = (_to.y - _from.y);
	v.z = (_to.z - _from.z);
	return v;
}

double XM_CALLCONV AppXamlDX12::CHot3dxD3D12Geometry::Magnitude(XMFLOAT3 v)
{
	return sqrt((v.x * v.x) + (v.y * v.y) + (v.z * v.z));
}

double XM_CALLCONV AppXamlDX12::CHot3dxD3D12Geometry::AngleBetweenTwoLines(XMFLOAT3 v, XMFLOAT3 s)
{
	double angle = (((v.x * s.x) + (v.y * s.y) + (v.z * s.z)) / (Magnitude(v) * Magnitude(s)));
	return cos(angle);
}

void XM_CALLCONV AppXamlDX12::CHot3dxD3D12Geometry::InitVector(XMFLOAT3 v)
{
	v.x = 0.0F;
	v.y = 0.0F;
	v.z = 0.0F;
}

double XM_CALLCONV AppXamlDX12::CHot3dxD3D12Geometry::FindPlaneConstant(XMFLOAT3 planeNormal, XMFLOAT3 a)
{
	return (planeNormal.x * a.x) + (planeNormal.y * a.y) + (planeNormal.z * a.z);
}

XMFLOAT3 XM_CALLCONV AppXamlDX12::CHot3dxD3D12Geometry::FindPlaneNormal(XMFLOAT3 a, XMFLOAT3 b, XMFLOAT3 c)
{
	XMFLOAT3 n,j,k = {}; 
	j.x = a.x - b.x;
	j.y = a.y - b.x;
	j.z = a.z - b.z;
	k.x = c.x - b.x;
	k.y = c.y - b.y;
	k.z = c.z - b.z;

	n.x = ((j.y * k.z) - (k.y * j.z));
	n.y = ((j.x * k.z) - (k.x * j.z));
	n.z = ((j.x * k.y) - (k.x * j.y));
	return n;
}

XMFLOAT3 XM_CALLCONV AppXamlDX12::CHot3dxD3D12Geometry::FindPointIntersectPlane(XMFLOAT3 planeNormal, XMFLOAT3 lineOrigin, XMFLOAT3 lineDirection, float c)
{
	XMFLOAT3 intersectPoint;
	intersectPoint.x = 0.0F;
	intersectPoint.y = 0.0F;
	intersectPoint.z = 0.0F;
	float k = 0.0F;
	float denom = (planeNormal.x * (lineDirection.x - lineOrigin.x) + planeNormal.y * (lineDirection.y - lineOrigin.y) + planeNormal.z * (lineDirection.z - lineOrigin.z));
	if (denom == 0.0F) { k = 0.0F; }
	else {
		k = (-(planeNormal.x * lineOrigin.x) - (planeNormal.y * lineOrigin.y) - (planeNormal.z * lineOrigin.z) + c) / (planeNormal.x * (lineDirection.x - lineOrigin.x) + planeNormal.y * (lineDirection.y - lineOrigin.y) + planeNormal.z * (lineDirection.z - lineOrigin.z));
	}
	intersectPoint.x = lineOrigin.x + ((lineDirection.x - lineOrigin.x) * k);
	intersectPoint.y = lineOrigin.y + ((lineDirection.y - lineOrigin.y) * k);
	intersectPoint.z = lineOrigin.z + ((lineDirection.z - lineOrigin.z) * k);

	return intersectPoint;
}

double XM_CALLCONV CHot3dxD3D12Geometry::distanceBetweenPointsVec(XMVECTOR v, XMVECTOR pos)
{

	double x = (double)XMVectorGetX(v) - (double)XMVectorGetX(pos);
	double y = (double)XMVectorGetY(v) - (double)XMVectorGetY(pos);
	double z = (double)XMVectorGetZ(v) - (double)XMVectorGetZ(pos);
	return sqrt((pow((x), 2.0)) + (pow((y), 2.0)) + (pow((z), 2.0)));
}

XMVECTOR XM_CALLCONV AppXamlDX12::CHot3dxD3D12Geometry::directionBetweenPointsVec(XMVECTOR _from, XMVECTOR _to)
{
	
		XMVECTOR from = XMLoadFloat3(&XMFLOAT3(XMVectorGetX(_from), XMVectorGetY(_from), XMVectorGetZ(_from)));
		XMVECTOR to = XMLoadFloat3(&XMFLOAT3(XMVectorGetX(_to), XMVectorGetY(_from), XMVectorGetZ(_to)));
		XMVECTOR result = XMVectorSubtract(to, from);
		//v.x = (XMVectorGetX(to) - XMVectorGetX(from));
		//v.y = (XMVectorGetY(to) - XMVectorGetY(from));
		//v.z = (XMVectorGetZ(to) - XMVectorGetZ(from));
		(XMVectorGetX(result), XMVectorGetY(result), XMVectorGetZ(result));
		
	
	return XMVECTOR(XMLoadFloat3(&XMFLOAT3(XMVectorGetX(result), XMVectorGetY(result), XMVectorGetZ(result))));
}


/*
XMVECTOR AppXamlDX12::CHot3dxD3D12Geometry::CenterOfFace(CD3D9Face face)
{
	int fvCnt = 3;//face.GetVertexCount();
	XMVECTOR l; l.x = 30000.0F; l.y = 30000.0F; l.z = 30000.0F;
	XMVECTOR g; g.x = -30000.0F; g.y = -30000.0F; g.z = -30000.0F;
	for (int j = 1; j < fvCnt + 1; j++)
	{
		XMVECTOR n;
		XMVECTOR v;
		v = GetFaceVertex(face, j);
		if (v.x < l.x)l.x = v.x;
		if (v.y < l.y)l.y = v.y;
		if (v.z < l.z)l.z = v.z;
		if (v.x > g.x)g.x = v.x;
		if (v.y > g.y)g.y = v.y;
		if (v.z > g.z)g.z = v.z;
	}
	g.x += l.x; g.x /= 2.0F; g.y += l.y; g.y /= 2.0F; g.z += l.z; g.z /= 2.0F;
	return g;
}
*/
double XM_CALLCONV AppXamlDX12::CHot3dxD3D12Geometry::MagnitudeVec(XMVECTOR v)
{
	float x = XMVectorGetX(v);
	float y = XMVectorGetY(v);
	float z = XMVectorGetZ(v);
	return (x * x) + (y * y) + (z * z);
}

double XM_CALLCONV AppXamlDX12::CHot3dxD3D12Geometry::AngleBetweenTwoLinesVec(XMVECTOR v, XMVECTOR s)
{
	float vx = XMVectorGetX(v);
	float vy = XMVectorGetY(v);
	float vz = XMVectorGetZ(v);
	float sx = XMVectorGetX(s);
	float sy = XMVectorGetY(s);
	float sz = XMVectorGetZ(s);
	double angle = (((vx * sx) + (vy * sy) + (vz * sz)) / (MagnitudeVec(v) * MagnitudeVec(s)));
	return cos(angle);
}

void XM_CALLCONV AppXamlDX12::CHot3dxD3D12Geometry::InitVectorVec(XMVECTOR v)
{
	XMVectorSetX(v, 0.0f);
	XMVectorSetY(v, 0.0f);
	XMVectorSetZ(v, 0.0f);
	XMVectorSetW(v, 0.0f);
}
XMVECTOR XM_CALLCONV AppXamlDX12::CHot3dxD3D12Geometry::FindPlaneNormalVec(XMVECTOR a, XMVECTOR b, XMVECTOR c)
{

	XMVECTOR j, k;
	XMVECTOR n = {};
	j = XMVectorSubtract(a, b);
	k = XMVectorSubtract(c, b);
	XMVectorSetX(n, ((XMVectorGetY(j)*XMVectorGetZ(k))- (XMVectorGetY(k) * XMVectorGetZ(j))));
	XMVectorSetY(n, ((XMVectorGetX(j) * XMVectorGetZ(k)) - (XMVectorGetX(k) * XMVectorGetZ(j))));
	XMVectorSetZ(n, ((XMVectorGetX(j) * XMVectorGetY(k)) - (XMVectorGetX(k) * XMVectorGetY(j))));
	
	return n;
}
/**
*	a,b,c 3 points of a plane
  */
double XM_CALLCONV AppXamlDX12::CHot3dxD3D12Geometry::FindPlaneConstantVec(XMVECTOR planeNormal, XMVECTOR a)
{
	return (XMVectorGetX(planeNormal) * XMVectorGetX(a)) + (XMVectorGetY(planeNormal) * XMVectorGetY(a)) + (XMVectorGetZ(planeNormal) * XMVectorGetZ(a));
}

XMVECTOR XM_CALLCONV AppXamlDX12::CHot3dxD3D12Geometry::FindPointIntersectPlaneVec(XMVECTOR planeNormal, XMVECTOR lineOrigin, XMVECTOR lineDirection, float c)
{
	XMVECTOR intersectPoint = {};
	XMVectorSetX(intersectPoint, 0.0f);
	XMVectorSetY(intersectPoint, 0.0f);
	XMVectorSetZ(intersectPoint, 0.0f);
	XMVectorSetW(intersectPoint, 0.0f);
	
	float k = 0.0F;
	float denom = (XMVectorGetX(planeNormal)*(XMVectorGetX(lineDirection)- XMVectorGetX(lineOrigin))+ XMVectorGetY(planeNormal)*(XMVectorGetY(lineDirection)- XMVectorGetY(lineOrigin))+ XMVectorGetZ(planeNormal)*(XMVectorGetZ(lineDirection)- XMVectorGetZ(lineOrigin)));
	//float denom = (planeNormal.x*(lineDirection.x-lineOrigin.x)+planeNormal.y*(lineDirection.y-lineOrigin.y)+planeNormal.z*(lineDirection.z-lineOrigin.z));
	if(denom==0.0F)
	{
		k= 0.0F;
	}
	else
	{
	     k = (-(XMVectorGetX(planeNormal) * XMVectorGetX(lineOrigin)) - (XMVectorGetY(planeNormal) * XMVectorGetY(lineOrigin)) - (XMVectorGetZ(planeNormal) * XMVectorGetZ(lineOrigin)) + c) / (XMVectorGetX(planeNormal) * (XMVectorGetX(lineDirection) - XMVectorGetX(lineOrigin)) + XMVectorGetY(planeNormal) * (XMVectorGetY(lineDirection) - XMVectorGetY(lineOrigin)) + XMVectorGetZ(planeNormal) * (XMVectorGetZ(lineDirection) - XMVectorGetZ(lineOrigin)));
	//k = (-(planeNormal.x * lineOrigin.x) - (planeNormal.y * lineOrigin.y) - (planeNormal.z * lineOrigin.z) + c) / (planeNormal.x * (lineDirection.x - lineOrigin.x) + planeNormal.y * (lineDirection.y - lineOrigin.y) + planeNormal.z * (lineDirection.z - lineOrigin.z));
    }
	XMVectorSetX(intersectPoint, (XMVectorGetX(lineOrigin) + ((XMVectorGetX(lineDirection) - XMVectorGetX(lineOrigin)) * k)));
	XMVectorSetY(intersectPoint, (XMVectorGetY(lineOrigin) + ((XMVectorGetY(lineDirection) - XMVectorGetY(lineOrigin)) * k)));
	XMVectorSetZ(intersectPoint, (XMVectorGetZ(lineOrigin) + ((XMVectorGetZ(lineDirection) - XMVectorGetZ(lineOrigin)) * k)));

	return intersectPoint;
}
/*
XMVECTOR AppXamlDX12::CHot3dxD3D12Geometry::GetFaceVertex(CD3D9Face face, int id)
{
	XMVECTOR v;
	switch (id) {
	case 1:v = face.p1; break;
	case 2:v = face.p2; break;
	case 3:v = face.p3; break;
	}
	return v;
}
*/