//--------------------------------------------------------------------------------------
// File: BinaryWriter.cpp
//
// Copyright (c) Jeff Kubitz - hot3dx. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "BinaryWriter.h"

using namespace DirectX;


// Constructor reads from the filesystem.
BinaryWriter::BinaryWriter(_In_z_ wchar_t const* fileName) :
    mPos(nullptr),
    mEnd(nullptr)
{
    size_t dataSize;

    HRESULT hr = WriteEntireFile(fileName, mOwnedData, &dataSize);
    if (FAILED(hr))
    {
        DebugTrace("ERROR: BinaryWriter failed (%08X) to load '%ls'\n", hr, fileName);
        throw std::exception("BinaryWriter");
    }

    mPos = mOwnedData.get();
    mEnd = mOwnedData.get() + dataSize;
}


// Constructor reads from an existing memory buffer.
BinaryWriter::BinaryWriter(_In_reads_bytes_(dataSize) uint8_t const* dataBlob, size_t dataSize) :
    mPos(dataBlob),
    mEnd(dataBlob + dataSize)
{
}


// Writes from the filesystem into memory.
HRESULT BinaryWriter::WriteEntireFile(_In_z_ wchar_t const* fileName, _Inout_ std::unique_ptr<uint8_t[]>& data, _Out_ size_t* dataSize)
{
    // Open the file.
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
    ScopedHandle hFile(safe_handle(CreateFile2(fileName, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, OPEN_EXISTING, nullptr)));
#else
    ScopedHandle hFile(safe_handle(CreateFileW(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)));
#endif

    if (!hFile)
        return HRESULT_FROM_WIN32(GetLastError());

    // Get the file size.
    FILE_STANDARD_INFO fileInfo;
    if (!GetFileInformationByHandleEx(hFile.get(), FileStandardInfo, &fileInfo, sizeof(fileInfo)))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // File is too big for 32-bit allocation, so reject read.
    if (fileInfo.EndOfFile.HighPart > 0)
        return E_FAIL;

    // Create enough space for the file data.
    data.reset(new uint8_t[fileInfo.EndOfFile.LowPart]);

    if (!data)
        return E_OUTOFMEMORY;

    // Write the data in.
    DWORD bytesWrite = 0;

    if (!WriteFile(hFile.get(), data.get(), fileInfo.EndOfFile.LowPart, &bytesWrite, nullptr))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (bytesWrite < fileInfo.EndOfFile.LowPart)
        return E_FAIL;

    *dataSize = bytesWrite;

    return S_OK;
}
