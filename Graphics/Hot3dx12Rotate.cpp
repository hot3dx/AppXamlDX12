#include "pch.h"
#include "Hot3dx12Rotate.h"
//--------------------------------------------------------------------------------------
// File: Hot3dx12Rotate.cpp
//
// Copyright (c) 2020 Jeff Kubitz - hot3dx. All rights reserved.
// 
// DirectX 12 C++ Move and Rotation Functions
//--------------------------------------------------------------------------------------

#include "pch.h"

AppXamlDX12::Hot3dx12Rotate::Hot3dx12Rotate()
{
	m_fCamMove_degreeradian = 0.017453293005625408F;
	// radian 57.29577791868204900000
	m_fCamMove_camerarotation = 10.0F;
	m_fCamMove_anglerotation = m_fCamMove_camerarotation * m_fCamMove_degreeradian;
	m_fCamMove_cameraradius = 81.0F;
	m_bArrayInit = false;
	return;
}

double AppXamlDX12::Hot3dx12Rotate::DegreesToRadians(double degree)
{
	m_fCamMove_camerarotation = m_fCamMove_anglerotation * m_fCamMove_degreeradian;
	return m_fCamMove_camerarotation;
}

void AppXamlDX12::Hot3dx12Rotate::InitSphereVars(void)
{
	//throw ref new Platform::NotImplementedException();
}

float* AppXamlDX12::Hot3dx12Rotate::CalculateMeshBoxAndCenterCV(CUSTOMVERTEX* v, int count)
{
	static float box[9];
	box[0] = v[0].x;
	box[1] = v[0].x;
	box[2] = v[0].y;
	box[3] = v[0].y;
	box[4] = v[0].z;
	box[5] = v[0].z;
	int i = 1;
	while (i < count)
	{
		if (v[i].x < box[0]) { box[0] = v[i].x; }
		if (v[i].x > box[1]) { box[1] = v[i].x; }
		if (v[i].y < box[2]) { box[2] = v[i].y; }
		if (v[i].y > box[3]) { box[3] = v[i].y; }
		if (v[i].z < box[4]) { box[4] = v[i].z; }
		if (v[i].z > box[5]) { box[5] = v[i].z; }
		i++;
	}

	box[6] = box[0] + ((box[1] - box[0]) / 2);
	box[7] = box[2] + ((box[3] - box[2]) / 2);
	box[8] = box[4] + ((box[5] - box[4]) / 2);

	wchar_t mybox[200] = {};
	swprintf(mybox, 200, L"\n box center x: %f y: %f z: %f\n", box[6], box[7], box[8]);
	OutputDebugString(mybox);
	return box;

}

int AppXamlDX12::Hot3dx12Rotate::RotateOnMeshZeroAxisCV(CUSTOMVERTEX* v, VOID* pV, int Count, float a, float b, float c)
{
	// storage values to vertices
	v = (CUSTOMVERTEX*)pV;
	// Find box and center of Mesh
	float* box = CalculateMeshBoxAndCenterCV(v, Count);
	if (box != NULL)
	{
		int i = 0;
		v[Count / 2].color = 0xffffffff;
		float mag1 = (float)sqrt(
		((double)v[i].x * (double)v[i].x)
			+ ((double)v[i].y * (double)v[i].y)
			+ ((double)v[i].z * (double)v[i].z));

		if (mag1 != 0.0)
		{
			while (i < Count)
			{
				// center it on point of rotation
				v[i].x -= box[6];
				v[i].y -= box[7];
				v[i].z -= box[8];
				float x, y, z;

				// x axis
				if (a != 0.0f)
				{
					y = v[i].y / mag1;
					z = v[i].z / mag1;
					v[i].y = mag1 * yCoordofXRot3(y, z, a * (3.141592654f / 180.0f));
					v[i].z = mag1 * zCoordofXRot3(y, z, a * (3.141592654f / 180.0f));
				}
				// y axis
				if (b != 0.0f)
				{
					x = v[i].x / mag1;
					z = v[i].z / mag1;
					v[i].x = mag1 * xCoordofYRot3(x, z, b * (3.141592654f / 180.0f));
					v[i].z = mag1 * zCoordofYRot3(x, z, b * (3.141592654f / 180.0f));
				}
				// z axis
				if (c != 0.0f)
				{
					x = v[i].x / mag1;
					y = v[i].y / mag1;
					v[i].x = mag1 * xCoordofZRot3(x, y, c * (3.141592654f / 180.0f));
					v[i].y = mag1 * yCoordofZRot3(x, y, c * (3.141592654f / 180.0f));
				}
				// add in offset
				v[i].x += box[6];
				v[i].y += box[7];
				v[i].z += box[8];
				i++;
			}// eo i while

		// set vertices values to storage
			pV = (VOID*)v;
			return 1;
		}//if(mag!=0.0)
	}//if(box!= NULL)
	free(box);
	return 0;
}

