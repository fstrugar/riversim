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

#include "DXUT.h"

#include "wsGPUDataTools.h"

#include "DxEventNotifier.h"

wsGPUToolsContext::wsGPUToolsContext()
{
   m_active = false;
   m_device = DxEventNotifier::GetD3DDevice();

   assert( m_device != NULL );

   m_psNormalMap.SetShaderInfo( "wsGPUDataTools.psh", "calculateNormalMap" );
   m_psNormalMapSpecOutputWithHeight.SetShaderInfo( "wsGPUDataTools.psh", "calculateNormalMapSpecOutputWithHeight" );

   HRESULT hr;

   V( D3DXCreateTextureFromFile( m_device, wsFindResource(L"Media\\noise2.dds").c_str(), &m_noise2 ) );
   V( D3DXCreateTextureFromFile( m_device, wsFindResource(L"Media\\noise3.dds").c_str(), &m_perlinNoise ) );

   BeginScene();
}

wsGPUToolsContext::~wsGPUToolsContext()
{ 
   EndScene();

   SAFE_RELEASE( m_noise2 );
   SAFE_RELEASE( m_perlinNoise );
}

HRESULT wsGPUToolsContext::BeginScene()
{
   if( m_active || FAILED( m_device->BeginScene() ) )
   {
      assert( false );
      return E_FAIL;
   }

   HRESULT hr;
   for( int i = 0; i < 4; i++ )
   {
      if( FAILED( m_device->GetRenderTarget( i, &m_oldRenderTargets[i] ) ) )
         m_oldRenderTargets[i] = NULL;
   }
   V( m_device->GetDepthStencilSurface( &m_oldDepthStencil ) );
   V( m_device->SetDepthStencilSurface( NULL ) );

   m_active = true;

   return S_OK;
}

HRESULT wsGPUToolsContext::EndScene()
{
   if( !m_active || FAILED( m_device->EndScene() ) )
   {
      assert( false );
      return E_FAIL;
   }

   // Not restoring previous one... sorry.
   m_device->SetPixelShader( NULL );

   HRESULT hr;
   for( int i = 0; i < 4; i++ )
   {
      V( m_device->SetRenderTarget( i, m_oldRenderTargets[i] ) );
   }

   m_device->SetDepthStencilSurface( m_oldDepthStencil );

   for( int i = 0; i < 4; i++ )
   {
      SAFE_RELEASE( m_oldRenderTargets[i] );
   }
   SAFE_RELEASE( m_oldDepthStencil );

   m_active = false;

   return S_OK;
}

HRESULT wsGPUToolsContext::CalculateNormalMap( IDirect3DTexture9 * sourceTex, IDirect3DTexture9 * destTex, float worldSizeX, float worldSizeY, float worldSizeZ, bool specOutputWithHeight, bool wrapEdges )
{
   if( !m_active )   {      assert( false );      return E_FAIL;   }

   HRESULT hr;

   D3DSURFACE_DESC srcDesc, dstDesc;
   V( sourceTex->GetLevelDesc( 0, &srcDesc ) );
   V( destTex->GetLevelDesc( 0, &dstDesc ) );

   V( wsSetRenderTarget( 0, destTex ) );

   wsPixelShader & psShader = (!specOutputWithHeight)?(m_psNormalMap):(m_psNormalMapSpecOutputWithHeight);

   V( m_device->SetPixelShader( psShader ) );

   if( wrapEdges )
      psShader.SetTexture( "g_inputTexture", sourceTex, D3DTADDRESS_WRAP, D3DTADDRESS_WRAP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
   else
      psShader.SetTexture( "g_inputTexture", sourceTex, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );

   psShader.SetFloatArray( "g_samplerPixelSize", 1.0f / (float)srcDesc.Width, 1.0f / (float)srcDesc.Height );
   
   //psShader.SetFloatArray( "g_samplerSize", srcDesc.Width, srcDesc.Height );

   float stepX = 1.0f / (srcDesc.Width-1) * worldSizeX;
   float stepY = 1.0f / (srcDesc.Height-1) * worldSizeY;
   psShader.SetFloatArray( "g_worldSpaceInfo", stepX, stepY, worldSizeZ );

   V( wsRenderSimulation( srcDesc.Width, srcDesc.Height, 0 ) );

   psShader.SetTexture( "g_inputTexture", NULL );

   return S_OK;
}

HRESULT wsGPUToolsContext::CalculateSplatMap( IDirect3DTexture9 * terrainHeightmap, IDirect3DTexture9 * terrainNormalmap, IDirect3DTexture9 * waterHeightmap, IDirect3DTexture9 * waterVelocitymap, IDirect3DTexture9 * destTex )
{
   static wsPixelShader psSplatMap( "wsGPUDataTools.psh", "calculateSplatMap" );

   HRESULT hr;

   D3DSURFACE_DESC srcDesc, dstDesc;
   V( terrainHeightmap->GetLevelDesc( 0, &srcDesc ) );
   V( destTex->GetLevelDesc( 0, &dstDesc ) );

   V( wsSetRenderTarget( 0, destTex ) );

   V( m_device->SetPixelShader( psSplatMap ) );

   psSplatMap.SetFloatArray( "g_samplerPixelSize", 1.0f / (float)srcDesc.Width, 1.0f / (float)srcDesc.Height );
   //psSplatMap.SetFloatArray( "g_samplerSize", srcDesc.Width, srcDesc.Height );
   //float stepX = 1.0f / (srcDesc.Width-1) * worldSizeX;
   //float stepY = 1.0f / (srcDesc.Height-1) * worldSizeY;
   //psSplatMap.SetFloatArray( "g_worldSpaceInfo", stepX, stepY, worldSizeZ );

   psSplatMap.SetTexture( "g_terrainHeightmapTexture", terrainHeightmap, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
   psSplatMap.SetTexture( "g_terrainNormalmapTexture", terrainNormalmap, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
   psSplatMap.SetTexture( "g_waterHeightmapTexture", waterHeightmap, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
   psSplatMap.SetTexture( "g_waterVelocitymapTexture", waterVelocitymap, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
   psSplatMap.SetTexture( "g_perlinNoise", m_perlinNoise, D3DTADDRESS_WRAP, D3DTADDRESS_WRAP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );

   V( wsRenderSimulation( srcDesc.Width, srcDesc.Height, 0 ) );

   return S_OK;
}

HRESULT wsGPUToolsContext::CalculateDetailWaveMap( IDirect3DTexture9 * destTex, float time )
{
   static wsPixelShader psShader( "wsGPUDataTools.psh", "calculateDetailMap" );

   HRESULT hr;

   D3DSURFACE_DESC dstDesc;
   V( destTex->GetLevelDesc( 0, &dstDesc ) );

   V( wsSetRenderTarget( 0, destTex ) );

   V( m_device->SetPixelShader( psShader ) );

   //psShader.SetFloatArray( "g_samplerPixelSize", 1.0f / (float)dstDesc.Width, 1.0f / (float)dstDesc.Height );

   psShader.SetTexture( "g_noiseTexture", m_noise2, D3DTADDRESS_WRAP, D3DTADDRESS_WRAP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
   psShader.SetFloat( "g_time", time );

   V( wsRenderSimulation( dstDesc.Width, dstDesc.Height, 0 ) );

   return S_OK;
}
