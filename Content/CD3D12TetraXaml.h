﻿#pragma once

#include "..\Common\DeviceResources.h"
#include "Content\ShaderStructures.h"
#include "..\Common\StepTimer.h"

namespace AppXamlDX12
{
	// This sample renderer instantiates a basic rendering pipeline.
	class CD3D12TetraXaml
	{
	public:
		CD3D12TetraXaml(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		~CD3D12TetraXaml();
		void CreateDeviceDependentResources();
		void CreateWindowSizeDependentResources();
		void Update(DX::StepTimer const& timer);
		bool Render();
		VertexPositionColor4* InitVB2(VertexPositionColor4* sphereVerts, int* numVerts);

		void SaveState();
		// were private
		void LoadState();
		void Rotate(float radians);

		// eo were private

		void StartTracking();
		void TrackingUpdate(float positionX);
		void StopTracking();
		bool IsTracking() { return m_tracking; }
		void ReleaseDeviceDependentResources();

		//CCameraXYMoveRotation          m_CamXYMoveRotate;
		bool                           is3DVisible;

		ID3D12GraphicsCommandList* GetComList() { return m_commandList.Get(); }

	private:
		

	private:
		// Constant buffers must be 256-byte aligned.
		static const UINT c_alignedConstantBufferSize = (sizeof(ModelViewProjectionConstantBuffer) + 255) & ~255;

		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		// Direct3D resources for cube geometry.
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	m_commandList;
		Microsoft::WRL::ComPtr<ID3D12RootSignature>			m_rootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState>			m_pipelineState;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		m_cbvHeap;
		Microsoft::WRL::ComPtr<ID3D12Resource>				m_vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D12Resource>				m_indexBuffer;
		Microsoft::WRL::ComPtr<ID3D12Resource>				m_constantBuffer;
		ModelViewProjectionConstantBuffer					m_constantBufferData;
		UINT8* m_mappedConstantBuffer;
		UINT												m_cbvDescriptorSize;
		D3D12_RECT											m_scissorRect;
		std::vector<byte>									m_vertexShader;
		std::vector<byte>									m_pixelShader;
		D3D12_VERTEX_BUFFER_VIEW							m_vertexBufferView;
		D3D12_INDEX_BUFFER_VIEW								m_indexBufferView;

		// Variables used with the rendering loop.
		bool	m_loadingComplete;
		float	m_radiansPerSecond;
		float	m_angle;
		bool	m_tracking;
		UINT    sceneVertexCount;
	};
}