int AppXamlDX12::Hot3dx12Rotate::RotateMeshOnAnyAxisCV(float ox, float oy, float oz, // origin axis
	CUSTOMVERTEX* v, VOID* pV, int Count, // mesh info
	float a, float b, float c) // angle rotation: x,y.z
{
	// storage values to vertices
	v = (CUSTOMVERTEX*)pV;
	// Find box and center of Mesh
	float DegRad = 3.141592654f / 180.0f;
	int i = 0;

	while (i < Count)
	{
		float mag1 = (float)sqrt(
		(((double)v[i].x - ox) * ((double)v[i].x - ox))
			+ (((double)v[i].y - oy) * ((double)v[i].y - oy))
			+ (((double)v[i].z - oz) * ((double)v[i].z - oz)));
		// center it on point of rotation
		v[i].x -= ox;
		v[i].y -= oy;
		v[i].z -= oz;

		if (mag1 != 0.0)
		{
			float x, y, z;
			// x axis
			if (a != 0.0f)
			{
				y = v[i].y / mag1;
				z = v[i].z / mag1;
				v[i].y = mag1 * yCoordofXRot3(y, z, a * DegRad);
				v[i].z = mag1 * zCoordofXRot3(y, z, a * DegRad);
			}
			// y axis
			if (b != 0.0f)
			{
				x = v[i].x / mag1;
				z = v[i].z / mag1;
				v[i].x = mag1 * xCoordofYRot3(x, z, b * DegRad);
				v[i].z = mag1 * zCoordofYRot3(x, z, b * DegRad);
			}
			// z axis
			if (c != 0.0f)
			{
				x = v[i].x / mag1;
				y = v[i].y / mag1;
				v[i].x = mag1 * xCoordofZRot3(x, y, c * DegRad);
				v[i].y = mag1 * yCoordofZRot3(x, y, c * DegRad);
			}
		}//if(mag!=0.0)
		// add in offset
		v[i].x += ox;
		v[i].y += oy;
		v[i].z += oz;
		i++;
	}// eo i while

// set vertices values to storage
	pV = (VOID*)v;

	return 1;
}



