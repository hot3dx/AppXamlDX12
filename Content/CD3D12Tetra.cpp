#include "pch.h" 
#include "Common\DeviceResources.h"
#include "Common\DirectXHelper.h"
#include "CD3D12Tetra.h"

#include "shader_mesh_simple_pixel.hlsl"
#include "shader_mesh_simple_vert.hlsl"

using namespace DX;

CD3D12Tetra::CD3D12Tetra(DeviceResources* parent)
	: m_pTetraFrameIndex(0),
	m_pTetraCbSrvDataBegin(nullptr),
	//m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
	//m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
	m_pTetraFenceValues{},
	m_pTetrartvDescriptorSize(0),
	m_pTetracbSrvDescriptorSize(0),
	m_pTetraRendermode{ 1 }
{
	return;
}

CD3D12Tetra::~CD3D12Tetra()
{

	free(m_pTetraVB);
	free(m_pSysMemTetraVB);
}

void CD3D12Tetra::LoadTetraPipeline(DeviceResources* parent, ComPtr<ID3D12Device> dev12)
{
	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(dev12->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_TetraCommandQueue)));
	NAME_D3D12_OBJECT(m_TetraCommandQueue);

	m_pTetraFrameIndex = parent->GetSwapChain()->GetCurrentBackBufferIndex();

	// Create descriptor heaps.
	{
		// Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC m_pTetrartvHeapDesc = {};
		m_pTetrartvHeapDesc.NumDescriptors = m_pTetraFrameCount + 2;    // swap chain back buffers + 1 intermediate UI render buffer + 1 intermediate scene buffer
		m_pTetrartvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		m_pTetrartvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(dev12->CreateDescriptorHeap(&m_pTetrartvHeapDesc, IID_PPV_ARGS(&m_pTetrartvHeap)));

		m_pTetrartvDescriptorSize = dev12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// Describe and create a constant buffer view and shader resource view descriptor heap.
		// Flags indicate that this descriptor heap can be bound to the pipeline 
		// and that descriptors contained in it can be referenced by a root table.
		D3D12_DESCRIPTOR_HEAP_DESC m_pTetracbvHeapDesc = {};
		m_pTetracbvHeapDesc.NumDescriptors = 3;  // 1 constant buffer and 2 SRV. 
		m_pTetracbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		m_pTetracbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		ThrowIfFailed(dev12->CreateDescriptorHeap(&m_pTetracbvHeapDesc, IID_PPV_ARGS(&m_pTetracbSrvHeap)));

		m_pTetracbSrvDescriptorSize = dev12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	// Create a command allocator for each back buffer in the swapchain.
	for (UINT n = 0; n < m_pTetraFrameCount; ++n)
	{
		ThrowIfFailed(dev12->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pTetraCommandAllocators[n])));
	}
}

