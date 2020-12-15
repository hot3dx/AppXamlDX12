//--------------------------------------------------------------------------------------
// File: Hot3dxDrawnObject.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Hot3dxDrawnObject.h"

#include "CommonStates.h"
#include "DirectXHelpers.h"
#include "Effects.h"
#include "Geometry.h"
#include "GraphicsMemory.h"
#include "PlatformHelpers.h"
#include "MyResourceUploadBatch.h"

using namespace DirectX;
using namespace std;
using Microsoft::WRL::ComPtr;

namespace
{
    const float SQRT2 = 1.41421356237309504880f;
    const float SQRT3 = 1.73205080756887729352f;
    const float SQRT6 = 2.44948974278317809820f;

    inline void CheckIndexOverflowHot3dx(size_t value)
    {
        // Use >=, not > comparison, because some D3D level 9_x hardware does not support 0xFFFF index values.
        if (value >= USHRT_MAX)
            throw std::exception("Index value out of range: cannot tesselate primitive so finely");
    }


    // Collection types used when generating the geometry.
    inline void index_push_backHot3dx(IndexCollectionColor& indices, size_t value)
    {
        CheckIndexOverflowHot3dx(value);
        indices.push_back(static_cast<uint16_t>(value));
    }


    // Helper for flipping winding of geometric primitives for LH vs. RH coords
    inline void ReverseWindingHot3dx(IndexCollectionColor& indices, VertexCollectionColor& vertices)
    {
        assert((indices.size() % 3) == 0);
        for (auto it = indices.begin(); it != indices.end(); it += 3)
        {
            std::swap(*it, *(it + 2));
        }

        for (auto it = vertices.begin(); it != vertices.end(); ++it)
        {
           // it->position.x = (1.f - it->position.x);
        }
    }


    // Helper for inverting normals of geometric primitives for 'inside' vs. 'outside' viewing
    inline void InvertNormalsHot3dx(VertexCollectionColor& vertices)
    {
        for (auto it = vertices.begin(); it != vertices.end(); ++it)
        {
            //it->normal.x = -it->normal.x;
            //it->normal.y = -it->normal.y;
           // it->normal.z = -it->normal.z;
        }
    }
}
// Internal Hot3dxDrawnObject implementation class.
class Hot3dxDrawnObject::Impl
{
public:
    Impl() noexcept : mIndexCount(0), mVertexBufferView{}, mIndexBufferView{} {}

    void Initialize(const VertexCollectionColor& vertices, const IndexCollectionColor& indices, _In_opt_ ID3D12Device* device);

    void InitializeDrawnObjectColor(const VertexCollectionColor& vertices, const IndexCollectionColor& indices, _In_opt_ ID3D12Device* device);

    void LoadStaticBuffers(_In_ ID3D12Device* device, ResourceUploadBatch& resourceUploadBatch);

    void Draw(_In_ ID3D12GraphicsCommandList* commandList) const;

    UINT                        mIndexCount;
    SharedGraphicsResource      mIndexBuffer;
    SharedGraphicsResource      mVertexBuffer;
    ComPtr<ID3D12Resource>      mStaticIndexBuffer;
    ComPtr<ID3D12Resource>      mStaticVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW    mVertexBufferView;
    D3D12_INDEX_BUFFER_VIEW     mIndexBufferView;
};


// Initializes a geometric primitive instance that will draw the specified vertex and index data.
void Hot3dxDrawnObject::Impl::Initialize(
    const VertexCollectionColor& vertices,
    const IndexCollectionColor& indices,
    _In_opt_ ID3D12Device* device)
{
    if (vertices.size() >= USHRT_MAX)
        throw std::exception("Too many vertices for 16-bit index buffer");

    if (indices.size() > UINT32_MAX)
        throw std::exception("Too many indices");

    // Vertex data
    uint64_t sizeInBytes = uint64_t(vertices.size()) * sizeof(vertices[0]);
    if (sizeInBytes > uint64_t(D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM * 1024u * 1024u))
        throw std::exception("VB too large for DirectX 12");

    auto vertSizeBytes = static_cast<size_t>(sizeInBytes);

    mVertexBuffer = GraphicsMemory::Get(device).Allocate(vertSizeBytes);

    auto verts = reinterpret_cast<const uint8_t*>(vertices.data());
    memcpy(mVertexBuffer.Memory(), verts, vertSizeBytes);

    // Index data
    sizeInBytes = uint64_t(indices.size()) * sizeof(indices[0]);
    if (sizeInBytes > uint64_t(D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM * 1024u * 1024u))
        throw std::exception("IB too large for DirectX 12");

    auto indSizeBytes = static_cast<size_t>(sizeInBytes);

    mIndexBuffer = GraphicsMemory::Get(device).Allocate(indSizeBytes);

    auto ind = reinterpret_cast<const uint8_t*>(indices.data());
    memcpy(mIndexBuffer.Memory(), ind, indSizeBytes);

    // Record index count for draw
    mIndexCount = static_cast<UINT>(indices.size());

    // Create views
    mVertexBufferView.BufferLocation = mVertexBuffer.GpuAddress();
    mVertexBufferView.StrideInBytes = static_cast<UINT>(sizeof(VertexCollectionColor::value_type));
    mVertexBufferView.SizeInBytes = static_cast<UINT>(mVertexBuffer.Size());

    mIndexBufferView.BufferLocation = mIndexBuffer.GpuAddress();
    mIndexBufferView.SizeInBytes = static_cast<UINT>(mIndexBuffer.Size());
    mIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
}

void Hot3dxDrawnObject::Impl::InitializeDrawnObjectColor(const VertexCollectionColor& vertices, const IndexCollectionColor& indices, ID3D12Device* device)
{

    if (vertices.size() >= USHRT_MAX)
        throw std::exception("Too many vertices for 16-bit index buffer");

    if (indices.size() > UINT32_MAX)
        throw std::exception("Too many indices");

    // Vertex data
    uint64_t sizeInBytes = uint64_t(vertices.size()) * sizeof(vertices[0]);
    if (sizeInBytes > uint64_t(D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM * 1024u * 1024u))
        throw std::exception("VB too large for DirectX 12");

    auto vertSizeBytes = static_cast<size_t>(sizeInBytes);

    mVertexBuffer = GraphicsMemory::Get(device).Allocate(vertSizeBytes);

    auto verts = reinterpret_cast<const uint8_t*>(vertices.data());
    memcpy(mVertexBuffer.Memory(), verts, vertSizeBytes);

    // Index data
    sizeInBytes = uint64_t(indices.size()) * sizeof(indices[0]);
    if (sizeInBytes > uint64_t(D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM * 1024u * 1024u))
        throw std::exception("IB too large for DirectX 12");

    auto indSizeBytes = static_cast<size_t>(sizeInBytes);

    mIndexBuffer = GraphicsMemory::Get(device).Allocate(indSizeBytes);

    auto ind = reinterpret_cast<const uint8_t*>(indices.data());
    memcpy(mIndexBuffer.Memory(), ind, indSizeBytes);

    // Record index count for draw
    mIndexCount = static_cast<UINT>(indices.size());

    // Create views
    mVertexBufferView.BufferLocation = mVertexBuffer.GpuAddress();
    mVertexBufferView.StrideInBytes = static_cast<UINT>(sizeof(VertexCollectionColor::value_type));
    mVertexBufferView.SizeInBytes = static_cast<UINT>(mVertexBuffer.Size());

    mIndexBufferView.BufferLocation = mIndexBuffer.GpuAddress();
    mIndexBufferView.SizeInBytes = static_cast<UINT>(mIndexBuffer.Size());
    mIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
}


