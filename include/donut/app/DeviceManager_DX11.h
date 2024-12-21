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

#include <string>

#include <donut/app/DeviceManager.h>
#include <donut/core/log.h>

#include <Windows.h>
#include <dxgi1_3.h>
#include <dxgidebug.h>

#include <nvrhi/d3d11.h>
#include <nvrhi/validation.h>

class DeviceManager_DX11 : public donut::app::DeviceManager
{
protected:
    nvrhi::RefCountPtr<IDXGIFactory1> m_DxgiFactory;
    nvrhi::RefCountPtr<IDXGIAdapter> m_DxgiAdapter;
    nvrhi::RefCountPtr<ID3D11Device> m_Device;
    nvrhi::RefCountPtr<ID3D11DeviceContext> m_ImmediateContext;
    nvrhi::RefCountPtr<IDXGISwapChain> m_SwapChain;
    DXGI_SWAP_CHAIN_DESC m_SwapChainDesc{};
    HWND m_hWnd = nullptr;

    nvrhi::DeviceHandle m_NvrhiDevice;
    nvrhi::TextureHandle m_RhiBackBuffer;
    nvrhi::RefCountPtr<ID3D11Texture2D> m_D3D11BackBuffer;

    std::string m_RendererString;

public:
    [[nodiscard]] std::string GetAdapterName(DXGI_ADAPTER_DESC const& aDesc)
    {
        size_t length = wcsnlen(aDesc.Description, _countof(aDesc.Description));

        std::string name;
        name.resize(length);
        WideCharToMultiByte(CP_ACP, 0, aDesc.Description, int(length), name.data(), int(name.size()), nullptr, nullptr);

        return name;
    }

    [[nodiscard]] const char* GetRendererString() const override
    {
        return m_RendererString.c_str();
    }

    [[nodiscard]] nvrhi::IDevice* GetDevice() const override
    {
        return m_NvrhiDevice;
    }

    bool BeginFrame() override;
    void ReportLiveObjects() override;
    bool EnumerateAdapters(std::vector<donut::app::AdapterInfo>& outAdapters) override;

    [[nodiscard]] nvrhi::GraphicsAPI GetGraphicsAPI() const override
    {
        return nvrhi::GraphicsAPI::D3D11;
    }
protected:
    bool CreateInstanceInternal() override;
    bool CreateDevice() override;
    bool CreateSwapChain() override;
    void DestroyDeviceAndSwapChain() override;
    void ResizeSwapChain() override;
    void Shutdown() override;

    nvrhi::ITexture* GetCurrentBackBuffer() override
    {
        return m_RhiBackBuffer;
    }

    nvrhi::ITexture* GetBackBuffer(uint32_t index) override
    {
        if (index == 0)
            return m_RhiBackBuffer;

        return nullptr;
    }

    uint32_t GetCurrentBackBufferIndex() override
    {
        return 0;
    }

    uint32_t GetBackBufferCount() override
    {
        return 1;
    }

    bool Present() override;
    bool CreateRenderTarget();
    void ReleaseRenderTarget();
};