HRESULT CD3D12Tetra::LoadTetraAssets(DeviceResources* parent, ComPtr<ID3D12Device> dev12, UINT m_width, UINT m_height)
{
	// Create root signatures.
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

		// This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

		if (FAILED(dev12->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
		CD3DX12_ROOT_PARAMETER1 rootParameters[1];

		// Root signature for render pass1.
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE);
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);

		// Allow input layout and deny uneccessary access to certain pipeline stages.
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
		ThrowIfFailed(dev12->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_pTetraRenderPass1RootSignature)));
	
	   
	}

	// Create the pipeline state, which includes compiling and loading shaders.
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

		// Define the vertex input layout for render pass 1. 
		D3D12_INPUT_ELEMENT_DESC t_renderPass1InputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC renderPass1PSODesc = {};
		renderPass1PSODesc.InputLayout = { t_renderPass1InputElementDescs, _countof(t_renderPass1InputElementDescs) };
		renderPass1PSODesc.pRootSignature = m_pTetraRenderPass1RootSignature.Get();
		renderPass1PSODesc.VS = { PSMain };// , sizeof(PSMain) };
		renderPass1PSODesc.PS = { PSMain };// , sizeof(PSMain) };
		renderPass1PSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		renderPass1PSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		renderPass1PSODesc.DepthStencilState.DepthEnable = FALSE;
		renderPass1PSODesc.DepthStencilState.StencilEnable = FALSE;
		renderPass1PSODesc.SampleMask = UINT_MAX;
		renderPass1PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		renderPass1PSODesc.NumRenderTargets = 1;
		renderPass1PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		renderPass1PSODesc.SampleDesc.Count = 1;
		ThrowIfFailed(dev12->CreateGraphicsPipelineState(&renderPass1PSODesc, IID_PPV_ARGS(&m_pTetrarenderPass1PSO)));
		NAME_D3D12_OBJECT(m_pTetrarenderPass1PSO);


		// Create the command list.
		ThrowIfFailed(dev12->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pTetraCommandAllocators[m_pTetraFrameIndex].Get(), nullptr, IID_PPV_ARGS(&m_pTetraCommandList)));
		NAME_D3D12_OBJECT(m_pTetraCommandList);
	}
	// Create a constant buffer.
	{
		ThrowIfFailed(dev12->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(1024 * 64),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_pTetraConstantBuffer)));

		// Describe and create a constant buffer view.
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_pTetraConstantBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = (sizeof(parent->GetSceneConstantBuffer()) + 255) & ~255;    // CB size is required to be 256-byte aligned.
		CD3DX12_CPU_DESCRIPTOR_HANDLE cbHandle(m_pTetracbSrvHeap->GetCPUDescriptorHandleForHeapStart());
		dev12->CreateConstantBufferView(&cbvDesc, cbHandle);

		// Map and initialize the constant buffer. We don't unmap this until the
		// app closes. Keeping things mapped for the lifetime of the resource is okay.
		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_pTetraConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pTetraCbSrvDataBegin)));
		memcpy(m_pTetraCbSrvDataBegin, &parent->GetSceneConstantBuffer(), sizeof(parent->GetSceneConstantBuffer()));
	}

	LoadTetra(dev12, m_width, m_height);

	// Close the command list and execute it to begin the vertex buffer copy into
	// the default heap.
	ThrowIfFailed(m_pTetraCommandList->Close());
	ID3D12CommandList* ppCommandLists[] = { m_pTetraCommandList.Get() };
	m_TetraCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Create synchronization objects and wait until assets have been uploaded to the GPU.
	{
		ThrowIfFailed(dev12->CreateFence(m_pTetraFenceValues[m_pTetraFrameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pTetraFence)));
		m_pTetraFenceValues[m_pTetraFrameIndex]++;

		// Create an event handle to use for frame synchronization.
		m_pTetraFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_pTetraFenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}

		// Wait for the command list to execute; we are reusing the same command 
		// list in our main loop but for now, we just want to wait for setup to 
		// complete before continuing.
		//WaitForGpu(parent);
	}
	return S_OK;
}

void CD3D12Tetra::OnUpdate(DeviceResources* parent)
{
	static float time = 0;
	//parent->GetSceneConstantBuffer().orthProjMatrix = XMMatrixTranspose(XMMatrixOrthographicLH(2.0f * m_aspectRatio, 2.0f, 0.0f, 1.0f));  // Transpose from row-major to col-major, which by default is used in HLSL.
	m_pTetraRendermode = parent->GetRenderMode();
	//parent->GetSceneConstantBuffer().laneSize = m_WaveIntrinsicsSupport.WaveLaneCountMin;
	//parent->GetSceneConstantBuffer().time = time;
	//parent->GetSceneConstantBuffer().mousePosition.x = m_mousePosition[0];
	//parent->GetSceneConstantBuffer().mousePosition.y = m_mousePosition[1];
	//parent->GetSceneConstantBuffer().resolution.x = static_cast<float>(parent->m_width);
	//parent->GetSceneConstantBuffer().resolution.y = static_cast<float>(parent->m_height);
	memcpy(m_pTetraCbSrvDataBegin, &parent->GetSceneConstantBuffer(), sizeof(parent->GetSceneConstantBuffer()));
	//time += 0.1f;
}