void AppXamlDX12::Hot3dx12Rotate::CalculateSphere(int* count)
{
	// all points on sphere at origin
	// x^2 + y^2 + z^2 = r^2 
	// x^2 + y^2 + z^2 - r^2 = 0
	// add x, y, z to position sphere in 3D space
	int cnt = 0;
	double pi = 3.1415926535897932F;
	double repCount = (360.0F * m_fCamMove_degreeradian) / m_fCamMove_anglerotation;
	double* ox;
	double* oy;
	int num = (int)repCount;
	ox = (double*)malloc((num + 1) * sizeof(double));
	if (ox != NULL)
	{
		oy = (double*)malloc((num + 1) * sizeof(double));
		if (oy != NULL)
		{
			double angle = 0.0F;
			double distance;
			int half = 1 + num / 2;

			for (int i = 0; i < half; i++)
			{
				ox[i] = m_fCamMove_cameraradius * (1.0 * cos(angle));
				oy[i] = sqrt(pow(m_fCamMove_cameraradius, 2.0) - pow(ox[i], 2.0));//*sin(angle);

				if (oy[i] != 0.0F)
				{
					distance = sqrt(pow((ox[i - 1] - ox[i]), 2.0)
						+ pow((oy[i - 1] - oy[i]), 2.0));
					double radius = fabs(oy[i]);
					double circum = 2 * pi * radius;
					int rep = (int)(circum / distance);
					double zangle = 0.0F;
					double addangle = (360.0F / rep) * m_fCamMove_degreeradian;

					for (int j = 0; j < rep; j++)
					{
						cnt++;
						zangle += addangle;
					}// eo j for
				}// eo if i < 0
				else
				{
					cnt++;

				}
				angle += m_fCamMove_anglerotation;

			}// eo i for


			if (m_bArrayInit == true)
			{
				free(m_fCamMove_px); free(m_fCamMove_py); free(m_fCamMove_pz);
			}
			///////////////////////////////////////
			m_fCamMove_px = (double*)malloc(cnt * sizeof(double));
			if (m_fCamMove_px != NULL)
			{
				m_fCamMove_py = (double*)malloc(cnt * sizeof(double));
				if (m_fCamMove_py != NULL)
				{
					m_fCamMove_pz = (double*)malloc(cnt * sizeof(double));
					if (m_fCamMove_pz != NULL)
					{
						m_bArrayInit = true;
						angle = 0.0F;
						cnt = 0;
						int i = 0;
						for (i=0; i < half; i++)
						{
							ox[i] = m_fCamMove_cameraradius * (1.0 * cos(angle));
							oy[i] = sqrt(pow(m_fCamMove_cameraradius, 2.0) - pow(ox[i], 2.0));//*sin(angle);

							if (oy[i] != 0.0F)
							{
								distance = sqrt(pow((ox[i - 1] - ox[i]), 2.0)
									+ pow((oy[i - 1] - oy[i]), 2.0));
								double radius = fabs(oy[i]);
								double circum = 2 * pi * radius;
								int rep = (int)(circum / distance);
								double zangle = 0.0F;
								double addangle = (360.0F / rep) * m_fCamMove_degreeradian;

								for (int j = 0; j < rep; j++)
								{
									m_fCamMove_px[cnt] = ox[i];
									m_fCamMove_py[cnt] = radius * (1.0 * cos(zangle));
									m_fCamMove_pz[cnt] = radius * (1.0 * sin(zangle));
									cnt++;
									zangle += addangle;
								}// eo j for
							}// eo if i < 0
							else
							{

								m_fCamMove_px[cnt] = ox[i];
								m_fCamMove_py[cnt] = 0.0F;
								m_fCamMove_pz[cnt] = 0.0F;
								cnt++;

							}
							angle += m_fCamMove_anglerotation;

						}// eo i for
						float x = (float)-ox[0];
						m_fCamMove_px[cnt] = x;// +150.0F;
						m_fCamMove_py[cnt] = 0.0f;// +150.0F;
						m_fCamMove_pz[cnt] = 0.0F;
						cnt++;
						m_iCount = count[0] = cnt;
						free(ox);
						free(oy);
					}
				}
			}
			free(ox);
		}free(oy);
	}// eo ox and oy != NULL
}

AppXamlDX12::CUSTOMVERTEX* AppXamlDX12::Hot3dx12Rotate::CalculateSphereCV(CUSTOMVERTEX* v, int* n, DWORD dwcolor)
{
	// all points on sphere at origin
	// x^2 + y^2 + z^2 = r^2 
	// x^2 + y^2 + z^2 - r^2 = 0
	// add x, y, z to position sphere in 3D space
	int cnt = 0;
	double pi = 3.1415926535897932F;
	double repCount = (360.0F * m_fCamMove_degreeradian) / m_fCamMove_anglerotation;
	double* ox;
	double* oy;
	int num = (int)repCount;
	ox = (double*)malloc(num * sizeof(double));
	if (ox != NULL)
	{
		oy = (double*)malloc(num * sizeof(double));
		if (oy != NULL)
		{


			double angle = 0.0F;
			double distance;
			int half = 1 + num / 2;

			for (int i = 0; i < half; i++)
			{
				ox[i] = m_fCamMove_cameraradius * ((double)1.0 * cos(angle));
				oy[i] = sqrt(pow(m_fCamMove_cameraradius, (double)2.0) - pow(ox[i], (double)2.0));//*sin(angle);

				if (oy[i] != (double)0)
				{
					distance = sqrt(pow((ox[i - 1] - ox[i]), 2.0)
						+ pow((oy[i - 1] - oy[i]), 2.0));
					double radius = fabs(oy[i]);
					double circum = 2 * pi * radius;
					int rep = (int)(circum / distance);
					double zangle = (double)0.0;
					double addangle = ((double)360.0 / rep) * m_fCamMove_degreeradian;

					for (int j = 0; j < rep; j++)
					{
						cnt++;
						zangle += addangle;
					}// eo j for
				}// eo if i < 0
				else
				{
					cnt++;

				}
				angle += m_fCamMove_anglerotation;

			}// eo i for


			if (m_bArrayInit == true) {}

			///////////////////////////////////////
			v = (CUSTOMVERTEX*)malloc((cnt + 2) * sizeof(CUSTOMVERTEX));
			if (v != NULL)
			{
				m_bArrayInit = true;
				angle = 0.0F;
				cnt = 0;
				distance = 0.0F;

				for (int i = 0; i < half; i++)
				{
					ox[i] = m_fCamMove_cameraradius * (1.0 * cos(angle));
					oy[i] = sqrt(pow(m_fCamMove_cameraradius, 2.0) - pow(ox[i], 2.0));//*sin(angle);

					if (oy[i] != 0.0F)
					{
						distance = sqrt(pow((ox[i - 1] - ox[i]), 2.0)
							+ pow((oy[i - 1] - oy[i]), 2.0));
						double radius = fabs(oy[i]);
						double circum = 2 * pi * radius;
						int rep = (int)(circum / distance);
						double zangle = (double)0;
						double addangle = ((double)360.0 / rep) * m_fCamMove_degreeradian;

						for (int j = 0; j < rep; j++)
						{
							v[cnt].x = ((float)ox[i]);// +150.0f;
							v[cnt].y = (float)(radius * (1.0 * cos(zangle)));// +150.0f;
							v[cnt].z = (float)(radius * (1.0 * sin(zangle)));
							v[cnt].rhw = 1.0f;
							v[cnt].color = dwcolor;
							cnt++;
							zangle += addangle;
						}// eo j for
					}// eo if i < 0
					else
					{
						// First point of the sphere x axis						
						if (cnt == 0) {
							v[cnt].x = (float)ox[0];// +150.0F;
							v[cnt].y = 0.0f;// +150.0F;
							v[cnt].z = 0.0F;
							v[cnt].rhw = 1.0f;
							v[cnt].color = 0x00FF0000;// dwcolor;
							cnt++;
						}
					} // eo else
					angle += m_fCamMove_anglerotation;
				}// eo i for

				// Last point point of the sphere x axis
				float x = (float)-ox[0];
				v[cnt].x = x;// +150.0F;
				v[cnt].y = 0.0f;// +150.0F;
				v[cnt].z = 0.0F;
				v[cnt].rhw = 1.0f;
				v[cnt].color = 0x0000FF00;// dwcolor;
				cnt++;

				m_iCount = n[0] = cnt;

				free(ox);
				free(oy);
				return v;
			}//eo v  !=NULL
			free(ox);
		}free(oy);
	}// eo ox and oy != NULL
	free(v);
	return NULL;
}