// Load VB/IB resources for static geometry.
_Use_decl_annotations_
void Hot3dxDrawnObject::Impl::LoadStaticBuffers(
    ID3D12Device* device,
    ResourceUploadBatch& resourceUploadBatch)
{
    CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

    // Convert dynamic VB to static VB
    if (!mStaticVertexBuffer)
    {
        assert(mVertexBuffer);

        auto desc = CD3DX12_RESOURCE_DESC::Buffer(mVertexBuffer.Size());

        ThrowIfFailed(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(mStaticVertexBuffer.GetAddressOf())
        ));

        SetDebugObjectName(mStaticVertexBuffer.Get(), L"Hot3dxDrawnObject");

        resourceUploadBatch.Upload(mStaticVertexBuffer.Get(), mVertexBuffer);

        resourceUploadBatch.Transition(mStaticVertexBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

        // Update view
        mVertexBufferView.BufferLocation = mStaticVertexBuffer->GetGPUVirtualAddress();

        mVertexBuffer.Reset();
    }

    // Convert dynamic IB to static IB
    if (!mStaticIndexBuffer)
    {
        assert(mIndexBuffer);

        auto desc = CD3DX12_RESOURCE_DESC::Buffer(mIndexBuffer.Size());

        ThrowIfFailed(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(mStaticIndexBuffer.GetAddressOf())
        ));

        SetDebugObjectName(mStaticIndexBuffer.Get(), L"Hot3dxDrawnObject");

        resourceUploadBatch.Upload(mStaticIndexBuffer.Get(), mIndexBuffer);

        resourceUploadBatch.Transition(mStaticIndexBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

        // Update view
        mIndexBufferView.BufferLocation = mStaticIndexBuffer->GetGPUVirtualAddress();

        mIndexBuffer.Reset();
    }
}


// Draws the primitive.
_Use_decl_annotations_
void Hot3dxDrawnObject::Impl::Draw(ID3D12GraphicsCommandList* commandList) const
{
    commandList->IASetVertexBuffers(0, 1, &mVertexBufferView);
    commandList->IASetIndexBuffer(&mIndexBufferView);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList->DrawIndexedInstanced(mIndexCount, 1, 0, 0, 0);
}

//--------------------------------------------------------------------------------------
// Hot3dxDrawnObject
//--------------------------------------------------------------------------------------

// Constructor.
Hot3dxDrawnObject::Hot3dxDrawnObject() noexcept(false)
    : pImpl(std::make_unique<Impl>())
{
}


// Destructor.
Hot3dxDrawnObject::~Hot3dxDrawnObject()
{
}


// Public entrypoints.
_Use_decl_annotations_
void Hot3dxDrawnObject::LoadStaticBuffers(ID3D12Device* device, ResourceUploadBatch& resourceUploadBatch)
{
    pImpl->LoadStaticBuffers(device, resourceUploadBatch);
}


_Use_decl_annotations_
void Hot3dxDrawnObject::Draw(ID3D12GraphicsCommandList* commandList) const
{
    pImpl->Draw(commandList);
}


//--------------------------------------------------------------------------------------
// Cube (aka a Hexahedron) or Box
//--------------------------------------------------------------------------------------

// Creates a cube primitive.
std::unique_ptr<Hot3dxDrawnObject> Hot3dxDrawnObject::CreateCube(
    float size,
    bool rhcoords,
    _In_opt_ ID3D12Device* device)
{
    VertexCollectionColor vertices;
   IndexCollectionColor indices;
    ComputeBoxColor(vertices, indices, XMFLOAT3(size, size, size), rhcoords);

    // Create the primitive object.
    std::unique_ptr<Hot3dxDrawnObject> drawnObject(new Hot3dxDrawnObject());

    drawnObject->pImpl->Initialize(vertices, indices, device);

    return drawnObject;
}

void Hot3dxDrawnObject::CreateCube(
    std::vector<VertexType>& vertices,
    std::vector<uint16_t>& indices,
    float size,
    bool rhcoords)
{
    ComputeBoxColor(vertices, indices, XMFLOAT3(size, size, size), rhcoords);
}


// Creates a box primitive.
std::unique_ptr<Hot3dxDrawnObject> Hot3dxDrawnObject::CreateBox(
    const XMFLOAT3& size,
    bool rhcoords,
    _In_opt_ ID3D12Device* device)
{
    VertexCollectionColor vertices;
    IndexCollectionColor indices;
    ComputeBoxColor(vertices, indices, size, rhcoords);

    // Create the primitive object.
    std::unique_ptr<Hot3dxDrawnObject> drawnObject(new Hot3dxDrawnObject());

    drawnObject->pImpl->Initialize(vertices, indices, device);

    return drawnObject;
}

void Hot3dxDrawnObject::CreateBox(
    std::vector<VertexType>& vertices,
    std::vector<uint16_t>& indices,
    const XMFLOAT3& size,
    bool rhcoords)
{
    ComputeBoxColor(vertices, indices, size, rhcoords);
}


//--------------------------------------------------------------------------------------
// Sphere
//--------------------------------------------------------------------------------------

// Creates a sphere primitive.
std::unique_ptr<Hot3dxDrawnObject> Hot3dxDrawnObject::CreateSphere(
    float diameter,
    size_t tessellation,
    bool rhcoords,
    _In_opt_ ID3D12Device* device)
{
    // Create the primitive object.
    std::unique_ptr<Hot3dxDrawnObject> primitive(new Hot3dxDrawnObject());

    VertexCollectionColor vertices;
    IndexCollectionColor indices;

    ComputeSphereColor(vertices, indices, diameter, tessellation, rhcoords);

    primitive->pImpl->Initialize(vertices, indices, device);

    return primitive;
}

void Hot3dxDrawnObject::CreateSphere(
    std::vector<VertexType>& vertices,
    std::vector<uint16_t>& indices,
    float diameter,
    size_t tessellation,
    bool rhcoords)
{
    ComputeSphereColor(vertices, indices, diameter, tessellation, rhcoords);
}


//--------------------------------------------------------------------------------------
// Geodesic sphere
//--------------------------------------------------------------------------------------

// Creates a geosphere primitive.
std::unique_ptr<Hot3dxDrawnObject> Hot3dxDrawnObject::CreateGeoSphere(
    float diameter,
    size_t tessellation,
    bool rhcoords,
    _In_opt_ ID3D12Device* device)
{
    // Create the primitive object.
    std::unique_ptr<Hot3dxDrawnObject> primitive(new Hot3dxDrawnObject());

    VertexCollectionColor vertices;
    IndexCollectionColor indices;
    ComputeGeoSphereColor(vertices, indices, diameter, tessellation, rhcoords);

    primitive->pImpl->Initialize(vertices, indices, device);

    return primitive;
}

void Hot3dxDrawnObject::CreateGeoSphere(
    std::vector<VertexType>& vertices,
    std::vector<uint16_t>& indices,
    float diameter,
    size_t tessellation,
    bool rhcoords)
{
    ComputeGeoSphereColor(vertices, indices, diameter, tessellation, rhcoords);
}


//--------------------------------------------------------------------------------------
// Cylinder / Cone
//--------------------------------------------------------------------------------------

// Creates a cylinder primitive.
std::unique_ptr<Hot3dxDrawnObject> Hot3dxDrawnObject::CreateCylinder(
    float height,
    float diameter,
    size_t tessellation,
    bool rhcoords,
    _In_opt_ ID3D12Device* device)
{
    // Create the primitive object.
    std::unique_ptr<Hot3dxDrawnObject> primitive(new Hot3dxDrawnObject());

    VertexCollectionColor vertices;
    IndexCollectionColor indices;
    ComputeCylinderColor(vertices, indices, height, diameter, tessellation, rhcoords);

    primitive->pImpl->Initialize(vertices, indices, device);

    return primitive;
}

void Hot3dxDrawnObject::CreateCylinder(
    std::vector<VertexType>& vertices,
    std::vector<uint16_t>& indices,
    float height, float diameter,
    size_t tessellation,
    bool rhcoords)
{
    ComputeCylinderColor(vertices, indices, height, diameter, tessellation, rhcoords);
}


// Creates a cone primitive.
std::unique_ptr<Hot3dxDrawnObject> Hot3dxDrawnObject::CreateCone(
    float diameter,
    float height,
    size_t tessellation,
    bool rhcoords,
    _In_opt_ ID3D12Device* device)
{
    // Create the primitive object.
    std::unique_ptr<Hot3dxDrawnObject> primitive(new Hot3dxDrawnObject());

    VertexCollectionColor vertices;
    IndexCollectionColor indices;
    ComputeConeColor(vertices, indices, diameter, height, tessellation, rhcoords);

    primitive->pImpl->Initialize(vertices, indices, device);

    return primitive;
}

void Hot3dxDrawnObject::CreateCone(
    std::vector<VertexType>& vertices,
    std::vector<uint16_t>& indices,
    float diameter,
    float height,
    size_t tessellation,
    bool rhcoords)
{
    ComputeConeColor(vertices, indices, diameter, height, tessellation, rhcoords);
}


