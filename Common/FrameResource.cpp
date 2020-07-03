//--------------------------------------------------------------------------------------
// File: 
//
// Copyright (c) Jeff Kubitz - hot3dx. All rights reserved.
// 
//
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "FrameResource.h"

FrameResource::FrameResource(ID3D12Device* pDevice, UINT groupRowCount, UINT groupColumnCount) : 
    m_fenceValue(0),
    m_groupRowCount(groupRowCount),
    m_groupColumnCount(groupColumnCount)
{
    m_modelMatrices.resize(m_groupRowCount * m_groupColumnCount);

    // The command allocator is used by the main sample class when 
    // resetting the command list in the main update loop. Each frame 
    // resource needs a command allocator because command allocators 
    // cannot be reused until the GPU is done executing the commands 
    // associated with it.
    DX::ThrowIfFailed(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
    DX::ThrowIfFailed(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS(&m_bundleAllocator)));

    // Create an upload heap for the constant buffers.
    DX::ThrowIfFailed(pDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(sizeof(SceneConstantBuffer) * m_groupRowCount * m_groupColumnCount),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_cbvUploadHeap)));

    // Map the constant buffers. Note that unlike D3D11, the resource 
    // does not need to be unmapped for use by the GPU. In this sample, 
    // the resource stays 'permenantly' mapped to avoid overhead with 
    // mapping/unmapping each frame.
    CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
    DX::ThrowIfFailed(m_cbvUploadHeap->Map(0, &readRange, reinterpret_cast<void**>(&m_pConstantBuffers)));

    // Update all of the model matrices once; our cities don't move so 
    // we don't need to do this ever again.
    SetgroupPositions(8.0f, -8.0f);
}

