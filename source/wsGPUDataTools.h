//////////////////////////////////////////////////////////////////////aatags<aa78hjy>//
// AdVantage Terrain SDK, Copyright (C) 2004 - 2008 Filip Strugar.
// 
// Distributed as a FREE PUBLIC part of the SDK source code under the following rules:
// 
// * This code file can be used on 'as-is' basis. Author is not liable for any damages arising from its use.
// * The distribution of a modified version of the software is subject to the following restrictions:
//  1. The origin of this software must not be misrepresented; you must not claim that you wrote the original 
//     software. If you use this software in a product, an acknowledgment in the product documentation would be 
//     appreciated but is not required.
//  2. Altered source versions must be plainly marked as such and must not be misrepresented as being the original
//     software.
//  3. The license notice must not be removed from any source distributions.
//////////////////////////////////////////////////////////////////////aatage<aa78hjy>//

#pragma once

#include "wsCommon.h"
#include "wsShader.h"

class wsGPUToolsContext
{
private:
   bool                    m_active;
   IDirect3DDevice9 *      m_device;

   IDirect3DSurface9 *     m_oldRenderTargets[4];
   IDirect3DSurface9 *     m_oldDepthStencil;
   
   wsPixelShader           m_psNormalMap;
   wsPixelShader           m_psNormalMapSpecOutputWithHeight;

   IDirect3DTexture9 *     m_noise2;
   IDirect3DTexture9 *     m_perlinNoise;

public:
   wsGPUToolsContext();
   ~wsGPUToolsContext();

   //////////////////////////////////////////////////////////////////////////

   HRESULT                 CalculateNormalMap( IDirect3DTexture9 * sourceTex, IDirect3DTexture9 * destTex, float worldSizeX, float worldSizeY, float worldSizeZ, bool specOutputWithHeight = false, bool wrapEdges = false );

   HRESULT                 CalculateSplatMap( IDirect3DTexture9 * terrainHeightmap, IDirect3DTexture9 * terrainNormalmap, IDirect3DTexture9 * waterHeightmap, IDirect3DTexture9 * waterVelocitymap, IDirect3DTexture9 * destTex );

   HRESULT                 CalculateDetailWaveMap( IDirect3DTexture9 * destTex, float time );

   //////////////////////////////////////////////////////////////////////////

private:
   // BeginScene will setup DirectX for tools execution, and EndScene will restore its previous state. All other functions present
   // in this class must happen in between these two calls.
   HRESULT                 BeginScene();
   HRESULT                 EndScene();
};