//--------------------------------------------------------------------------------------
// Torus
//--------------------------------------------------------------------------------------

// Creates a torus primitive.
std::unique_ptr<Hot3dxDrawnObject> Hot3dxDrawnObject::CreateTorus(
    float diameter,
    float thickness,
    size_t tessellation,
    bool rhcoords,
    _In_opt_ ID3D12Device* device)
{
    // Create the primitive object.
    std::unique_ptr<Hot3dxDrawnObject> primitive(new Hot3dxDrawnObject());

    VertexCollectionColor vertices;
    IndexCollectionColor indices;
    ComputeTorusColor(vertices, indices, diameter, thickness, tessellation, rhcoords);

    primitive->pImpl->Initialize(vertices, indices, device);

    return primitive;
}

void Hot3dxDrawnObject::CreateTorus(
    std::vector<VertexType>& vertices,
    std::vector<uint16_t>& indices,
    float diameter,
    float thickness,
    size_t tessellation,
    bool rhcoords)
{
    ComputeTorusColor(vertices, indices, diameter, thickness, tessellation, rhcoords);
}


//--------------------------------------------------------------------------------------
// Tetrahedron
//--------------------------------------------------------------------------------------

std::unique_ptr<Hot3dxDrawnObject> Hot3dxDrawnObject::CreateTetrahedron(
    float size,
    bool rhcoords,
    _In_opt_ ID3D12Device* device)
{
    // Create the primitive object.
    std::unique_ptr<Hot3dxDrawnObject> primitive(new Hot3dxDrawnObject());

    VertexCollectionColor vertices;
    IndexCollectionColor indices;
    ComputeTetrahedronColor(vertices, indices, size, rhcoords);

    primitive->pImpl->Initialize(vertices, indices, device);

    return primitive;
}

void Hot3dxDrawnObject::CreateTetrahedron(
    std::vector<VertexType>& vertices,
    std::vector<uint16_t>& indices,
    float size,
    bool rhcoords)
{
    ComputeTetrahedronColor(vertices, indices, size, rhcoords);
}


//--------------------------------------------------------------------------------------
// Octahedron
//--------------------------------------------------------------------------------------

std::unique_ptr<Hot3dxDrawnObject> Hot3dxDrawnObject::CreateOctahedron(
    float size,
    bool rhcoords,
    _In_opt_ ID3D12Device* device)
{
    // Create the primitive object.
    std::unique_ptr<Hot3dxDrawnObject> primitive(new Hot3dxDrawnObject());

    VertexCollectionColor vertices;
    IndexCollectionColor indices;
    ComputeOctahedronColor(vertices, indices, size, rhcoords);

    primitive->pImpl->Initialize(vertices, indices, device);

    return primitive;
}

void Hot3dxDrawnObject::CreateOctahedron(
    std::vector<VertexType>& vertices,
    std::vector<uint16_t>& indices,
    float size,
    bool rhcoords)
{
    ComputeOctahedronColor(vertices, indices, size, rhcoords);
}


//--------------------------------------------------------------------------------------
// Dodecahedron
//--------------------------------------------------------------------------------------

std::unique_ptr<Hot3dxDrawnObject> Hot3dxDrawnObject::CreateDodecahedron(
    float size,
    bool rhcoords,
    _In_opt_ ID3D12Device* device)
{
    // Create the primitive object.
    std::unique_ptr<Hot3dxDrawnObject> primitive(new Hot3dxDrawnObject());

    VertexCollectionColor vertices;
    IndexCollectionColor indices;
    ComputeDodecahedronColor(vertices, indices, size, rhcoords);

    primitive->pImpl->Initialize(vertices, indices, device);

    return primitive;
}

void Hot3dxDrawnObject::CreateDodecahedron(
    std::vector<VertexType>& vertices,
    std::vector<uint16_t>& indices,
    float size,
    bool rhcoords)
{
    ComputeDodecahedronColor(vertices, indices, size, rhcoords);
}


//--------------------------------------------------------------------------------------
// Icosahedron
//--------------------------------------------------------------------------------------

std::unique_ptr<Hot3dxDrawnObject> Hot3dxDrawnObject::CreateIcosahedron(
    float size,
    bool rhcoords,
    _In_opt_ ID3D12Device* device)
{
    // Create the primitive object.
    std::unique_ptr<Hot3dxDrawnObject> primitive(new Hot3dxDrawnObject());

    VertexCollectionColor vertices;
    IndexCollectionColor indices;
    ComputeIcosahedronColor(vertices, indices, size, rhcoords);

    primitive->pImpl->Initialize(vertices, indices, device);

    return primitive;
}

void Hot3dxDrawnObject::CreateIcosahedron(
    std::vector<VertexType>& vertices,
    std::vector<uint16_t>& indices,
    float size,
    bool rhcoords)
{
    ComputeIcosahedronColor(vertices, indices, size, rhcoords);
}


//--------------------------------------------------------------------------------------
// Custom
//--------------------------------------------------------------------------------------

std::unique_ptr<Hot3dxDrawnObject> Hot3dxDrawnObject::CreateCustom(
    const std::vector<VertexType>& vertices,
    const std::vector<uint16_t>& indices,
    _In_opt_ ID3D12Device* device)
{
    // Extra validation
    if (vertices.empty() || indices.empty())
        throw std::exception("Requires both vertices and indices");

    if (indices.size() % 3)
        throw std::exception("Expected triangular faces");

    size_t nVerts = vertices.size();
    if (nVerts >= USHRT_MAX)
        throw std::exception("Too many vertices for 16-bit index buffer");

    for (auto it = indices.cbegin(); it != indices.cend(); ++it)
    {
        if (*it >= nVerts)
        {
            throw std::exception("Index not in vertices list");
        }
    }
    // Create the primitive object.
    std::unique_ptr<Hot3dxDrawnObject> primitive(new Hot3dxDrawnObject());

    // copy geometry
    primitive->pImpl->Initialize(vertices, indices, device);

    return primitive;
}

std::unique_ptr<Hot3dxDrawnObject> __cdecl DirectX::Hot3dxDrawnObject::CreateDrawnObjectColor(const std::vector<VertexPositionColor>& vertices, const std::vector<uint16_t>& indices, ID3D12Device* device)
{
    // Extra validation
    if (vertices.empty() || indices.empty())
        throw std::exception("Requires both vertices and indices");

    if (indices.size() % 3)
        throw std::exception("Expected triangular faces");

    size_t nVerts = vertices.size();
    if (nVerts >= USHRT_MAX)
        throw std::exception("Too many vertices for 16-bit index buffer");

    for (auto it = indices.cbegin(); it != indices.cend(); ++it)
    {
        if (*it >= nVerts)
        {
            throw std::exception("Index not in vertices list");
        }
    }
    // Create the primitive object.
    std::unique_ptr<Hot3dxDrawnObject> hot3dxDrawn(new Hot3dxDrawnObject());

    // copy geometry
    hot3dxDrawn->pImpl->InitializeDrawnObjectColor(vertices, indices, device);

    return hot3dxDrawn;
}

std::unique_ptr<Hot3dxDrawnObject> __cdecl DirectX::Hot3dxDrawnObject::CreateDrawnObjectTexture(const std::vector<VertexType>& vertices, const std::vector<uint16_t>& indices, ID3D12Device* device)
{
    // Extra validation
    if (vertices.empty() || indices.empty())
        throw std::exception("Requires both vertices and indices");

    if (indices.size() % 3)
        throw std::exception("Expected triangular faces");

    size_t nVerts = vertices.size();
    if (nVerts >= USHRT_MAX)
        throw std::exception("Too many vertices for 16-bit index buffer");

    for (auto it = indices.cbegin(); it != indices.cend(); ++it)
    {
        if (*it >= nVerts)
        {
            throw std::exception("Index not in vertices list");
        }
    }
    // Create the primitive object.
    std::unique_ptr<Hot3dxDrawnObject> hot3dxDrawn(new Hot3dxDrawnObject());

    // copy geometry
    hot3dxDrawn->pImpl->Initialize(vertices, indices, device);

    return hot3dxDrawn;
}

