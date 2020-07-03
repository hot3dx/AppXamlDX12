//--------------------------------------------------------------------------------------
// File: 
//
// Copyright (c) Jeff Kubitz - hot3dx. All rights reserved.
// 
//
//--------------------------------------------------------------------------------------

#pragma once

#include "DirectXHelper.h"
#include "SimpleCamera.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;


struct LightState
{
    XMFLOAT4 position;
    XMFLOAT4 direction;
    XMFLOAT4 color;
    XMFLOAT4 falloff;
    XMFLOAT4X4 view;
    XMFLOAT4X4 projection;
};

struct SceneConstantBuffer
{
    XMFLOAT4X4 mvp;        // Model-view-projection (MVP) matrix.
    FLOAT padding[48];
};

struct SceneConstantBufferMulti
{
    XMFLOAT4X4 model;
    XMFLOAT4X4 view;
    XMFLOAT4X4 projection;
    XMFLOAT4 ambientColor;
    BOOL sampleShadowMap;
    BOOL padding[3];        // Must be aligned to be made up of N float4s.
    LightState lights[DX::NumLights];
};

class FrameResource
{

public:
    

    FrameResource(ID3D12Device* pDevice, UINT groupRowCount, UINT groupColumnCount);
    FrameResource(ID3D12Device* pDevice, ID3D12PipelineState* pPso, ID3D12PipelineState* pGridMapPso, ID3D12PipelineState* pTetraMapPso, ID3D12DescriptorHeap* pDsvHeap, ID3D12DescriptorHeap* pCbvSrvHeap, D3D12_VIEWPORT* pViewport, UINT frameResourceIndex);
     ~FrameResource();
     void Bind(ID3D12GraphicsCommandList* pCommandList, BOOL scenePass, D3D12_CPU_DESCRIPTOR_HANDLE* pRtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE* pDsvHandle);
     void Init();
     void SwapBarriers();
     void Finish();
     void WriteConstantBuffers(D3D12_VIEWPORT* pViewport, SimpleCamera* pSceneCamera, SimpleCamera lightCams[DX::NumLights], LightState lights[DX::NumLights]);

     // Bundle CommandAllocators and CommandLists
     ComPtr<ID3D12CommandAllocator> m_commandAllocator;// s[DX::CommandListCount];
     ComPtr<ID3D12GraphicsCommandList> m_commandList;// [DX::CommandListCount] ;

     ComPtr<ID3D12CommandAllocator> m_bundleAllocator;// [DX::NumContexts] ;
     ComPtr<ID3D12GraphicsCommandList> m_bundle;// [DX::NumContexts] ;

     // MultiThread CommandAllocators and CommandLists

    ID3D12CommandList* m_batchSubmit[DX::NumContexts * 2 + DX::CommandListCount];

    ComPtr<ID3D12CommandAllocator> m_commandAllocators[DX::CommandListCount] ;
    ComPtr<ID3D12GraphicsCommandList> m_commandLists[DX::CommandListCount];

    ComPtr<ID3D12CommandAllocator> m_tetraCommandAllocators[DX::NumContexts] ;
    ComPtr<ID3D12GraphicsCommandList> m_tetraCommandLists[DX::NumContexts] ;

    ComPtr<ID3D12CommandAllocator> m_gridCommandAllocators[DX::NumContexts];
    ComPtr<ID3D12GraphicsCommandList> m_gridCommandLists[DX::NumContexts];

    ComPtr<ID3D12CommandAllocator> m_sceneCommandAllocators[DX::NumContexts];
    ComPtr<ID3D12GraphicsCommandList> m_sceneCommandLists[DX::NumContexts];

private:
    ComPtr<ID3D12Resource> m_cbvUploadHeap;
    SceneConstantBuffer* m_pConstantBuffers;
    UINT64 m_fenceValue;

    

private:
    ComPtr<ID3D12PipelineState> m_pipelineState;
    ComPtr<ID3D12PipelineState> m_pipelineStateGridMap;
    ComPtr<ID3D12PipelineState> m_pipelineStateTetraMap;
    ComPtr<ID3D12Resource> m_gridTexture;
    ComPtr<ID3D12Resource> m_tetraTexture;
    D3D12_CPU_DESCRIPTOR_HANDLE m_gridDepthView;
    D3D12_CPU_DESCRIPTOR_HANDLE m_tetraDepthView;
    ComPtr<ID3D12Resource> m_gridConstantBuffer;
    ComPtr<ID3D12Resource> m_tetraConstantBuffer;
    ComPtr<ID3D12Resource> m_sceneConstantBuffer;
    SceneConstantBufferMulti* mp_gridConstantBufferWO;
    SceneConstantBufferMulti* mp_tetraConstantBufferWO;
    SceneConstantBufferMulti* mp_sceneConstantBufferWO;
    D3D12_GPU_DESCRIPTOR_HANDLE m_nullSrvHandle;    // Null SRV for out of bounds behavior.
    D3D12_GPU_DESCRIPTOR_HANDLE m_gridDepthHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE m_gridCbvHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE m_tetraDepthHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE m_tetraCbvHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE m_sceneDepthHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE m_sceneCbvHandle;


    std::vector<XMFLOAT4X4> m_modelMatrices;
    UINT m_groupRowCount;
    UINT m_groupColumnCount;

    std::vector<XMFLOAT4X4> m_gridMatrix;

    

    void InitBundle(ID3D12Device* pDevice, ID3D12PipelineState* pPso1, ID3D12PipelineState* pPso2,
        UINT frameResourceIndex, UINT numIndices, D3D12_INDEX_BUFFER_VIEW* pIndexBufferViewDesc, D3D12_VERTEX_BUFFER_VIEW* pVertexBufferViewDesc,
        ID3D12DescriptorHeap* pCbvSrvDescriptorHeap, UINT cbvSrvDescriptorSize, ID3D12DescriptorHeap* pSamplerDescriptorHeap, ID3D12RootSignature* pRootSignature);

    void PopulateCommandList(ID3D12GraphicsCommandList* pCommandList, ID3D12PipelineState* pPso1, ID3D12PipelineState* pPso2,
        UINT frameResourceIndex, UINT numIndices, D3D12_INDEX_BUFFER_VIEW* pIndexBufferViewDesc, D3D12_VERTEX_BUFFER_VIEW* pVertexBufferViewDesc,
        ID3D12DescriptorHeap* pCbvSrvDescriptorHeap, UINT cbvSrvDescriptorSize, ID3D12DescriptorHeap* pSamplerDescriptorHeap, ID3D12RootSignature* pRootSignature);

    void XM_CALLCONV UpdateConstantBuffers(FXMMATRIX view, CXMMATRIX projection);

private:
    void SetgroupPositions(FLOAT intervalX, FLOAT intervalZ);

};