FrameResource::FrameResource(ID3D12Device* pDevice, 
    ID3D12PipelineState* pPso, 
    ID3D12PipelineState* pGridMapPso, 
    ID3D12PipelineState* pTetraMapPso,
    ID3D12DescriptorHeap* pDsvHeap, 
    ID3D12DescriptorHeap* pCbvSrvHeap, 
    D3D12_VIEWPORT* pViewport, 
    UINT frameResourceIndex) :
    m_fenceValue(0),
    m_pipelineState(pPso),
    m_pipelineStateGridMap(pGridMapPso),
    m_pipelineStateTetraMap(pTetraMapPso)
{
    for (UINT i = 0; i < DX::CommandListCount; i++)
    {
        DX::ThrowIfFailed(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[i])));
        DX::ThrowIfFailed(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[i].Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandLists[i])));

        DX::NAME_D3D12_OBJECT_INDEXED(m_commandLists, i);

        // Close these command lists; don't record into them for now.
        DX::ThrowIfFailed(m_commandLists[i]->Close());
    }

    for (UINT i = 0; i < DX::NumContexts; i++)
    {
        // Create command list allocators for worker threads. One alloc is 
        // for the shadow pass command list, and one is for the scene pass.
        DX::ThrowIfFailed(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_gridCommandAllocators[i])));
        DX::ThrowIfFailed(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_tetraCommandAllocators[i])));
        DX::ThrowIfFailed(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_sceneCommandAllocators[i])));

        DX::ThrowIfFailed(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_gridCommandAllocators[i].Get(), m_pipelineStateGridMap.Get(), IID_PPV_ARGS(&m_gridCommandLists[i])));
        DX::ThrowIfFailed(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_tetraCommandAllocators[i].Get(), m_pipelineStateTetraMap.Get(), IID_PPV_ARGS(&m_tetraCommandLists[i])));
        DX::ThrowIfFailed(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_sceneCommandAllocators[i].Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_sceneCommandLists[i])));

        DX::NAME_D3D12_OBJECT_INDEXED(m_gridCommandLists, i);
        DX::NAME_D3D12_OBJECT_INDEXED(m_tetraCommandLists, i);
        DX::NAME_D3D12_OBJECT_INDEXED(m_sceneCommandLists, i);

        // Close these command lists; don't record into them for now. We will 
        // reset them to a recording state when we start the render loop.
        DX::ThrowIfFailed(m_gridCommandLists[i]->Close());
        DX::ThrowIfFailed(m_tetraCommandLists[i]->Close());
        DX::ThrowIfFailed(m_sceneCommandLists[i]->Close());
    }

    // Describe and create the grid map texture.
    CD3DX12_RESOURCE_DESC gridTexDesc(
        D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        0,
        static_cast<UINT>(pViewport->Width),
        static_cast<UINT>(pViewport->Height),
        1,
        1,
        DXGI_FORMAT_R32_TYPELESS,
        1,
        0,
        D3D12_TEXTURE_LAYOUT_UNKNOWN,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    D3D12_CLEAR_VALUE clearValue;        // Performance tip: Tell the runtime at resource creation the desired clear value.
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    DX::ThrowIfFailed(pDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &gridTexDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(&m_gridTexture)));

    DX::NAME_D3D12_OBJECT(m_gridTexture);

    // Get a handle to the start of the descriptor heap then offset 
    // it based on the frame resource index.
    const UINT dsvDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE depthHandle(pDsvHeap->GetCPUDescriptorHandleForHeapStart(), 1 + frameResourceIndex, dsvDescriptorSize); // + 1 for the shadow map.

    // Describe and create the shadow depth view and cache the CPU 
    // descriptor handle.
    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
    depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.Texture2D.MipSlice = 0;
    pDevice->CreateDepthStencilView(m_gridTexture.Get(), &depthStencilViewDesc, depthHandle);
    m_gridDepthView = depthHandle;

    // Get a handle to the start of the descriptor heap then offset it 
    // based on the existing textures and the frame resource index. Each 
    // frame has 1 SRV (shadow tex) and 2 CBVs.
    const UINT nullSrvCount = 2;                                // Null descriptors at the start of the heap.
    const UINT textureCount = 1;// _countof(SampleAssets::Textures);    // Diffuse + normal textures near the start of the heap.  Ideally, track descriptor heap contents/offsets at a higher level.
    const UINT cbvSrvDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvCpuHandle(pCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
    CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvGpuHandle(pCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
    m_nullSrvHandle = cbvSrvGpuHandle;
    cbvSrvCpuHandle.Offset(nullSrvCount + textureCount + (frameResourceIndex * DX::c_frameCount), cbvSrvDescriptorSize);
    cbvSrvGpuHandle.Offset(nullSrvCount + textureCount + (frameResourceIndex * DX::c_frameCount), cbvSrvDescriptorSize);

    // Describe and create a shader resource view (SRV) for the shadow depth 
    // texture and cache the GPU descriptor handle. This SRV is for sampling 
    // the shadow map from our shader. It uses the same texture that we use 
    // as a depth-stencil during the shadow pass.
    D3D12_SHADER_RESOURCE_VIEW_DESC gridSrvDesc = {};
    gridSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    gridSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    gridSrvDesc.Texture2D.MipLevels = 1;
    gridSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    pDevice->CreateShaderResourceView(m_gridTexture.Get(), &gridSrvDesc, cbvSrvCpuHandle);
    m_gridDepthHandle = cbvSrvGpuHandle;

    // Increment the descriptor handles.
    cbvSrvCpuHandle.Offset(cbvSrvDescriptorSize);
    cbvSrvGpuHandle.Offset(cbvSrvDescriptorSize);

    // Create the constant buffers.
    const UINT constantBufferSize = (sizeof(SceneConstantBufferMulti) + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1); // must be a multiple 256 bytes
    DX::ThrowIfFailed(pDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_gridConstantBuffer)));
    DX::ThrowIfFailed(pDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_sceneConstantBuffer)));

    // Map the constant buffers and cache their heap pointers.
    CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
    DX::ThrowIfFailed(m_gridConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mp_gridConstantBufferWO)));
    DX::ThrowIfFailed(m_sceneConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mp_sceneConstantBufferWO)));

    // Create the constant buffer views: one for the Grid pass and
    // another for the scene pass.
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.SizeInBytes = constantBufferSize;

    // Describe and create the grid constant buffer view (CBV) and 
    // cache the GPU descriptor handle.
    cbvDesc.BufferLocation = m_gridConstantBuffer->GetGPUVirtualAddress();
    pDevice->CreateConstantBufferView(&cbvDesc, cbvSrvCpuHandle);
    m_gridCbvHandle = cbvSrvGpuHandle;

    // Increment the descriptor handles.
    cbvSrvCpuHandle.Offset(cbvSrvDescriptorSize);
    cbvSrvGpuHandle.Offset(cbvSrvDescriptorSize);

    ///////////////////////////////////////////////
    //////////// tetra resources
    /////////////////////////////////////////////////////

        // Describe and create the grid map texture.
    CD3DX12_RESOURCE_DESC tetraTexDesc(
        D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        0,
        static_cast<UINT>(pViewport->Width),
        static_cast<UINT>(pViewport->Height),
        1,
        1,
        DXGI_FORMAT_R32_TYPELESS,
        1,
        0,
        D3D12_TEXTURE_LAYOUT_UNKNOWN,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    D3D12_CLEAR_VALUE clearValue2;        // Performance tip: Tell the runtime at resource creation the desired clear value.
    clearValue2.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue2.DepthStencil.Depth = 1.0f;
    clearValue2.DepthStencil.Stencil = 0;

    DX::ThrowIfFailed(pDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &tetraTexDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue2,
        IID_PPV_ARGS(&m_tetraTexture)));

    DX::NAME_D3D12_OBJECT(m_tetraTexture);

    // Get a handle to the start of the descriptor heap then offset 
    // it based on the frame resource index.
    const UINT dsvDescriptorSize2 = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE depthHandle2(pDsvHeap->GetCPUDescriptorHandleForHeapStart(), 1 + frameResourceIndex, dsvDescriptorSize2); // + 1 for the shadow map.

    // Describe and create the shadow depth view and cache the CPU 
    // descriptor handle.
    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc2 = {};
    depthStencilViewDesc2.Format = DXGI_FORMAT_D32_FLOAT;
    depthStencilViewDesc2.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc2.Texture2D.MipSlice = 0;
    pDevice->CreateDepthStencilView(m_tetraTexture.Get(), &depthStencilViewDesc2, depthHandle2);
    m_tetraDepthView = depthHandle2;

    // Get a handle to the start of the descriptor heap then offset it 
    // based on the existing textures and the frame resource index. Each 
    // frame has 1 SRV (shadow tex) and 2 CBVs.
    const UINT nullSrvCount2 = 2;                                // Null descriptors at the start of the heap.
    const UINT textureCount2 = 1;// _countof(SampleAssets::Textures);    // Diffuse + normal textures near the start of the heap.  Ideally, track descriptor heap contents/offsets at a higher level.
    const UINT cbvSrvDescriptorSize2 = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvCpuHandle2(pCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
    CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvGpuHandle2(pCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
    m_nullSrvHandle = cbvSrvGpuHandle2;
    cbvSrvCpuHandle.Offset(nullSrvCount2 + textureCount2 + (frameResourceIndex * DX::c_frameCount), cbvSrvDescriptorSize2);
    cbvSrvGpuHandle.Offset(nullSrvCount2 + textureCount2 + (frameResourceIndex * DX::c_frameCount), cbvSrvDescriptorSize2);

    // Describe and create a shader resource view (SRV) for the shadow depth 
    // texture and cache the GPU descriptor handle. This SRV is for sampling 
    // the shadow map from our shader. It uses the same texture that we use 
    // as a depth-stencil during the shadow pass.
    D3D12_SHADER_RESOURCE_VIEW_DESC tetraSrvDesc = {};
    tetraSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    tetraSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    tetraSrvDesc.Texture2D.MipLevels = 1;
    tetraSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    pDevice->CreateShaderResourceView(m_tetraTexture.Get(), &tetraSrvDesc, cbvSrvCpuHandle2);
    m_tetraDepthHandle = cbvSrvGpuHandle2;

    // Increment the descriptor handles.
    cbvSrvCpuHandle2.Offset(cbvSrvDescriptorSize2);
    cbvSrvGpuHandle2.Offset(cbvSrvDescriptorSize2);

    // Create the constant buffers.
    const UINT constantBufferSize2 = (sizeof(SceneConstantBufferMulti) + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1); // must be a multiple 256 bytes
    DX::ThrowIfFailed(pDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize2),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_tetraConstantBuffer)));
    DX::ThrowIfFailed(pDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize2),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_sceneConstantBuffer)));

    // Map the constant buffers and cache their heap pointers.
    CD3DX12_RANGE readRange2(0, 0);        // We do not intend to read from this resource on the CPU.
    DX::ThrowIfFailed(m_tetraConstantBuffer->Map(0, &readRange2, reinterpret_cast<void**>(&mp_tetraConstantBufferWO)));
    DX::ThrowIfFailed(m_sceneConstantBuffer->Map(0, &readRange2, reinterpret_cast<void**>(&mp_sceneConstantBufferWO)));

    // Create the constant buffer views: one for the Tetra pass and
    // another for the scene pass.
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc2 = {};
    cbvDesc2.SizeInBytes = constantBufferSize2;

    ///////////////////////////////////////////////////////////////////
    ///// eo tetra resources
    ////////////////////////////////////////////////////////////////////
    // Describe and create the tetra constant buffer view (CBV) and 
    // cache the GPU descriptor handle.
    cbvDesc.BufferLocation = m_tetraConstantBuffer->GetGPUVirtualAddress();
    pDevice->CreateConstantBufferView(&cbvDesc, cbvSrvCpuHandle);
    m_tetraCbvHandle = cbvSrvGpuHandle;

    // Increment the descriptor handles.
    cbvSrvCpuHandle.Offset(cbvSrvDescriptorSize);
    cbvSrvGpuHandle.Offset(cbvSrvDescriptorSize);

    // Describe and create the scene constant buffer view (CBV) and 
    // cache the GPU descriptor handle.
    cbvDesc.BufferLocation = m_sceneConstantBuffer->GetGPUVirtualAddress();
    pDevice->CreateConstantBufferView(&cbvDesc, cbvSrvCpuHandle);
    m_sceneCbvHandle = cbvSrvGpuHandle;

    // Batch up grid command lists for execution later.
    const UINT batchSize = _countof(m_sceneCommandLists) + _countof(m_gridCommandLists) + 3;
    m_batchSubmit[0] = m_commandLists[DX::CommandListPre].Get();
    memcpy(m_batchSubmit + 1, m_gridCommandLists, _countof(m_gridCommandLists) * sizeof(ID3D12CommandList*));
    m_batchSubmit[_countof(m_gridCommandLists) + 1] = m_commandLists[DX::CommandListMid].Get();
    memcpy(m_batchSubmit + _countof(m_gridCommandLists) + 2, m_sceneCommandLists, _countof(m_sceneCommandLists) * sizeof(ID3D12CommandList*));
    m_batchSubmit[batchSize - 1] = m_commandLists[DX::CommandListPost].Get();

    // Batch up tetra command lists for execution later.
    const UINT batchSize2 = _countof(m_sceneCommandLists) + _countof(m_tetraCommandLists) + 3;
    m_batchSubmit[0] = m_commandLists[DX::CommandListPre].Get();
    memcpy(m_batchSubmit + 1, m_tetraCommandLists, _countof(m_tetraCommandLists) * sizeof(ID3D12CommandList*));
    m_batchSubmit[_countof(m_tetraCommandLists) + 1] = m_commandLists[DX::CommandListMid].Get();
    memcpy(m_batchSubmit + _countof(m_tetraCommandLists) + 2, m_sceneCommandLists, _countof(m_sceneCommandLists) * sizeof(ID3D12CommandList*));
    m_batchSubmit[batchSize2 - 1] = m_commandLists[DX::CommandListPost].Get();

}