void DirectX::ComputeBoxColor(VertexCollectionColor& vertices, IndexCollectionColor& indices, const XMFLOAT3& size, bool rhcoords)
{
    vertices.clear();
    indices.clear();

    // A box has six faces, each one pointing in a different direction.
    const int FaceCount = 6;

    XMFLOAT4 color = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
    
    static const XMVECTORF32 faceNormals[FaceCount] =
    {
        { { {  0,  0,  1, 0 } } },
        { { {  0,  0, -1, 0 } } },
        { { {  1,  0,  0, 0 } } },
        { { { -1,  0,  0, 0 } } },
        { { {  0,  1,  0, 0 } } },
        { { {  0, -1,  0, 0 } } }
    };

    static const XMVECTORF32 textureCoordinates[4] =
    {
        { { { 1, 0, 0, 0 } } },
        { { { 1, 1, 0, 0 } } },
        { { { 0, 1, 0, 0 } } },
        { { { 0, 0, 0, 0 } } }
    };
    
    XMVECTOR tsize = XMLoadFloat3(&size);
    tsize = XMVectorDivide(tsize, g_XMTwo);

    // Create each face in turn.
    for (int i = 0; i < FaceCount; i++)
    {
        XMVECTOR normal = faceNormals[i];

        // Get two vectors perpendicular both to the face normal and to each other.
        XMVECTOR basis = (i >= 4) ? g_XMIdentityR2 : g_XMIdentityR1;

        XMVECTOR side1 = XMVector3Cross(normal, basis);
        XMVECTOR side2 = XMVector3Cross(normal, side1);

        // Six indices (two triangles) per face.
        size_t vbase = vertices.size();
        index_push_backHot3dx(indices, vbase + 0);
        index_push_backHot3dx(indices, vbase + 1);
        index_push_backHot3dx(indices, vbase + 2);

        index_push_backHot3dx(indices, vbase + 0);
        index_push_backHot3dx(indices, vbase + 2);
        index_push_backHot3dx(indices, vbase + 3);

        // Four vertices per face.
        // (normal - side1 - side2) * tsize // normal // t0
        XMVECTOR t = XMVectorMultiply(XMVectorSubtract(XMVectorSubtract(normal, side1), side2), tsize);
         vertices.push_back(VertexPositionColor(XMFLOAT3(XMVectorGetX(t), XMVectorGetY(t), XMVectorGetZ(t)), color) );

        // (normal - side1 + side2) * tsize // normal // t1
         t = XMVectorMultiply(XMVectorAdd(XMVectorSubtract(normal, side1), side2), tsize);
         vertices.push_back(VertexPositionColor(XMFLOAT3(XMVectorGetX(t), XMVectorGetY(t), XMVectorGetZ(t)), color));

        // (normal + side1 + side2) * tsize // normal // t2
        t = XMVectorMultiply(XMVectorAdd(normal, XMVectorAdd(side1, side2)), tsize);
        vertices.push_back(VertexPositionColor(XMFLOAT3(XMVectorGetX(t), XMVectorGetY(t), XMVectorGetZ(t)), color));

        // (normal + side1 - side2) * tsize // normal // t3
        t = XMVectorMultiply(XMVectorSubtract(XMVectorAdd(normal, side1), side2), tsize);
        vertices.push_back(VertexPositionColor(XMFLOAT3(XMVectorGetX(t), XMVectorGetY(t), XMVectorGetZ(t)), color));
    }

    // Build RH above
    if (!rhcoords)
        ReverseWindingHot3dx(indices, vertices);
}

void DirectX::ComputeSphereColor(VertexCollectionColor& vertices, IndexCollectionColor& indices, float diameter, size_t tessellation, bool rhcoords)
{
    vertices.clear();
    indices.clear();
    XMFLOAT4 color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

    if (tessellation < 3)
        throw std::out_of_range("tesselation parameter out of range");

    size_t verticalSegments = tessellation;
    size_t horizontalSegments = tessellation * 2;

    float radius = diameter / 2;

    // Create rings of vertices at progressively higher latitudes.
    for (size_t i = 0; i <= verticalSegments; i++)
    {
        float v = 1 - float(i) / verticalSegments;

        float latitude = (i * XM_PI / verticalSegments) - XM_PIDIV2;
        float dy, dxz;

        XMScalarSinCos(&dy, &dxz, latitude);

        // Create a single ring of vertices at this latitude.
        for (size_t j = 0; j <= horizontalSegments; j++)
        {
            float u = float(j) / horizontalSegments;

            float longitude = j * XM_2PI / horizontalSegments;
            float dx, dz;

            XMScalarSinCos(&dx, &dz, longitude);

            dx *= dxz;
            dz *= dxz;

            XMVECTOR normal = XMVectorSet(dx, dy, dz, 0);
            XMVECTOR textureCoordinate = XMVectorSet(u, v, 0, 0);

            XMVECTOR t = XMVectorScale(normal, radius);
            vertices.push_back(VertexPositionColor(XMFLOAT3(XMVectorGetX(t), XMVectorGetY(t), XMVectorGetZ(t)), color));
        }
    }

    // Fill the index buffer with triangles joining each pair of latitude rings.
    size_t stride = horizontalSegments + 1;

    for (size_t i = 0; i < verticalSegments; i++)
    {
        for (size_t j = 0; j <= horizontalSegments; j++)
        {
            size_t nextI = i + 1;
            size_t nextJ = (j + 1) % stride;

            index_push_backHot3dx(indices, i * stride + j);
            index_push_backHot3dx(indices, nextI * stride + j);
            index_push_backHot3dx(indices, i * stride + nextJ);

            index_push_backHot3dx(indices, i * stride + nextJ);
            index_push_backHot3dx(indices, nextI * stride + j);
            index_push_backHot3dx(indices, nextI * stride + nextJ);
        }
    }

    // Build RH above
    if (!rhcoords)
        ReverseWindingHot3dx(indices, vertices);
}

