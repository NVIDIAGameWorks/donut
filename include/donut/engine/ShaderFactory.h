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
#include <vector>
#include <unordered_map>
#include <nvrhi/nvrhi.h>
#include <memory>
#include <filesystem>
#include <functional>


namespace donut::vfs
{
    class IBlob;
    class IFileSystem;
}

namespace donut::engine
{
    struct ShaderMacro
    {
        std::string name;
        std::string definition;

        ShaderMacro(const std::string& _name, const std::string& _definition)
            : name(_name)
            , definition(_definition)
        { }
    };

    struct StaticShader
    {
        void const* pBytecode = nullptr;
        size_t size = 0;
    };

    #if DONUT_WITH_DX11 && DONUT_WITH_STATIC_SHADERS
    #define DONUT_MAKE_DXBC_SHADER(symbol) donut::engine::StaticShader{symbol,sizeof(symbol)}
    #else
    #define DONUT_MAKE_DXBC_SHADER(symbol) donut::engine::StaticShader()
    #endif

    #if DONUT_WITH_DX12 && DONUT_WITH_STATIC_SHADERS
    #define DONUT_MAKE_DXIL_SHADER(symbol) donut::engine::StaticShader{symbol,sizeof(symbol)}
    #else
    #define DONUT_MAKE_DXIL_SHADER(symbol) donut::engine::StaticShader()
    #endif

    #if DONUT_WITH_VULKAN && DONUT_WITH_STATIC_SHADERS
    #define DONUT_MAKE_SPIRV_SHADER(symbol) donut::engine::StaticShader{symbol,sizeof(symbol)}
    #else
    #define DONUT_MAKE_SPIRV_SHADER(symbol) donut::engine::StaticShader()
    #endif

    // Macro to use with ShaderFactory::CreateStaticPlatformShader.
    // If there are symbols g_MyShader_dxbc, g_MyShader_dxil, g_MyShader_spirv - just use:
    //      CreateStaticPlatformShader(DONUT_MAKE_PLATFORM_SHADER(g_MyShader), defines, shaderDesc);
    // and all available platforms will be resolved automatically.
    #define DONUT_MAKE_PLATFORM_SHADER(basename) DONUT_MAKE_DXBC_SHADER(basename##_dxbc), DONUT_MAKE_DXIL_SHADER(basename##_dxil), DONUT_MAKE_SPIRV_SHADER(basename##_spirv)

    // Similar to DONUT_MAKE_PLATFORM_SHADER but for libraries - they are not available on DX11/DXBC.
    //      CreateStaticPlatformShaderLibrary(DONUT_MAKE_PLATFORM_SHADER_LIBRARY(g_MyShaderLibrary), defines);
    #define DONUT_MAKE_PLATFORM_SHADER_LIBRARY(basename) DONUT_MAKE_DXIL_SHADER(basename##_dxil), DONUT_MAKE_SPIRV_SHADER(basename##_spirv)

    class ShaderFactory
    {
    private:
        nvrhi::DeviceHandle m_Device;
        std::unordered_map<std::string, std::shared_ptr<vfs::IBlob>> m_BytecodeCache;
		std::shared_ptr<vfs::IFileSystem> m_fs;
		std::filesystem::path m_basePath;

    public:
        ShaderFactory(
            nvrhi::DeviceHandle device,
            std::shared_ptr<vfs::IFileSystem> fs,
			const std::filesystem::path& basePath);

        virtual ~ShaderFactory();

        void ClearCache();

        std::shared_ptr<vfs::IBlob> GetBytecode(const char* fileName, const char* entryName);

        // Creates a shader from binary file.
        nvrhi::ShaderHandle CreateShader(const char* fileName, const char* entryName, const std::vector<ShaderMacro>* pDefines, const nvrhi::ShaderDesc& desc);

        // Creates a shader library from binary file.
        nvrhi::ShaderLibraryHandle CreateShaderLibrary(const char* fileName, const std::vector<ShaderMacro>* pDefines);

        // Creates a shader from the bytecode array.
        nvrhi::ShaderHandle CreateStaticShader(StaticShader shader, const std::vector<ShaderMacro>* pDefines, const nvrhi::ShaderDesc& desc);

        // Creates a shader from one of the platform-speficic bytecode arrays, selecting it based on the device's graphics API.
        nvrhi::ShaderHandle CreateStaticPlatformShader(StaticShader dxbc, StaticShader dxil, StaticShader spirv, const std::vector<ShaderMacro>* pDefines, const nvrhi::ShaderDesc& desc);

        // Creates a shader library from the bytecode array.
        nvrhi::ShaderLibraryHandle CreateStaticShaderLibrary(StaticShader shader, const std::vector<ShaderMacro>* pDefines);

        // Creates a shader library from one of the platform-speficic bytecode arrays, selecting it based on the device's graphics API.
        nvrhi::ShaderLibraryHandle CreateStaticPlatformShaderLibrary(StaticShader dxil, StaticShader spirv, const std::vector<ShaderMacro>* pDefines);
      
        // Tries to create a shader from one of the platform-specific bytecode arrays (calling CreateStaticPlatformShader).
        // If that fails (e.g. there is no static bytecode), creates a shader from the filesystem binary file (calling CreateShader).
        nvrhi::ShaderHandle CreateAutoShader(const char* fileName, const char* entryName, StaticShader dxbc, StaticShader dxil, StaticShader spirv, const std::vector<ShaderMacro>* pDefines, const nvrhi::ShaderDesc& desc);

        // Tries to create a shader library from one of the platform-specific bytecode arrays (calling CreateStaticPlatformShaderLibrary).
        // If that fails (e.g. there is no static bytecode), creates a shader library from the filesystem binary file (calling CreateShaderLibrary).
        nvrhi::ShaderLibraryHandle CreateAutoShaderLibrary(const char* fileName, StaticShader dxil, StaticShader spirv, const std::vector<ShaderMacro>* pDefines);

        // Looks up a shader binary based on a provided hash and the function used to generate it
        std::pair<const void*, size_t> FindShaderFromHash(uint64_t hash, std::function<uint64_t(std::pair<const void*, size_t>, nvrhi::GraphicsAPI)> hashGenerator);
    };
}