FrameResource::~FrameResource()
{
    // Bundle
    m_cbvUploadHeap->Unmap(0, nullptr);
    m_pConstantBuffers = nullptr;

    // MultiThreaded
    for (int i = 0; i < DX::CommandListCount; i++)
    {
        m_commandAllocators[i] = nullptr;
        m_commandLists[i] = nullptr;
    }

    m_gridConstantBuffer = nullptr;
    m_tetraConstantBuffer = nullptr;
    m_sceneConstantBuffer = nullptr;

    for (int i = 0; i < DX::NumContexts; i++)
    {
        m_gridCommandLists[i] = nullptr;
        m_gridCommandAllocators[i] = nullptr;

        m_tetraCommandLists[i] = nullptr;
        m_tetraCommandAllocators[i] = nullptr;

        m_sceneCommandLists[i] = nullptr;
        m_sceneCommandAllocators[i] = nullptr;
    }

    m_gridTexture = nullptr;
    m_tetraTexture = nullptr;
}

void FrameResource::Bind(ID3D12GraphicsCommandList* pCommandList, BOOL scenePass, D3D12_CPU_DESCRIPTOR_HANDLE* pRtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE* pDsvHandle)
{
}

void FrameResource::Init()
{
}

void FrameResource::SwapBarriers()
{
}