void DirectX::ComputeGeoSphereColor(VertexCollectionColor& vertices, IndexCollectionColor& indices, float diameter, size_t tessellation, bool rhcoords)
{
    vertices.clear();
    indices.clear();

    XMFLOAT4 color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

    // An undirected edge between two vertices, represented by a pair of indexes into a vertex array.
    // Becuse this edge is undirected, (a,b) is the same as (b,a).
    typedef std::pair<uint16_t, uint16_t> UndirectedEdge;

    // Makes an undirected edge. Rather than overloading comparison operators to give us the (a,b)==(b,a) property,
    // we'll just ensure that the larger of the two goes first. This'll simplify things greatly.
    auto makeUndirectedEdge = [](uint16_t a, uint16_t b)
    {
        return std::make_pair(max(a, b), min(a, b));
    };

    // Key: an edge
    // Value: the index of the vertex which lies midway between the two vertices pointed to by the key value
    // This map is used to avoid duplicating vertices when subdividing triangles along edges.
    typedef std::map<UndirectedEdge, uint16_t> EdgeSubdivisionMap;


    static const XMFLOAT3 OctahedronVertices[] =
    {
        // when looking down the negative z-axis (into the screen)
        XMFLOAT3(0,  1,  0), // 0 top
        XMFLOAT3(0,  0, -1), // 1 front
        XMFLOAT3(1,  0,  0), // 2 right
        XMFLOAT3(0,  0,  1), // 3 back
        XMFLOAT3(-1,  0,  0), // 4 left
        XMFLOAT3(0, -1,  0) // 5 bottom
    };
    static const uint16_t OctahedronIndices[] =
    {
        0, 1, 2, // top front-right face
        0, 2, 3, // top back-right face
        0, 3, 4, // top back-left face
        0, 4, 1, // top front-left face
        5, 1, 4, // bottom front-left face
        5, 4, 3, // bottom back-left face
        5, 3, 2, // bottom back-right face
        5, 2, 1 // bottom front-right face
    };

    const float radius = diameter / 2.0f;

    // Start with an octahedron; copy the data into the vertex/index collection.

    std::vector<XMFLOAT3> vertexPositions(std::begin(OctahedronVertices), std::end(OctahedronVertices));

    indices.insert(indices.begin(), std::begin(OctahedronIndices), std::end(OctahedronIndices));

    // We know these values by looking at the above index list for the octahedron. Despite the subdivisions that are
    // about to go on, these values aren't ever going to change because the vertices don't move around in the array.
    // We'll need these values later on to fix the singularities that show up at the poles.
    const uint16_t northPoleIndex = 0;
    const uint16_t southPoleIndex = 5;

    for (size_t iSubdivision = 0; iSubdivision < tessellation; ++iSubdivision)
    {
        assert(indices.size() % 3 == 0); // sanity

        // We use this to keep track of which edges have already been subdivided.
        EdgeSubdivisionMap subdividedEdges;

        // The new index collection after subdivision.
        IndexCollection newIndices;

        const size_t triangleCount = indices.size() / 3;
        for (size_t iTriangle = 0; iTriangle < triangleCount; ++iTriangle)
        {
            // For each edge on this triangle, create a new vertex in the middle of that edge.
            // The winding order of the triangles we output are the same as the winding order of the inputs.

            // Indices of the vertices making up this triangle
            uint16_t iv0 = indices[iTriangle * 3 + 0];
            uint16_t iv1 = indices[iTriangle * 3 + 1];
            uint16_t iv2 = indices[iTriangle * 3 + 2];

            // Get the new vertices
            XMFLOAT3 v01; // vertex on the midpoint of v0 and v1
            XMFLOAT3 v12; // ditto v1 and v2
            XMFLOAT3 v20; // ditto v2 and v0
            uint16_t iv01; // index of v01
            uint16_t iv12; // index of v12
            uint16_t iv20; // index of v20

            // Function that, when given the index of two vertices, creates a new vertex at the midpoint of those vertices.
            auto divideEdge = [&](uint16_t i0, uint16_t i1, XMFLOAT3& outVertex, uint16_t& outIndex)
            {
                const UndirectedEdge edge = makeUndirectedEdge(i0, i1);

                // Check to see if we've already generated this vertex
                auto it = subdividedEdges.find(edge);
                if (it != subdividedEdges.end())
                {
                    // We've already generated this vertex before
                    outIndex = it->second; // the index of this vertex
                    outVertex = vertexPositions[outIndex]; // and the vertex itself
                }
                else
                {
                    // Haven't generated this vertex before: so add it now

                    // outVertex = (vertices[i0] + vertices[i1]) / 2
                    XMStoreFloat3(
                        &outVertex,
                        XMVectorScale(
                            XMVectorAdd(XMLoadFloat3(&vertexPositions[i0]), XMLoadFloat3(&vertexPositions[i1])),
                            0.5f
                        )
                    );

                    outIndex = static_cast<uint16_t>(vertexPositions.size());
                    CheckIndexOverflowHot3dx(outIndex);
                    vertexPositions.push_back(outVertex);

                    // Now add it to the map.
                    auto entry = std::make_pair(edge, outIndex);
                    subdividedEdges.insert(entry);
                }
            };

            // Add/get new vertices and their indices
            divideEdge(iv0, iv1, v01, iv01);
            divideEdge(iv1, iv2, v12, iv12);
            divideEdge(iv0, iv2, v20, iv20);

            // Add the new indices. We have four new triangles from our original one:
            //        v0
            //        o
            //       /a\
            //  v20 o---o v01
            //     /b\c/d\
            // v2 o---o---o v1
            //       v12
            const uint16_t indicesToAdd[] =
            {
                 iv0, iv01, iv20, // a
                iv20, iv12,  iv2, // b
                iv20, iv01, iv12, // c
                iv01,  iv1, iv12, // d
            };
            newIndices.insert(newIndices.end(), std::begin(indicesToAdd), std::end(indicesToAdd));
        }

        indices = std::move(newIndices);
    }

    // Now that we've completed subdivision, fill in the final vertex collection
    vertices.reserve(vertexPositions.size());
    for (auto it = vertexPositions.begin(); it != vertexPositions.end(); ++it)
    {
        auto vertexValue = *it;

        auto normal = XMVector3Normalize(XMLoadFloat3(&vertexValue));
        auto pos = XMVectorScale(normal, radius);

        XMFLOAT3 normalFloat3;
        XMStoreFloat3(&normalFloat3, normal);

        // calculate texture coordinates for this vertex
        float longitude = atan2(normalFloat3.x, -normalFloat3.z);
        float latitude = acos(normalFloat3.y);

        float u = longitude / XM_2PI + 0.5f;
        float v = latitude / XM_PI;

        auto texcoord = XMVectorSet(1.0f - u, v, 0.0f, 0.0f);
        vertices.push_back(VertexPositionColor(XMFLOAT3(XMVectorGetX(pos), XMVectorGetY(pos), XMVectorGetZ(pos)), color));
    }

    // There are a couple of fixes to do. One is a texture coordinate wraparound fixup. At some point, there will be
    // a set of triangles somewhere in the mesh with texture coordinates such that the wraparound across 0.0/1.0
    // occurs across that triangle. Eg. when the left hand side of the triangle has a U coordinate of 0.98 and the
    // right hand side has a U coordinate of 0.0. The intent is that such a triangle should render with a U of 0.98 to
    // 1.0, not 0.98 to 0.0. If we don't do this fixup, there will be a visible seam across one side of the sphere.
    // 
    // Luckily this is relatively easy to fix. There is a straight edge which runs down the prime meridian of the
    // completed sphere. If you imagine the vertices along that edge, they circumscribe a semicircular arc starting at
    // y=1 and ending at y=-1, and sweeping across the range of z=0 to z=1. x stays zero. It's along this edge that we
    // need to duplicate our vertices - and provide the correct texture coordinates.
    size_t preFixupVertexCount = vertices.size();
    for (size_t i = 0; i < preFixupVertexCount; ++i)
    {
        // This vertex is on the prime meridian if position.x and texcoord.u are both zero (allowing for small epsilon).
        bool isOnPrimeMeridian = XMVector2NearEqual(
            XMVectorSet(vertices[i].position.x, vertices[i].position.x, 0.0f, 0.0f),
            XMVectorZero(),
            XMVectorSplatEpsilon());

        if (isOnPrimeMeridian)
        {
            size_t newIndex = vertices.size(); // the index of this vertex that we're about to add
            CheckIndexOverflowHot3dx(newIndex);

            // copy this vertex, correct the texture coordinate, and add the vertex
            VertexPositionColor v = vertices[i];
            //v.textureCoordinate.x = 1.0f;
            vertices.push_back(v);

            // Now find all the triangles which contain this vertex and update them if necessary
            for (size_t j = 0; j < indices.size(); j += 3)
            {
                uint16_t* triIndex0 = &indices[j + 0];
                uint16_t* triIndex1 = &indices[j + 1];
                uint16_t* triIndex2 = &indices[j + 2];

                if (*triIndex0 == i)
                {
                    // nothing; just keep going
                }
                else if (*triIndex1 == i)
                {
                    std::swap(triIndex0, triIndex1); // swap the pointers (not the values)
                }
                else if (*triIndex2 == i)
                {
                    std::swap(triIndex0, triIndex2); // swap the pointers (not the values)
                }
                else
                {
                    // this triangle doesn't use the vertex we're interested in
                    continue;
                }

                // If we got to this point then triIndex0 is the pointer to the index to the vertex we're looking at
                assert(*triIndex0 == i);
                assert(*triIndex1 != i && *triIndex2 != i); // assume no degenerate triangles

                const VertexPositionColor& v0 = vertices[*triIndex0];
                const VertexPositionColor& v1 = vertices[*triIndex1];
                const VertexPositionColor& v2 = vertices[*triIndex2];

                // check the other two vertices to see if we might need to fix this triangle

                if (abs(v0.position.x - v1.position.x) > 0.5f ||
                    abs(v0.position.x - v2.position.x) > 0.5f)
                {
                    // yep; replace the specified index to point to the new, corrected vertex
                    *triIndex0 = static_cast<uint16_t>(newIndex);
                }
            }
        }
    }

    // And one last fix we need to do: the poles. A common use-case of a sphere mesh is to map a rectangular texture onto
    // it. If that happens, then the poles become singularities which map the entire top and bottom rows of the texture
    // onto a single point. In general there's no real way to do that right. But to match the behavior of non-geodesic
    // spheres, we need to duplicate the pole vertex for every triangle that uses it. This will introduce seams near the
    // poles, but reduce stretching.
    auto fixPole = [&](size_t poleIndex)
    {
        auto poleVertex = vertices[poleIndex];
        bool overwrittenPoleVertex = false; // overwriting the original pole vertex saves us one vertex

        for (size_t i = 0; i < indices.size(); i += 3)
        {
            // These pointers point to the three indices which make up this triangle. pPoleIndex is the pointer to the
            // entry in the index array which represents the pole index, and the other two pointers point to the other
            // two indices making up this triangle.
            uint16_t* pPoleIndex;
            uint16_t* pOtherIndex0;
            uint16_t* pOtherIndex1;
            if (indices[i + 0] == poleIndex)
            {
                pPoleIndex = &indices[i + 0];
                pOtherIndex0 = &indices[i + 1];
                pOtherIndex1 = &indices[i + 2];
            }
            else if (indices[i + 1] == poleIndex)
            {
                pPoleIndex = &indices[i + 1];
                pOtherIndex0 = &indices[i + 2];
                pOtherIndex1 = &indices[i + 0];
            }
            else if (indices[i + 2] == poleIndex)
            {
                pPoleIndex = &indices[i + 2];
                pOtherIndex0 = &indices[i + 0];
                pOtherIndex1 = &indices[i + 1];
            }
            else
            {
                continue;
            }

            const auto& otherVertex0 = vertices[*pOtherIndex0];
            const auto& otherVertex1 = vertices[*pOtherIndex1];

            // Calculate the texcoords for the new pole vertex, add it to the vertices and update the index
            VertexPositionColor newPoleVertex = poleVertex;
            newPoleVertex.position.x = (otherVertex0.position.x + otherVertex1.position.x) / 2;
            newPoleVertex.position.y = poleVertex.position.y;

            if (!overwrittenPoleVertex)
            {
                vertices[poleIndex] = newPoleVertex;
                overwrittenPoleVertex = true;
            }
            else
            {
                CheckIndexOverflowHot3dx(vertices.size());

                *pPoleIndex = static_cast<uint16_t>(vertices.size());
                vertices.push_back(newPoleVertex);
            }
        }
    };

    fixPole(northPoleIndex);
    fixPole(southPoleIndex);

    // Build RH above
    if (!rhcoords)
        ReverseWindingHot3dx(indices, vertices);
}

