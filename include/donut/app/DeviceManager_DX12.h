/*
* Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <vector>

#include <donut/app/DeviceManager.h>

#include <Windows.h>
#include <dxgi1_5.h>
#include <dxgidebug.h>

#include <nvrhi/d3d12.h>
#include <nvrhi/validation.h>

class DeviceManager_DX12 : public donut::app::DeviceManager
{
protected:
    nvrhi::RefCountPtr<IDXGIFactory2>               m_DxgiFactory2;
    nvrhi::RefCountPtr<ID3D12Device>                m_Device12;
    nvrhi::RefCountPtr<ID3D12CommandQueue>          m_GraphicsQueue;
    nvrhi::RefCountPtr<ID3D12CommandQueue>          m_ComputeQueue;
    nvrhi::RefCountPtr<ID3D12CommandQueue>          m_CopyQueue;
    nvrhi::RefCountPtr<IDXGISwapChain3>             m_SwapChain;
    DXGI_SWAP_CHAIN_DESC1                           m_SwapChainDesc{};
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC                 m_FullScreenDesc{};
    nvrhi::RefCountPtr<IDXGIAdapter>                m_DxgiAdapter;
    HWND                                            m_hWnd = nullptr;
    bool                                            m_TearingSupported = false;

    std::vector<nvrhi::RefCountPtr<ID3D12Resource>> m_SwapChainBuffers;
    std::vector<nvrhi::TextureHandle>               m_RhiSwapChainBuffers;
    nvrhi::RefCountPtr<ID3D12Fence>                 m_FrameFence;
    std::vector<HANDLE>                             m_FrameFenceEvents;

    UINT64                                          m_FrameCount = 1;

    nvrhi::DeviceHandle                             m_NvrhiDevice;

    std::string                                     m_RendererString;

public:
    std::string GetAdapterName(DXGI_ADAPTER_DESC const& aDesc)
    {
        size_t length = wcsnlen(aDesc.Description, _countof(aDesc.Description));

        std::string name;
        name.resize(length);
        WideCharToMultiByte(CP_ACP, 0, aDesc.Description, int(length), name.data(), int(name.size()), nullptr, nullptr);

        return name;
    }

    const char *GetRendererString() const override
    {
        return m_RendererString.c_str();
    }

    nvrhi::IDevice *GetDevice() const override
    {
        return m_NvrhiDevice;
    }

    void ReportLiveObjects() override;
    bool EnumerateAdapters(std::vector<donut::app::AdapterInfo>& outAdapters) override;

    nvrhi::GraphicsAPI GetGraphicsAPI() const override
    {
        return nvrhi::GraphicsAPI::D3D12;
    }
    
protected:
    bool CreateInstanceInternal() override;
    bool CreateDevice() override;
    bool CreateSwapChain() override;
    void DestroyDeviceAndSwapChain() override;
    void ResizeSwapChain() override;
    nvrhi::ITexture* GetCurrentBackBuffer() override;
    nvrhi::ITexture* GetBackBuffer(uint32_t index) override;
    uint32_t GetCurrentBackBufferIndex() override;
    uint32_t GetBackBufferCount() override;
    bool BeginFrame() override;
    bool Present() override;
    void Shutdown() override;
    bool CreateRenderTargets();
    void ReleaseRenderTargets();
};