void FrameResource::Finish()
{
}

void FrameResource::WriteConstantBuffers(D3D12_VIEWPORT* pViewport, SimpleCamera* pSceneCamera, SimpleCamera lightCams[DX::NumLights], LightState lights[DX::NumLights])
{
}

void FrameResource::InitBundle(ID3D12Device* pDevice, ID3D12PipelineState* pPso1, ID3D12PipelineState* pPso2,
    UINT frameResourceIndex, UINT numIndices, D3D12_INDEX_BUFFER_VIEW* pIndexBufferViewDesc, D3D12_VERTEX_BUFFER_VIEW* pVertexBufferViewDesc,
    ID3D12DescriptorHeap* pCbvSrvDescriptorHeap, UINT cbvSrvDescriptorSize, ID3D12DescriptorHeap* pSamplerDescriptorHeap, ID3D12RootSignature* pRootSignature)
{
    DX::ThrowIfFailed(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, m_bundleAllocator.Get(), pPso1, IID_PPV_ARGS(&m_bundle)));
    DX::NAME_D3D12_OBJECT(m_bundle);

    PopulateCommandList(m_bundle.Get(), pPso1, pPso2, frameResourceIndex, numIndices, pIndexBufferViewDesc,
        pVertexBufferViewDesc, pCbvSrvDescriptorHeap, cbvSrvDescriptorSize, pSamplerDescriptorHeap, pRootSignature);

    DX::ThrowIfFailed(m_bundle->Close());
}