//--------------------------------------------------------------------------------------
// Cylinder / Cone
//--------------------------------------------------------------------------------------
namespace
{
    // Helper computes a point on a unit circle, aligned to the x/z plane and centered on the origin.
    inline XMVECTOR GetCircleVectorColor(size_t i, size_t tessellation)
    {
        float angle = i * XM_2PI / tessellation;
        float dx, dz;

        XMScalarSinCos(&dx, &dz, angle);

        XMVECTORF32 v = { { { dx, 0, dz, 0 } } };
        return v;
    }

    inline XMVECTOR GetCircleTangentColor(size_t i, size_t tessellation)
    {
        float angle = (i * XM_2PI / tessellation) + XM_PIDIV2;
        float dx, dz;

        XMScalarSinCos(&dx, &dz, angle);

        XMVECTORF32 v = { { { dx, 0, dz, 0 } } };
        return v;
    }


    // Helper creates a triangle fan to close the end of a cylinder / cone
    void CreateCylinderCapColor(VertexCollectionColor& vertices, IndexCollectionColor& indices, size_t tessellation, float height, float radius, bool isTop)
    {
        XMFLOAT4 color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

        // Create cap indices.
        for (size_t i = 0; i < tessellation - 2; i++)
        {
            size_t i1 = (i + 1) % tessellation;
            size_t i2 = (i + 2) % tessellation;

            if (isTop)
            {
                std::swap(i1, i2);
            }

            size_t vbase = vertices.size();
            index_push_backHot3dx(indices, vbase);
            index_push_backHot3dx(indices, vbase + i1);
            index_push_backHot3dx(indices, vbase + i2);
        }

        // Which end of the cylinder is this?
        XMVECTOR normal = g_XMIdentityR1;
        XMVECTOR textureScale = g_XMNegativeOneHalf;

        if (!isTop)
        {
            normal = XMVectorNegate(normal);
            textureScale = XMVectorMultiply(textureScale, g_XMNegateX);
        }

        // Create cap vertices.
        for (size_t i = 0; i < tessellation; i++)
        {
            XMVECTOR circleVector = GetCircleVectorColor(i, tessellation);

            XMVECTOR position = XMVectorAdd(XMVectorScale(circleVector, radius), XMVectorScale(normal, height));

            XMVECTOR textureCoordinate = XMVectorMultiplyAdd(XMVectorSwizzle<0, 2, 3, 3>(circleVector), textureScale, g_XMOneHalf);

            vertices.push_back(VertexPositionColor(XMFLOAT3(XMVectorGetX(position), XMVectorGetY(position), XMVectorGetZ(position)), color));
        }
    }
}

void DirectX::ComputeCylinderColor(VertexCollectionColor& vertices, IndexCollectionColor& indices, float height, float diameter, size_t tessellation, bool rhcoords)
{
    vertices.clear();
    indices.clear();

    XMFLOAT4 color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

    if (tessellation < 3)
        throw std::out_of_range("tesselation parameter out of range");

    height /= 2;

    XMVECTOR topOffset = XMVectorScale(g_XMIdentityR1, height);

    float radius = diameter / 2;
    size_t stride = tessellation + 1;

    // Create a ring of triangles around the outside of the cylinder.
    for (size_t i = 0; i <= tessellation; i++)
    {
        XMVECTOR normal = GetCircleVectorColor(i, tessellation);

        XMVECTOR sideOffset = XMVectorScale(normal, radius);

        float u = float(i) / tessellation;

        XMVECTOR textureCoordinate = XMLoadFloat(&u);

        XMVECTOR t = XMVectorAdd(sideOffset, topOffset);
        vertices.push_back(VertexPositionColor(XMFLOAT3(XMVectorGetX(t), XMVectorGetY(t), XMVectorGetZ(t)), color));
        t = XMVectorSubtract(sideOffset, topOffset);
            vertices.push_back(VertexPositionColor(XMFLOAT3(XMVectorGetX(t), XMVectorGetY(t), XMVectorGetZ(t)), color));

        index_push_backHot3dx(indices, i * 2);
        index_push_backHot3dx(indices, (i * 2 + 2) % (stride * 2));
        index_push_backHot3dx(indices, i * 2 + 1);

        index_push_backHot3dx(indices, i * 2 + 1);
        index_push_backHot3dx(indices, (i * 2 + 2) % (stride * 2));
        index_push_backHot3dx(indices, (i * 2 + 3) % (stride * 2));
    }

    // Create flat triangle fan caps to seal the top and bottom.
    CreateCylinderCapColor(vertices, indices, tessellation, height, radius, true);
    CreateCylinderCapColor(vertices, indices, tessellation, height, radius, false);

    // Build RH above
    if (!rhcoords)
        ReverseWindingHot3dx(indices, vertices);
}

void DirectX::ComputeConeColor(VertexCollectionColor& vertices, IndexCollectionColor& indices, float diameter, float height, size_t tessellation, bool rhcoords)
{
    vertices.clear();
    indices.clear();

    XMFLOAT4 color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

    if (tessellation < 3)
        throw std::out_of_range("tesselation parameter out of range");

    height /= 2;

    XMVECTOR topOffset = XMVectorScale(g_XMIdentityR1, height);

    float radius = diameter / 2;
    size_t stride = tessellation + 1;

    // Create a ring of triangles around the outside of the cone.
    for (size_t i = 0; i <= tessellation; i++)
    {
        XMVECTOR circlevec = GetCircleVectorColor(i, tessellation);

        XMVECTOR sideOffset = XMVectorScale(circlevec, radius);

        float u = float(i) / tessellation;

        XMVECTOR textureCoordinate = XMLoadFloat(&u);

        XMVECTOR pt = XMVectorSubtract(sideOffset, topOffset);

        XMVECTOR normal = XMVector3Cross(
            GetCircleTangentColor(i, tessellation),
            XMVectorSubtract(topOffset, pt));
        normal = XMVector3Normalize(normal);

        // Duplicate the top vertex for distinct normals
        vertices.push_back(VertexPositionColor(XMFLOAT3(XMVectorGetX(topOffset), XMVectorGetY(topOffset), XMVectorGetZ(topOffset)), color));
        vertices.push_back(VertexPositionColor(XMFLOAT3(XMVectorGetX(pt), XMVectorGetY(pt), XMVectorGetZ(pt)), color));

        index_push_backHot3dx(indices, i * 2);
        index_push_backHot3dx(indices, (i * 2 + 3) % (stride * 2));
        index_push_backHot3dx(indices, (i * 2 + 1) % (stride * 2));
    }

    // Create flat triangle fan caps to seal the bottom.
    CreateCylinderCapColor(vertices, indices, tessellation, height, radius, false);

    // Build RH above
    if (!rhcoords)
        ReverseWindingHot3dx(indices, vertices);
}

