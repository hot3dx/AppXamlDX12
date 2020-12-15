//--------------------------------------------------------------------------------------
// File: Hot3dxDrawnObject.h
//
// Copyright (c) Jeff Kubitz - Hot3dx. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#pragma once

#include "VertexTypes.h"
#include "Common\d3dx12.h"
#include <d3d12.h>
#include <memory>
#include <vector>


namespace DirectX
{
    typedef std::vector<DirectX::VertexPositionColor> VertexCollectionColor;
    typedef std::vector<uint16_t> IndexCollectionColor;

    void ComputeBoxColor(VertexCollectionColor& vertices,IndexCollectionColor& indices, const XMFLOAT3& size, bool rhcoords);
    void ComputeSphereColor(VertexCollectionColor& vertices,IndexCollectionColor& indices, float diameter, size_t tessellation, bool rhcoords);
    void ComputeGeoSphereColor(VertexCollectionColor& vertices,IndexCollectionColor& indices, float diameter, size_t tessellation, bool rhcoords);
    void ComputeCylinderColor(VertexCollectionColor& vertices,IndexCollectionColor& indices, float height, float diameter, size_t tessellation, bool rhcoords);
    void ComputeConeColor(VertexCollectionColor& vertices,IndexCollectionColor& indices, float diameter, float height, size_t tessellation, bool rhcoords);
    void ComputeTorusColor(VertexCollectionColor& vertices,IndexCollectionColor& indices, float diameter, float thickness, size_t tessellation, bool rhcoords);
    void ComputeTetrahedronColor(VertexCollectionColor& vertices,IndexCollectionColor& indices, float size, bool rhcoords);
    void ComputeOctahedronColor(VertexCollectionColor& vertices,IndexCollectionColor& indices, float size, bool rhcoords);
    void ComputeDodecahedronColor(VertexCollectionColor& vertices,IndexCollectionColor& indices, float size, bool rhcoords);
    void ComputeIcosahedronColor(VertexCollectionColor& vertices,IndexCollectionColor& indices, float size, bool rhcoords);
    
    class IEffect;
    class ResourceUploadBatch;

    class Hot3dxDrawnObject
    {
    public:
        Hot3dxDrawnObject(Hot3dxDrawnObject const&) = delete;
        Hot3dxDrawnObject& operator= (Hot3dxDrawnObject const&) = delete;

        virtual ~Hot3dxDrawnObject();

        using VertexType = VertexPositionColor;

        // Factory methods.
        static std::unique_ptr<Hot3dxDrawnObject> __cdecl CreateCube(float size = 1, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
        static std::unique_ptr<Hot3dxDrawnObject> __cdecl CreateBox(const XMFLOAT3& size, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
        static std::unique_ptr<Hot3dxDrawnObject> __cdecl CreateSphere(float diameter = 1, size_t tessellation = 16, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
        static std::unique_ptr<Hot3dxDrawnObject> __cdecl CreateGeoSphere(float diameter = 1, size_t tessellation = 3, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
        static std::unique_ptr<Hot3dxDrawnObject> __cdecl CreateCylinder(float height = 1, float diameter = 1, size_t tessellation = 32, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
        static std::unique_ptr<Hot3dxDrawnObject> __cdecl CreateCone(float diameter = 1, float height = 1, size_t tessellation = 32, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
        static std::unique_ptr<Hot3dxDrawnObject> __cdecl CreateTorus(float diameter = 1, float thickness = 0.333f, size_t tessellation = 32, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
        static std::unique_ptr<Hot3dxDrawnObject> __cdecl CreateTetrahedron(float size = 1, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
        static std::unique_ptr<Hot3dxDrawnObject> __cdecl CreateOctahedron(float size = 1, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
        static std::unique_ptr<Hot3dxDrawnObject> __cdecl CreateDodecahedron(float size = 1, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
        static std::unique_ptr<Hot3dxDrawnObject> __cdecl CreateIcosahedron(float size = 1, bool rhcoords = true, _In_opt_ ID3D12Device* device = nullptr);
        static std::unique_ptr<Hot3dxDrawnObject> __cdecl CreateCustom(const std::vector<VertexType>& vertices, const std::vector<uint16_t>& indices, _In_opt_ ID3D12Device* device = nullptr);
        static std::unique_ptr<Hot3dxDrawnObject> __cdecl CreateDrawnObjectColor(const std::vector<VertexPositionColor>& vertices, const std::vector<uint16_t>& indices, _In_opt_ ID3D12Device* device = nullptr);
        static std::unique_ptr<Hot3dxDrawnObject> __cdecl CreateDrawnObjectTexture(const std::vector<VertexType>& vertices, const std::vector<uint16_t>& indices, _In_opt_ ID3D12Device* device = nullptr);

        static void __cdecl CreateCube(std::vector<VertexType>& vertices, std::vector<uint16_t>& indices, float size = 1, bool rhcoords = true);
        static void __cdecl CreateBox(std::vector<VertexType>& vertices, std::vector<uint16_t>& indices, const XMFLOAT3& size, bool rhcoords = true);
        static void __cdecl CreateSphere(std::vector<VertexType>& vertices, std::vector<uint16_t>& indices, float diameter = 1, size_t tessellation = 16, bool rhcoords = true);
        static void __cdecl CreateGeoSphere(std::vector<VertexType>& vertices, std::vector<uint16_t>& indices, float diameter = 1, size_t tessellation = 3, bool rhcoords = true);
        static void __cdecl CreateCylinder(std::vector<VertexType>& vertices, std::vector<uint16_t>& indices, float height = 1, float diameter = 1, size_t tessellation = 32, bool rhcoords = true);
        static void __cdecl CreateCone(std::vector<VertexType>& vertices, std::vector<uint16_t>& indices, float diameter = 1, float height = 1, size_t tessellation = 32, bool rhcoords = true);
        static void __cdecl CreateTorus(std::vector<VertexType>& vertices, std::vector<uint16_t>& indices, float diameter = 1, float thickness = 0.333f, size_t tessellation = 32, bool rhcoords = true);
        static void __cdecl CreateTetrahedron(std::vector<VertexType>& vertices, std::vector<uint16_t>& indices, float size = 1, bool rhcoords = true);
        static void __cdecl CreateOctahedron(std::vector<VertexType>& vertices, std::vector<uint16_t>& indices, float size = 1, bool rhcoords = true);
        static void __cdecl CreateDodecahedron(std::vector<VertexType>& vertices, std::vector<uint16_t>& indices, float size = 1, bool rhcoords = true);
        static void __cdecl CreateIcosahedron(std::vector<VertexType>& vertices, std::vector<uint16_t>& indices, float size = 1, bool rhcoords = true);
        
        // Load VB/IB resources for static geometry.
        void __cdecl LoadStaticBuffers(_In_ ID3D12Device* device, ResourceUploadBatch& resourceUploadBatch);

        // Draw the primitive.
        void __cdecl Draw(_In_ ID3D12GraphicsCommandList* commandList) const;

    private:
        Hot3dxDrawnObject() noexcept(false);

        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;
    };
}