void FrameResource::SetgroupPositions(FLOAT intervalX, FLOAT intervalZ)
{
    for (UINT i = 0; i < m_groupRowCount; i++)
    {
        FLOAT groupOffsetZ = i * intervalZ;
        for (UINT j = 0; j < m_groupColumnCount; j++)
        {
            FLOAT groupOffsetX = j * intervalX;

            // The y position is based off of the group's row and column 
            // position to prevent z-fighting.
            XMStoreFloat4x4(&m_modelMatrices[i * m_groupColumnCount + j], XMMatrixTranslation(groupOffsetX, 0.02f * (i * m_groupColumnCount + j), groupOffsetZ));
        }
    }
}

void FrameResource::PopulateCommandList(ID3D12GraphicsCommandList* pCommandList, ID3D12PipelineState* pPso1, ID3D12PipelineState* pPso2,
    UINT frameResourceIndex, UINT numIndices, D3D12_INDEX_BUFFER_VIEW* pIndexBufferViewDesc, D3D12_VERTEX_BUFFER_VIEW* pVertexBufferViewDesc,
    ID3D12DescriptorHeap* pCbvSrvDescriptorHeap, UINT cbvSrvDescriptorSize, ID3D12DescriptorHeap* pSamplerDescriptorHeap, ID3D12RootSignature* pRootSignature)
{
    // If the root signature matches the root signature of the caller, then
    // bindings are inherited, otherwise the bind space is reset.
    pCommandList->SetGraphicsRootSignature(pRootSignature);

    ID3D12DescriptorHeap* ppHeaps[] = { pCbvSrvDescriptorHeap, pSamplerDescriptorHeap };
    pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pCommandList->IASetIndexBuffer(pIndexBufferViewDesc);
    pCommandList->IASetVertexBuffers(0, 1, pVertexBufferViewDesc);
    pCommandList->SetGraphicsRootDescriptorTable(0, pCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    pCommandList->SetGraphicsRootDescriptorTable(1, pSamplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

    // Calculate the descriptor offset due to multiple frame resources.
    // 1 SRV + how many CBVs we have currently.
    UINT frameResourceDescriptorOffset = 1 + (frameResourceIndex * m_groupRowCount * m_groupColumnCount);
    CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandle(pCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), frameResourceDescriptorOffset, cbvSrvDescriptorSize);

    PIXBeginEvent(pCommandList, 0, L"Draw cities");
    BOOL usePso1 = TRUE;
    for (UINT i = 0; i < m_groupRowCount; i++)
    {
        for (UINT j = 0; j < m_groupColumnCount; j++)
        {
            // Alternate which PSO to use; the pixel shader is different on 
            // each just as a PSO setting demonstration.
            pCommandList->SetPipelineState(usePso1 ? pPso1 : pPso2);
            usePso1 = !usePso1;

            // Set this group's CBV table and move to the next descriptor.
            pCommandList->SetGraphicsRootDescriptorTable(2, cbvSrvHandle);
            cbvSrvHandle.Offset(cbvSrvDescriptorSize);

            pCommandList->DrawIndexedInstanced(numIndices, 1, 0, 0, 0);
        }
    }
    PIXEndEvent(pCommandList);
}

void XM_CALLCONV FrameResource::UpdateConstantBuffers(FXMMATRIX view, CXMMATRIX projection)
{
    XMMATRIX model;
    XMFLOAT4X4 mvp;

    for (UINT i = 0; i < m_groupRowCount; i++)
    {
        for (UINT j = 0; j < m_groupColumnCount; j++)
        {
            model = XMLoadFloat4x4(&m_modelMatrices[i * m_groupColumnCount + j]);

            // Compute the model-view-projection matrix.
            XMStoreFloat4x4(&mvp, XMMatrixTranspose(model * view * projection));

            // Copy this matrix into the appropriate location in the upload heap subresource.
            memcpy(&m_pConstantBuffers[i * m_groupColumnCount + j], &mvp, sizeof(mvp));
        }
    }
}