VertexPositionColor* AppXamlDX12::Hot3dx12Rotate::CalculateSphereVPC(VertexPositionColor* v, int* n, DWORD dwcolor)
{

	// all points on sphere at origin
	// x^2 + y^2 + z^2 = r^2 
	// x^2 + y^2 + z^2 - r^2 = 0
	// add x, y, z to position sphere in 3D space
	int cnt = 0;
	double pi = 3.1415926535897932F;
	double repCount = (360.0F * m_fCamMove_degreeradian) / m_fCamMove_anglerotation;
	double* ox;
	double* oy;
	int num = (int)repCount;
	ox = (double*)malloc(num * sizeof(double));
	if (ox != NULL)
	{
		oy = (double*)malloc(num * sizeof(double));
		if (oy != NULL)
		{


			double angle = 0.0F;
			double distance;
			int half = 1 + num / 2;

			for (int i = 0; i < half; i++)
			{
				ox[i] = m_fCamMove_cameraradius * ((double)1.0 * cos(angle));
				oy[i] = sqrt(pow(m_fCamMove_cameraradius, (double)2.0) - pow(ox[i], (double)2.0));//*sin(angle);

				if (oy[i] != (double)0)
				{
					distance = sqrt(pow((ox[i - 1] - ox[i]), 2.0)
						+ pow((oy[i - 1] - oy[i]), 2.0));
					double radius = fabs(oy[i]);
					double circum = 2 * pi * radius;
					int rep = (int)(circum / distance);
					double zangle = (double)0.0;
					double addangle = ((double)360.0 / rep) * m_fCamMove_degreeradian;

					for (int j = 0; j < rep; j++)
					{
						cnt++;
						zangle += addangle;
					}// eo j for
				}// eo if i < 0
				else
				{
					cnt++;

				}
				angle += m_fCamMove_anglerotation;

			}// eo i for


			if (m_bArrayInit == true) {}

			///////////////////////////////////////
			v = (VertexPositionColor*)malloc((cnt + 2) * sizeof(VertexPositionColor));
			if (v != NULL)
			{
				m_bArrayInit = true;
				angle = 0.0F;
				cnt = 0;
				distance = 0.0F;

				for (int i = 0; i < half; i++)
				{
					ox[i] = m_fCamMove_cameraradius * (1.0 * cos(angle));
					oy[i] = sqrt(pow(m_fCamMove_cameraradius, 2.0) - pow(ox[i], 2.0));//*sin(angle);

					if (oy[i] != 0.0F)
					{
						distance = sqrt(pow((ox[i - 1] - ox[i]), 2.0)
							+ pow((oy[i - 1] - oy[i]), 2.0));
						double radius = fabs(oy[i]);
						double circum = 2 * pi * radius;
						int rep = (int)(circum / distance);
						double zangle = (double)0;
						double addangle = ((double)360.0 / rep) * m_fCamMove_degreeradian;

						for (int j = 0; j < rep; j++)
						{
							v[cnt].position.x = ((float)ox[i]);// +150.0f;
							v[cnt].position.y = (float)(radius * (1.0 * cos(zangle)));// +150.0f;
							v[cnt].position.z = (float)(radius * (1.0 * sin(zangle)));
							dwcolor = 0x090900FF; //Orangish?
							v[cnt].color = XMFLOAT4((float)GetRValue(dwcolor) * 0.00390625f, (float)GetGValue(dwcolor) * 0.00390625f, (float)GetBValue(dwcolor) * 0.00390625f, (float)GetCValue(dwcolor) * 0.00390625f);
							cnt++;
							zangle += addangle;
						}// eo j for
					}// eo if i < 0
					else
					{
						// First point of the sphere x axis						
						if (cnt == 0) {
							v[cnt].position.x = (float)ox[0];// +150.0F;
							v[cnt].position.y = 0.0f;// +150.0F;
							v[cnt].position.z = 0.0F;
							dwcolor = 0x00FF0000; // Blue
							v[cnt].color = XMFLOAT4((float)GetRValue(dwcolor) * 0.00390625f, (float)GetGValue(dwcolor) * 0.00390625f, (float)GetBValue(dwcolor) * 0.00390625f, (float)GetCValue(dwcolor) * 0.00390625f);// 0x000000FF; // Blue
							cnt++;
						}
					} // eo else
					angle += m_fCamMove_anglerotation;
				}// eo i for

				// Last point point of the sphere x axis
				float x = (float)-ox[0];
				v[cnt].position.x = x;// +150.0F;
				v[cnt].position.y = 0.0f;// +150.0F;
				v[cnt].position.z = 0.0F;
				dwcolor = 0x0000FF00; // Green// dwcolor;
				v[cnt].color = XMFLOAT4((float)GetRValue(dwcolor) * 0.00390625f, (float)GetGValue(dwcolor) * 0.00390625f, (float)GetBValue(dwcolor) * 0.00390625f, (float)GetCValue(dwcolor) * 0.00390625f);// 0x0000FF00; Green// dwcolor;
				cnt++;

				m_iCount = n[0] = cnt;

				free(ox);
				free(oy);
				return v;
			}//eo v  !=NULL
			free(ox);
		}free(oy);
	}// eo ox and oy != NULL
	free(v);
	return NULL;
}

