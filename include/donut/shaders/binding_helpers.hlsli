/*
* Copyright (c) 2021-2024, NVIDIA CORPORATION. All rights reserved.
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

#ifndef BINDING_HELPERS_HLSLI
#define BINDING_HELPERS_HLSLI

#if defined(SPIRV) || defined(TARGET_VULKAN) // Support old-style and new-style platform macros

#define VK_PUSH_CONSTANT [[vk::push_constant]]
#define VK_BINDING(reg,dset) [[vk::binding(reg,dset)]]
#define VK_DESCRIPTOR_SET(dset) ,space##dset

// To use dual-source blending on Vulkan, a pixel shader must specify the same Location but different Index
// decorations for two outputs. In HLSL, that can only be achieved with explicit attributes.
// Use on the declarations of pixel shader outputs.
#define VK_LOCATION(loc) [[vk::location(loc)]]
#define VK_LOCATION_INDEX(loc,idx) [[vk::location(loc)]] [[vk::index(idx)]]

#else

#define VK_PUSH_CONSTANT
#define VK_BINDING(reg,dset) 
#define VK_DESCRIPTOR_SET(dset)
#define VK_LOCATION(loc)
#define VK_LOCATION_INDEX(loc,idx)

#endif

// Helper macro to expand the register and space macros before concatenating tokens.
// Declares a register space explicitly on DX12 and Vulkan, skips it on DX11.
#ifdef TARGET_D3D11
#define REGISTER_HELPER(TY,REG,SPACE) register(TY##REG)
#else
#define REGISTER_HELPER(TY,REG,SPACE) register(TY##REG, space##SPACE)
#endif

// Macros to declare bindings for various resource types in a cross-platform way
// using register and space indices coming from other preprocessor macros.
#define REGISTER_CBUFFER(reg,space) REGISTER_HELPER(b,reg,space)
#define REGISTER_SAMPLER(reg,space) REGISTER_HELPER(s,reg,space)
#define REGISTER_SRV(reg,space)     REGISTER_HELPER(t,reg,space)
#define REGISTER_UAV(reg,space)     REGISTER_HELPER(u,reg,space)

// Macro to declare a constant buffer in a cross-platform way, compatible with the VK_PUSH_CONSTANT attribute.
#ifdef TARGET_D3D11
#define DECLARE_CBUFFER(ty,name,reg,space) cbuffer c_##name : REGISTER_CBUFFER(reg,space) { ty name; }
#else
#define DECLARE_CBUFFER(ty,name,reg,space) ConstantBuffer<ty> name : REGISTER_CBUFFER(reg,space)
#endif

// Macro to declare a push constant block on Vulkan and a regular cbuffer on other platforms.
#define DECLARE_PUSH_CONSTANTS(ty,name,reg,space) VK_PUSH_CONSTANT DECLARE_CBUFFER(ty,name,reg,space)

#endif // BINDING_HELPERS_HLSLI