void DirectX::ComputeTorusColor(VertexCollectionColor& vertices, IndexCollectionColor& indices, float diameter, float thickness, size_t tessellation, bool rhcoords)
{
    vertices.clear();
    indices.clear();

    XMFLOAT4 color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f); 
    
    if (tessellation < 3)
        throw std::out_of_range("tesselation parameter out of range");

    size_t stride = tessellation + 1;

    // First we loop around the main ring of the torus.
    for (size_t i = 0; i <= tessellation; i++)
    {
        float u = float(i) / tessellation;

        float outerAngle = i * XM_2PI / tessellation - XM_PIDIV2;

        // Create a transform matrix that will align geometry to
        // slice perpendicularly though the current ring position.
        XMMATRIX transform = XMMatrixTranslation(diameter / 2, 0, 0) * XMMatrixRotationY(outerAngle);

        // Now we loop along the other axis, around the side of the tube.
        for (size_t j = 0; j <= tessellation; j++)
        {
            float v = 1 - float(j) / tessellation;

            float innerAngle = j * XM_2PI / tessellation + XM_PI;
            float dx, dy;

            XMScalarSinCos(&dy, &dx, innerAngle);

            // Create a vertex.
            XMVECTOR normal = XMVectorSet(dx, dy, 0, 0);
            XMVECTOR position = XMVectorScale(normal, thickness / 2);
            XMVECTOR textureCoordinate = XMVectorSet(u, v, 0, 0);

            position = XMVector3Transform(position, transform);
            normal = XMVector3TransformNormal(normal, transform);

            vertices.push_back(VertexPositionColor(XMFLOAT3(XMVectorGetX(position), XMVectorGetY(position), XMVectorGetZ(position)), color));

            // And create indices for two triangles.
            size_t nextI = (i + 1) % stride;
            size_t nextJ = (j + 1) % stride;

            index_push_backHot3dx(indices, i * stride + j);
            index_push_backHot3dx(indices, i * stride + nextJ);
            index_push_backHot3dx(indices, nextI * stride + j);

            index_push_backHot3dx(indices, i * stride + nextJ);
            index_push_backHot3dx(indices, nextI * stride + nextJ);
            index_push_backHot3dx(indices, nextI * stride + j);
        }
    }

    // Build RH above
    if (!rhcoords)
        ReverseWindingHot3dx(indices, vertices);
}

void DirectX::ComputeTetrahedronColor(VertexCollectionColor& vertices, IndexCollectionColor& indices, float size, bool rhcoords)
{
    vertices.clear();
    indices.clear();

    XMFLOAT4 color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
    
    static const XMVECTORF32 verts[4] =
    {
        { { {              0.f,          0.f,        1.f, 0 } } },
        { { {  2.f * SQRT2 / 3.f,          0.f, -1.f / 3.f, 0 } } },
        { { {     -SQRT2 / 3.f,  SQRT6 / 3.f, -1.f / 3.f, 0 } } },
        { { {     -SQRT2 / 3.f, -SQRT6 / 3.f, -1.f / 3.f, 0 } } }
    };

    static const uint32_t faces[4 * 3] =
    {
        0, 1, 2,
        0, 2, 3,
        0, 3, 1,
        1, 3, 2
    };

    for (size_t j = 0; j < _countof(faces); j += 3)
    {
        uint32_t v0 = faces[j];
        uint32_t v1 = faces[j + 1];
        uint32_t v2 = faces[j + 2];

        XMVECTOR normal = XMVector3Cross(
            XMVectorSubtract(verts[v1].v, verts[v0].v),
            XMVectorSubtract(verts[v2].v, verts[v0].v));
        normal = XMVector3Normalize(normal);

        size_t base = vertices.size();
        index_push_backHot3dx(indices, base);
        index_push_backHot3dx(indices, base + 1);
        index_push_backHot3dx(indices, base + 2);

        // Duplicate vertices to use face normals
        XMVECTOR position = XMVectorScale(verts[v0], size);
        vertices.push_back(VertexPositionColor(XMFLOAT3(XMVectorGetX(position), XMVectorGetY(position), XMVectorGetZ(position)), color));


        position = XMVectorScale(verts[v1], size);
        vertices.push_back(VertexPositionColor(XMFLOAT3(XMVectorGetX(position), XMVectorGetY(position), XMVectorGetZ(position)), color));


        position = XMVectorScale(verts[v2], size);
        vertices.push_back(VertexPositionColor(XMFLOAT3(XMVectorGetX(position), XMVectorGetY(position), XMVectorGetZ(position)), color));

    }

    // Built LH above
    if (rhcoords)
        ReverseWindingHot3dx(indices, vertices);

    assert(vertices.size() == 4 * 3);
    assert(indices.size() == 4 * 3);

}

void DirectX::ComputeOctahedronColor(VertexCollectionColor& vertices, IndexCollectionColor& indices, float size, bool rhcoords)
{
    vertices.clear();
    indices.clear();

    XMFLOAT4 color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
    
    static const XMVECTORF32 verts[6] =
    {
        { { {  1,  0,  0, 0 } } },
        { { { -1,  0,  0, 0 } } },
        { { {  0,  1,  0, 0 } } },
        { { {  0, -1,  0, 0 } } },
        { { {  0,  0,  1, 0 } } },
        { { {  0,  0, -1, 0 } } }
    };

    static const uint32_t faces[8 * 3] =
    {
        4, 0, 2,
        4, 2, 1,
        4, 1, 3,
        4, 3, 0,
        5, 2, 0,
        5, 1, 2,
        5, 3, 1,
        5, 0, 3
    };

    for (size_t j = 0; j < _countof(faces); j += 3)
    {
        uint32_t v0 = faces[j];
        uint32_t v1 = faces[j + 1];
        uint32_t v2 = faces[j + 2];

        XMVECTOR normal = XMVector3Cross(
            XMVectorSubtract(verts[v1].v, verts[v0].v),
            XMVectorSubtract(verts[v2].v, verts[v0].v));
        normal = XMVector3Normalize(normal);

        size_t base = vertices.size();
        index_push_backHot3dx(indices, base);
        index_push_backHot3dx(indices, base + 1);
        index_push_backHot3dx(indices, base + 2);

        // Duplicate vertices to use face normals
        XMVECTOR position = XMVectorScale(verts[v0], size);
        vertices.push_back(VertexPositionColor(XMFLOAT3(XMVectorGetX(position), XMVectorGetY(position), XMVectorGetZ(position)), color));


        position = XMVectorScale(verts[v1], size);
        vertices.push_back(VertexPositionColor(XMFLOAT3(XMVectorGetX(position), XMVectorGetY(position), XMVectorGetZ(position)), color));


        position = XMVectorScale(verts[v2], size);
        vertices.push_back(VertexPositionColor(XMFLOAT3(XMVectorGetX(position), XMVectorGetY(position), XMVectorGetZ(position)), color));

    }

    // Built LH above
    if (rhcoords)
        ReverseWindingHot3dx(indices, vertices);

    assert(vertices.size() == 8 * 3);
    assert(indices.size() == 8 * 3);
   
}