void AppXamlDX12::Hot3dx12Rotate::MoveRotateCameraXY(int direction)
{
	throw ref new Platform::NotImplementedException();
}

void AppXamlDX12::Hot3dx12Rotate::MoveRotateCameraAny(float x, float y, float z)
{
	throw ref new Platform::NotImplementedException();
}

float AppXamlDX12::Hot3dx12Rotate::yCoordofXRot3(float y, float z, float radAngle) {
	return (float)((y * cos(radAngle)) + (z * sin(radAngle)));
}
//////////////////////////.......
float AppXamlDX12::Hot3dx12Rotate::zCoordofXRot3(float y, float z, float radAngle) {
	return (float)((z * cos(radAngle)) - (y * sin(radAngle)));
}

float AppXamlDX12::Hot3dx12Rotate::xCoordofYRot3(float x, float z, float a) {
	return (float)((x * cos(a)) + (z * sin(a)));
}

float AppXamlDX12::Hot3dx12Rotate::zCoordofYRot3(float x, float z, float a) {
	return (float)((z * cos(a)) - (x * sin(a)));
}

float AppXamlDX12::Hot3dx12Rotate::xCoordofZRot3(float x, float y, float radAngle) {
	//	return (float)((x* cos(a))+(y*(-sin(a))));
	return (float)((x * cos(radAngle)) - (y * sin(radAngle)));
}

float AppXamlDX12::Hot3dx12Rotate::yCoordofZRot3(float x, float y, float radAngle) {
	return (float)((x * sin(radAngle)) + (y * cos(radAngle)));
}