HRESULT CD3D12Tetra::LoadTetra(ComPtr<ID3D12Device> dev, UINT m_width, UINT m_height)
{
	HRESULT hr;

	
	vCount = 4 * 3;
	totalverts = 12;
	
	// Destroy the old vertex buffer, if any
	m_pTetraVB = NULL;// {};
	m_pSysMemTetraVB = NULL;// {};
	// Create a vertex buffer



	// Create the vertex buffer for render pass 1.
	{
		// { { XMFLOAT3(0.0f, 0.0f, 0.0f) }, { XMFLOAT3(0.0f, 0.0f , 0.0f) }, { XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f) } },
		// Define the geometry drawing grid.
		m_pTetraVB = (TETRAVERTEX*)malloc(totalverts * sizeof(TETRAVERTEX));
		m_pSysMemTetraVB = (TETRAVERTEX*)malloc(totalverts * sizeof(TETRAVERTEX));

		DWORD m_dwNVertices = 4;
		XMFLOAT3* ptetras = new XMFLOAT3[m_dwNVertices];
		float fScale = 10.0f;
		XMFLOAT3 v; v.x = 0; v.y = 0; v.z = 0;
		//ptetras->x = (0.5f * fScale) + v.x; ptetras->y = (0.5f * fScale) + v.y; ptetras->z = (-0.5f * fScale) + v.z;/* ptetras->n.x = -1.0f; ptetras->n.y = 0.0f; ptetras->n.z = 0.0f; ptetras->tu = -1.0f; ptetras->tv = -1.05f;*/ptetras++;
		//ptetras->x = (-0.5f * fScale) + v.x; ptetras->y = (0.5f * fScale) + v.y; ptetras->z = (-0.5f * fScale) + v.z;/* ptetras->n.x = 0.0f; ptetras->n.y = 1.0f; ptetras->n.z = 0.0f; ptetras->tu = -0.9f; ptetras->tv = -1.15f;*/ ptetras++;
		//ptetras->x = 0.0f + v.x; ptetras->y = 0.0f + v.y; ptetras->z = (0.5f * fScale) + v.z;/* ptetras->n.x = -0.5345225f; ptetras->n.y = 0.80178374f; ptetras->n.z = 0.26726124f; ptetras->tu = -1.0f; ptetras->tv = -1.0f;*/ ptetras++;
		//ptetras->x = 0.0f + v.x; ptetras->y = (-0.5f * fScale) + v.y; ptetras->z = (-0.5f * fScale) + v.z;/* ptetras->n.x = -0.5f; ptetras->n.y = 0.8320503f; ptetras->n.z = 0.0f; ptetras->tu = -0.9f; ptetras->tv = -1.15f;*/ ptetras++;

		//WORD fd[24] = { 0,1,2,0,2,3,0,3,1,1,3,2,2,1,0,3,2,0,1,3,0,2,3,1 };

		TETRAVERTEX tetraVertices[] =
		{
			// {{(0.5f * fScale) + v.x, (0.5f * fScale) + v.y, (-0.5f * fScale) + v.z }, { 0.8f, 0.8f, 0.0f, 1.0f } },
			// {{(-0.5f * fScale) + v.x, (0.5f * fScale) + v.y, (-0.5f * fScale) + v.z},{0.0f, 0.8f, 0.8f, 1.0f }},
			// {{0.0f + v.x, 0.0f + v.y, (0.5f * fScale) + v.z},{0.8f, 0.0f, 0.8f, 1.0f}},
			// {{0.0f + v.x, (-0.5f * fScale) + v.y, (-0.5f * fScale) + v.z},{0.8f, 0.8f, 0.8f, 1.0f}},
		
			 {{(0.5f * fScale) + v.x, (0.5f * fScale) + v.y, (-0.5f * fScale) + v.z }, { 0.8f, 0.8f, 0.0f, 1.0f } },
			 {{(-0.5f * fScale) + v.x, (0.5f * fScale) + v.y, (-0.5f * fScale) + v.z},{0.0f, 0.8f, 0.8f, 1.0f }},
			 {{0.0f + v.x, 0.0f + v.y, (0.5f * fScale) + v.z},{0.8f, 0.0f, 0.8f, 1.0f}},

			 {{(0.5f * fScale) + v.x, (0.5f * fScale) + v.y, (-0.5f * fScale) + v.z }, { 0.8f, 0.8f, 0.0f, 1.0f } },
			 {{0.0f + v.x, 0.0f + v.y, (0.5f * fScale) + v.z},{0.8f, 0.0f, 0.8f, 1.0f}},
			 {{0.0f + v.x, (-0.5f * fScale) + v.y, (-0.5f * fScale) + v.z},{0.8f, 0.8f, 0.8f, 1.0f}},

			 {{(0.5f * fScale) + v.x, (0.5f * fScale) + v.y, (-0.5f * fScale) + v.z }, { 0.8f, 0.8f, 0.0f, 1.0f } },
			 {{0.0f + v.x, (-0.5f * fScale) + v.y, (-0.5f * fScale) + v.z},{0.8f, 0.8f, 0.8f, 1.0f}},
			 {{(-0.5f * fScale) + v.x, (0.5f * fScale) + v.y, (-0.5f * fScale) + v.z},{0.0f, 0.8f, 0.8f, 1.0f }},

			 {{(-0.5f * fScale) + v.x, (0.5f * fScale) + v.y, (-0.5f * fScale) + v.z},{0.0f, 0.8f, 0.8f, 1.0f }},
			 {{0.0f + v.x, (-0.5f * fScale) + v.y, (-0.5f * fScale) + v.z},{0.8f, 0.8f, 0.8f, 1.0f}},
             {{0.0f + v.x, 0.0f + v.y, (0.5f * fScale) + v.z},{0.8f, 0.0f, 0.8f, 1.0f}},
			 
			 {{0.0f + v.x, 0.0f + v.y, (0.5f * fScale) + v.z},{0.8f, 0.0f, 0.8f, 1.0f}},
			 {{(-0.5f * fScale) + v.x, (0.5f * fScale) + v.y, (-0.5f * fScale) + v.z},{0.0f, 0.8f, 0.8f, 1.0f }},
			 {{(0.5f * fScale) + v.x, (0.5f * fScale) + v.y, (-0.5f * fScale) + v.z }, { 0.8f, 0.8f, 0.0f, 1.0f } },

			 {{0.0f + v.x, (-0.5f * fScale) + v.y, (-0.5f * fScale) + v.z},{0.8f, 0.8f, 0.8f, 1.0f}},
			 {{0.0f + v.x, 0.0f + v.y, (0.5f * fScale) + v.z},{0.8f, 0.0f, 0.8f, 1.0f}},
			 {{(0.5f * fScale) + v.x, (0.5f * fScale) + v.y, (-0.5f * fScale) + v.z }, { 0.8f, 0.8f, 0.0f, 1.0f } },

			 {{(-0.5f * fScale) + v.x, (0.5f * fScale) + v.y, (-0.5f * fScale) + v.z},{0.0f, 0.8f, 0.8f, 1.0f }},
			 {{0.0f + v.x, (-0.5f * fScale) + v.y, (-0.5f * fScale) + v.z},{0.8f, 0.8f, 0.8f, 1.0f}},
			 {{(0.5f * fScale) + v.x, (0.5f * fScale) + v.y, (-0.5f * fScale) + v.z }, { 0.8f, 0.8f, 0.0f, 1.0f } },

			 {{0.0f + v.x, 0.0f + v.y, (0.5f * fScale) + v.z},{0.8f, 0.0f, 0.8f, 1.0f}},
			 {{0.0f + v.x, (-0.5f * fScale) + v.y, (-0.5f * fScale) + v.z},{0.8f, 0.8f, 0.8f, 1.0f}},
			 {{(-0.5f * fScale) + v.x, (0.5f * fScale) + v.y, (-0.5f * fScale) + v.z},{0.0f, 0.8f, 0.8f, 1.0f }}

		};

		

		const UINT vertexBufferSize = sizeof(totalverts * sizeof(TETRAVERTEX));

		// Note: using upload heaps to transfer static data like vert buffers is not 
		// recommended. Every time the GPU needs it, the upload heap will be marshalled 
		// over. Please read up on Default Heap usage. An upload heap is used here for 
		// code simplicity and because there are very few verts to actually transfer.
		ThrowIfFailed(dev->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_pTetraRenderPass1VertBuff)));


		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_pTetraRenderPass1VertBuff->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, m_pTetraVB, sizeof(m_pTetraVB));
		m_pTetraRenderPass1VertBuff->Unmap(0, nullptr);

		// Initialize the vertex buffer view.
		m_pTetraRenderPass1VertBuffView.BufferLocation = m_pTetraRenderPass1VertBuff->GetGPUVirtualAddress();
		m_pTetraRenderPass1VertBuffView.StrideInBytes = sizeof(TETRAVERTEX);
		m_pTetraRenderPass1VertBuffView.SizeInBytes = vertexBufferSize;
	}

		
	// Create texture resources for render pass1. The first render pass will render the visualizations of wave intrinsics to an intermediate texture. 
