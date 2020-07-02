//--------------------------------------------------------------------------------------
// File: BinaryWriter.h
//
// Copyright (c) Jeff Kubitz - hot3dx. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#pragma once

#include "..\Common\d3dx12.h"
#include <memory>
#include <exception>
#include <stdexcept>
#include <type_traits>

#include "PlatformHelpers.h"


namespace DirectX
{
    // Helper for reading binary data, either from the filesystem a memory buffer.
    class BinaryWriter
    {
    public:
        explicit BinaryWriter(_In_z_ wchar_t const* fileName);
        BinaryWriter(_In_reads_bytes_(dataSize) uint8_t const* dataBlob, size_t dataSize);

        BinaryWriter(BinaryWriter const&) = delete;
        BinaryWriter& operator= (BinaryWriter const&) = delete;

        // Writes a single value.
        template<typename T> T const& Write()
        {
            return *WriteArray<T>(1);
        }


        // Writes an array of values.
        template<typename T> T const* WriteArray(size_t elementCount)
        {
            static_assert(std::is_pod<T>::value, "Can only read plain-old-data types");

            uint8_t const* newPos = mPos + sizeof(T) * elementCount;

            if (newPos < mPos)
                throw std::overflow_error("WriteArray");

            if (newPos > mEnd)
                throw std::exception("End of file");

            auto result = reinterpret_cast<T const*>(mPos);

            mPos = newPos;

            return result;
        }


        // Lower level helper reads directly from the filesystem into memory.
        static HRESULT WriteEntireFile(_In_z_ wchar_t const* fileName, _Inout_ std::unique_ptr<uint8_t[]>& data, _Out_ size_t* dataSize);


    private:
        // The data currently being read.
        uint8_t const* mPos;
        uint8_t const* mEnd;

        std::unique_ptr<uint8_t[]> mOwnedData;
    };
}
