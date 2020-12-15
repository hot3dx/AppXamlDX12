//--------------------------------------------------------------------------------------
// File: Hot3dx12Rotate.h
//
// Copyright (c) Jeff Kubitz - hot3dx. All rights reserved.
// 
// DirectX 12 C++ Move and Rotation Functions
//--------------------------------------------------------------------------------------
#pragma once

namespace AppXamlDX12
{
	// A structure for our custom vertex type
	struct CUSTOMVERTEX
	{

		float x, y, z, rhw; // The transformed position for the vertex
		DWORD color;        // The vertex color
	};


	ref class Hot3dx12Rotate
	{
	public:

	internal:Hot3dx12Rotate();

		float  m_fCamMove_north;
		float  m_fCamMove_northeast;
		float  m_fCamMove_east;
		float  m_fCamMove_southeast;
		float  m_fCamMove_south;
		float  m_fCamMove_southwest;
		float  m_fCamMove_west;
		float  m_fCamMove_northwest;
		float  m_fCamMove_anypointcamera;
		float  m_fCamMove_centerofsphere;
		float  m_fCamMove_camerapoint;
		float  m_fCamMove_cameradirection;
		double  m_fCamMove_camerarotation;
		double  m_fCamMove_cameraradius;
		float  m_fCamMove_gridcenter;
		double  m_fCamMove_degreeradian;
		double  m_fCamMove_anglerotation;
		bool    m_bArrayInit;
		double* m_fCamMove_px;
		double* m_fCamMove_py;
		double* m_fCamMove_pz;
		int      m_iCount;
		double  DegreesToRadians(double degree);
		void   InitSphereVars(void);
		float* CalculateMeshBoxAndCenterCV(CUSTOMVERTEX* v, int count);
		int RotateOnMeshZeroAxisCV(CUSTOMVERTEX* v, VOID* pV, int Count, float a, float b, float c);
		int RotateMeshOnAnyAxisCV(float ox, float oy, float oz, CUSTOMVERTEX* v, VOID* pV, int Count, float a, float b, float c);
		void   CalculateSphere(int* count);
		CUSTOMVERTEX* CalculateSphereCV(CUSTOMVERTEX* v, int* n, DWORD dwcolor);
		DirectX::VertexPositionColor* CalculateSphereVPC(DirectX::VertexPositionColor* v, int* n, DWORD dwcolor);
		void   MoveRotateCameraXY(int direction);
		void   MoveRotateCameraAny(float x, float y, float z);

		float yCoordofXRot3(float y, float z, float radAngle);

		float zCoordofXRot3(float y, float z, float radAngle);

		float xCoordofYRot3(float x, float z, float a);

		float zCoordofYRot3(float x, float z, float a);

		float xCoordofZRot3(float x, float y, float radAngle);

		float yCoordofZRot3(float x, float y, float radAngle);

	private:
		CUSTOMVERTEX* vertices;
		VOID* pVertices;

	};
}