// In render pass 2, it will blend UI layer and this intermediate texture and then use a magifier effect on the blended texture.
	{
		// Create the texture.
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = 1;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.Width = m_width;
		textureDesc.Height = m_height;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		const float ccolor[4] = { 0, 0, 0, 0 };
		CD3DX12_CLEAR_VALUE clearValue(DXGI_FORMAT_R8G8B8A8_UNORM, ccolor);

		dev->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&clearValue,
			IID_PPV_ARGS(&m_pTetraRenderPass1RenderTargets));
		NAME_D3D12_OBJECT(m_pTetraRenderPass1RenderTargets);

		// Create RTV for the texture
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pTetrartvHeap->GetCPUDescriptorHandleForHeapStart());
		rtvHandle.Offset(2, m_pTetrartvDescriptorSize); // First two are referencing to swapchain back buffers. 
		dev->CreateRenderTargetView(m_pTetraRenderPass1RenderTargets.Get(), nullptr, rtvHandle);

		// Create SRV for the texture
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_pTetracbSrvHeap->GetCPUDescriptorHandleForHeapStart());
		srvHandle.Offset(1, m_pTetracbSrvDescriptorSize); // First one is for constant buffer.
		dev->CreateShaderResourceView(m_pTetraRenderPass1RenderTargets.Get(), nullptr, srvHandle);
	}


	hr = S_OK;
	return S_OK;
}