void DirectX::ComputeDodecahedronColor(VertexCollectionColor& vertices, IndexCollectionColor& indices, float size, bool rhcoords)
{
    vertices.clear();
    indices.clear();

    XMFLOAT4 color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
    
    static const float a = 1.f / SQRT3;
    static const float b = 0.356822089773089931942f; // sqrt( ( 3 - sqrt(5) ) / 6 )
    static const float c = 0.934172358962715696451f; // sqrt( ( 3 + sqrt(5) ) / 6 );

    static const XMVECTORF32 verts[20] =
    {
        { { {  a,  a,  a, 0 } } },
        { { {  a,  a, -a, 0 } } },
        { { {  a, -a,  a, 0 } } },
        { { {  a, -a, -a, 0 } } },
        { { { -a,  a,  a, 0 } } },
        { { { -a,  a, -a, 0 } } },
        { { { -a, -a,  a, 0 } } },
        { { { -a, -a, -a, 0 } } },
        { { {  b,  c,  0, 0 } } },
        { { { -b,  c,  0, 0 } } },
        { { {  b, -c,  0, 0 } } },
        { { { -b, -c,  0, 0 } } },
        { { {  c,  0,  b, 0 } } },
        { { {  c,  0, -b, 0 } } },
        { { { -c,  0,  b, 0 } } },
        { { { -c,  0, -b, 0 } } },
        { { {  0,  b,  c, 0 } } },
        { { {  0, -b,  c, 0 } } },
        { { {  0,  b, -c, 0 } } },
        { { {  0, -b, -c, 0 } } }
    };

    static const uint32_t faces[12 * 5] =
    {
        0, 8, 9, 4, 16,
        0, 16, 17, 2, 12,
        12, 2, 10, 3, 13,
        9, 5, 15, 14, 4,
        3, 19, 18, 1, 13,
        7, 11, 6, 14, 15,
        0, 12, 13, 1, 8,
        8, 1, 18, 5, 9,
        16, 4, 14, 6, 17,
        6, 11, 10, 2, 17,
        7, 15, 5, 18, 19,
        7, 19, 3, 10, 11
    };

    static const XMVECTORF32 textureCoordinates[5] =
    {
        { { {  0.654508f, 0.0244717f, 0, 0 } } },
        { { { 0.0954915f,  0.206107f, 0, 0 } } },
        { { { 0.0954915f,  0.793893f, 0, 0 } } },
        { { {  0.654508f,  0.975528f, 0, 0 } } },
        { { {        1.f,       0.5f, 0, 0 } } }
    };

    static const uint32_t textureIndex[12][5] =
    {
        { 0, 1, 2, 3, 4 },
        { 2, 3, 4, 0, 1 },
        { 4, 0, 1, 2, 3 },
        { 1, 2, 3, 4, 0 },
        { 2, 3, 4, 0, 1 },
        { 0, 1, 2, 3, 4 },
        { 1, 2, 3, 4, 0 },
        { 4, 0, 1, 2, 3 },
        { 4, 0, 1, 2, 3 },
        { 1, 2, 3, 4, 0 },
        { 0, 1, 2, 3, 4 },
        { 2, 3, 4, 0, 1 }
    };

    size_t t = 0;
    for (size_t j = 0; j < _countof(faces); j += 5, ++t)
    {
        uint32_t v0 = faces[j];
        uint32_t v1 = faces[j + 1];
        uint32_t v2 = faces[j + 2];
        uint32_t v3 = faces[j + 3];
        uint32_t v4 = faces[j + 4];

        XMVECTOR normal = XMVector3Cross(
            XMVectorSubtract(verts[v1].v, verts[v0].v),
            XMVectorSubtract(verts[v2].v, verts[v0].v));
        normal = XMVector3Normalize(normal);

        size_t base = vertices.size();

        index_push_backHot3dx(indices, base);
        index_push_backHot3dx(indices, base + 1);
        index_push_backHot3dx(indices, base + 2);

        index_push_backHot3dx(indices, base);
        index_push_backHot3dx(indices, base + 2);
        index_push_backHot3dx(indices, base + 3);

        index_push_backHot3dx(indices, base);
        index_push_backHot3dx(indices, base + 3);
        index_push_backHot3dx(indices, base + 4);

        // Duplicate vertices to use face normals
        XMVECTOR position = XMVectorScale(verts[v0], size);
        vertices.push_back(VertexPositionColor(XMFLOAT3(XMVectorGetX(position), XMVectorGetY(position), XMVectorGetZ(position)), color));


        position = XMVectorScale(verts[v1], size);
        vertices.push_back(VertexPositionColor(XMFLOAT3(XMVectorGetX(position), XMVectorGetY(position), XMVectorGetZ(position)), color));


        position = XMVectorScale(verts[v2], size);
        vertices.push_back(VertexPositionColor(XMFLOAT3(XMVectorGetX(position), XMVectorGetY(position), XMVectorGetZ(position)), color));


        position = XMVectorScale(verts[v3], size);
        vertices.push_back(VertexPositionColor(XMFLOAT3(XMVectorGetX(position), XMVectorGetY(position), XMVectorGetZ(position)), color));


        position = XMVectorScale(verts[v4], size);
        vertices.push_back(VertexPositionColor(XMFLOAT3(XMVectorGetX(position), XMVectorGetY(position), XMVectorGetZ(position)), color));

    }

    // Built LH above
    if (rhcoords)
        ReverseWindingHot3dx(indices, vertices);

    assert(vertices.size() == 12 * 5);
    assert(indices.size() == 12 * 3 * 3);
}

void DirectX::ComputeIcosahedronColor(VertexCollectionColor& vertices, IndexCollectionColor& indices, float size, bool rhcoords)
{
    vertices.clear();
    indices.clear();

    XMFLOAT4 color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
    
    static const float  t = 1.618033988749894848205f; // (1 + sqrt(5)) / 2
    static const float t2 = 1.519544995837552493271f; // sqrt( 1 + sqr( (1 + sqrt(5)) / 2 ) )

    static const XMVECTORF32 verts[12] =
    {
        { { {    t / t2,  1.f / t2,       0, 0 } } },
        { { {   -t / t2,  1.f / t2,       0, 0 } } },
        { { {    t / t2, -1.f / t2,       0, 0 } } },
        { { {   -t / t2, -1.f / t2,       0, 0 } } },
        { { {  1.f / t2,       0,    t / t2, 0 } } },
        { { {  1.f / t2,       0,   -t / t2, 0 } } },
        { { { -1.f / t2,       0,    t / t2, 0 } } },
        { { { -1.f / t2,       0,   -t / t2, 0 } } },
        { { {       0,    t / t2,  1.f / t2, 0 }  } },
        { { {       0,   -t / t2,  1.f / t2, 0 } } },
        { { {       0,    t / t2, -1.f / t2, 0 } } },
        { { {       0,   -t / t2, -1.f / t2, 0 } } }
    };

    static const uint32_t faces[20 * 3] =
    {
        0, 8, 4,
        0, 5, 10,
        2, 4, 9,
        2, 11, 5,
        1, 6, 8,
        1, 10, 7,
        3, 9, 6,
        3, 7, 11,
        0, 10, 8,
        1, 8, 10,
        2, 9, 11,
        3, 11, 9,
        4, 2, 0,
        5, 0, 2,
        6, 1, 3,
        7, 3, 1,
        8, 6, 4,
        9, 4, 6,
        10, 5, 7,
        11, 7, 5
    };

    for (size_t j = 0; j < _countof(faces); j += 3)
    {
        uint32_t v0 = faces[j];
        uint32_t v1 = faces[j + 1];
        uint32_t v2 = faces[j + 2];

        XMVECTOR normal = XMVector3Cross(
            XMVectorSubtract(verts[v1].v, verts[v0].v),
            XMVectorSubtract(verts[v2].v, verts[v0].v));
        normal = XMVector3Normalize(normal);

        size_t base = vertices.size();
        index_push_backHot3dx(indices, base);
        index_push_backHot3dx(indices, base + 1);
        index_push_backHot3dx(indices, base + 2);

        // Duplicate vertices to use face normals
        XMVECTOR position = XMVectorScale(verts[v0], size);
        vertices.push_back(VertexPositionColor(XMFLOAT3(XMVectorGetX(position), XMVectorGetY(position), XMVectorGetZ(position)), color));


        position = XMVectorScale(verts[v1], size);
        vertices.push_back(VertexPositionColor(XMFLOAT3(XMVectorGetX(position), XMVectorGetY(position), XMVectorGetZ(position)), color));


        position = XMVectorScale(verts[v2], size);
        vertices.push_back(VertexPositionColor(XMFLOAT3(XMVectorGetX(position), XMVectorGetY(position), XMVectorGetZ(position)), color));

    }

    // Built LH above
    if (rhcoords)
        ReverseWindingHot3dx(indices, vertices);

    assert(vertices.size() == 20 * 3);
    assert(indices.size() == 20 * 3);
}


