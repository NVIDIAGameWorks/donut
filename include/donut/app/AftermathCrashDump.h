/*
* Copyright (c) 2014-2024, NVIDIA CORPORATION. All rights reserved.
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
#include <filesystem>
#include <set>
#include <nvrhi/common/aftermath.h>

namespace donut::app
{
    class DeviceManager;

    // AftermathCrashDump contains all interactions with the Aftermath crash dump SDK,
    // and gathers all relevant information to package together with the dump
    class AftermathCrashDump
    {
    public:
        static void WaitForCrashDump(uint32_t maxTimeoutSeconds = 60);
        static uint64_t GetShaderHashForBinary(std::pair<const void*, size_t> shaderBinary, nvrhi::GraphicsAPI api);

        AftermathCrashDump(DeviceManager& deviceManager);

        void EnableCrashDumpTracking();
        // markers are stored with Aftermath as hashed 64bit values
        // this method resolves the hash back to the original human-readable text
        const std::string& ResolveMarker(uint64_t markerHash);

        DeviceManager& GetDeviceManager();
        std::filesystem::path GetDumpFolder();
    private:
        static void InitializeAftermathCrashDump(AftermathCrashDump* dumper);

        DeviceManager& m_deviceManager;
        std::filesystem::path m_dumpFolder;
    };
} // end namespace donut::app