HRESULT CD3D12Tetra::RestoreTetra(ComPtr<ID3D12Device> dev)
{
	return E_NOTIMPL;
}

HRESULT CD3D12Tetra::RenderTetra(DeviceResources* parent, ComPtr<ID3D12Device> dev, XMMATRIX m, XMFLOAT3 v)
{
	// Render Pass 1. Render the scene (triangle) to an intermediate texture.
	// Render Pass 2. Compose the intermediate texture from render pass1 and UI layer together.

	// Command list allocators can only be reset when the associated 
	// command lists have finished execution on the GPU; apps should use 
	// fences to determine GPU execution progress.
	ThrowIfFailed(m_pTetraCommandAllocators[m_pTetraFrameIndex]->Reset());

	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
	ThrowIfFailed(m_pTetraCommandList->Reset(m_pTetraCommandAllocators[m_pTetraFrameIndex].Get(), m_pTetrarenderPass1PSO.Get()));

	// Render Pass 1: Render the scene (triangle) to an intermediate texture.
	// Set necessary state.
	m_pTetraCommandList->SetGraphicsRootSignature(m_pTetraRenderPass1RootSignature.Get());
	ID3D12DescriptorHeap* ppHeaps[] = { m_pTetracbSrvHeap.Get() };
	m_pTetraCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	m_pTetraCommandList->SetGraphicsRootDescriptorTable(0, m_pTetracbSrvHeap->GetGPUDescriptorHandleForHeapStart());
	m_pTetraCommandList->RSSetViewports(1, &parent->GetViewPort());
	m_pTetraCommandList->RSSetScissorRects(1, &parent->GetScissorRect());

	// Set up render target
	m_pTetraCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pTetraRenderPass1RenderTargets.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
	CD3DX12_CPU_DESCRIPTOR_HANDLE renderPass1RtvHandle(m_pTetrartvHeap->GetCPUDescriptorHandleForHeapStart(), 2, m_pTetrartvDescriptorSize);
	m_pTetraCommandList->OMSetRenderTargets(1, &renderPass1RtvHandle, FALSE, nullptr);

	// Record commands.
	const float clearColor[] = { 0, 0, 0, 0 };
	m_pTetraCommandList->ClearRenderTargetView(renderPass1RtvHandle, clearColor, 0, nullptr);
	m_pTetraCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pTetraCommandList->IASetVertexBuffers(0, 1, &m_pTetraRenderPass1VertBuffView);
	// 24 vertices, 8 faces
	m_pTetraCommandList->DrawInstanced(24, 8, 0, 0);

	m_pTetraCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pTetraRenderPass1RenderTargets.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	ThrowIfFailed(m_pTetraCommandList->Close());

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { m_pTetraCommandList.Get() };
	m_TetraCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	return S_OK;
}

void CD3D12Tetra::MoveToNextFrame(DeviceResources* parent)
{
	// Schedule a Signal command in the queue.
	const UINT64 currentFenceValue = m_pTetraFenceValues[m_pTetraFrameIndex];
	ThrowIfFailed(m_TetraCommandQueue->Signal(m_pTetraFence.Get(), currentFenceValue));

	// Update the frame index.
	m_pTetraFrameIndex = parent->GetSwapChain()->GetCurrentBackBufferIndex();

	// If the next frame is not ready to be rendered yet, wait until it is ready.
	if (m_pTetraFence->GetCompletedValue() < m_pTetraFenceValues[m_pTetraFrameIndex])
	{
		ThrowIfFailed(m_pTetraFence->SetEventOnCompletion(m_pTetraFenceValues[m_pTetraFrameIndex], m_pTetraFenceEvent));
		WaitForSingleObjectEx(m_pTetraFenceEvent, INFINITE, FALSE);
	}

	// Set the fence value for the next frame.
	m_pTetraFenceValues[m_pTetraFrameIndex] = currentFenceValue + 1;
}

void CD3D12Tetra::WaitForGpu(DeviceResources* parent)
{
	// Schedule a Signal command in the queue.
	ThrowIfFailed(m_TetraCommandQueue->Signal(m_pTetraFence.Get(), m_pTetraFenceValues[m_pTetraFrameIndex]));

	// Wait until the fence has been processed.
	DX::ThrowIfFailed(m_pTetraFence->SetEventOnCompletion(m_pTetraFenceValues[m_pTetraFrameIndex], m_pTetraFenceEvent));
	WaitForSingleObjectEx(m_pTetraFenceEvent, INFINITE, FALSE);

	// Increment the fence value for the current frame.
	m_pTetraFenceValues[m_pTetraFrameIndex]++;
}

void CD3D12Tetra::OnSizeChanged(DeviceResources* parent, ComPtr<ID3D12Device> dev, UINT width, UINT height, bool minimized)
{
	UNREFERENCED_PARAMETER(minimized);
	parent->UpdateForSizeChange(width, height);

	if (!parent->GetSwapChain())
	{
		return;
	}

	// Flush all current GPU commands.
	WaitForGpu(parent);

	// Release the resources holding references to the swap chain (requirement of
	// IDXGISwapChain::ResizeBuffers) and reset the frame fence values to the
	// current fence value.
	m_pTetraRenderPass1RenderTargets.Reset();
	for (UINT n = 0; n < m_pTetraFrameCount; n++)
	{

		m_pTetraFenceValues[n] = m_pTetraFenceValues[m_pTetraFrameIndex];
	}


	// Resize the swap chain to the desired dimensions.
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	parent->GetSwapChain()->GetDesc1(&desc);
	ThrowIfFailed(parent->GetSwapChain()->ResizeBuffers(m_pTetraFrameCount, width, height, desc.Format, desc.Flags));

	// Reset the frame index to the current back buffer index.
	m_pTetraFrameIndex = parent->GetSwapChain()->GetCurrentBackBufferIndex();

	LoadTetra(dev, width, height);
}


