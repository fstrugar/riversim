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

#include "wsCascadedWaveMapSim.h"

#include "ws3DPreviewCamera.h"

#include "wsCanvas.h"

#include "iniparser\src\IniParser.hpp"

wsCascadedWaveMapSim::wsCascadedWaveMapSim(void)
{
   memset( &m_mapDims, 0, sizeof(m_mapDims) );

   m_noiseTexture1            = NULL;
   m_noiseTexture2            = NULL;

   m_waveMapTempTexture       = NULL;
   m_combinedHeightsTemp      = NULL;
   m_combinedHNormTemp        = NULL;
   //m_waveMapTempTexture1    = NULL;

   //m_waveDetailMapTexture     = NULL;

   m_shaderWaveSim.SetShaderInfo( "wsCascadedWaveSim.psh", "waveSim" );
   m_shaderSimpleFlow.SetShaderInfo( "wsCascadedWaveSim.psh", "simpleFlow" );
   m_shaderUpdateDefaultValuesTexture.SetShaderInfo( "wsCascadedWaveSim.psh", "shaderUpdateDefaultValues" );
   m_shaderRepositionOnly.SetShaderInfo( "wsCascadedWaveSim.psh", "repositionOnly" );
   m_shaderUpdateCombinedHeights.SetShaderInfo( "wsCascadedWaveSim.psh", "updateCombinedHeights" );
   //m_shaderUpdateWaveDetailMap.SetShaderInfo( "wsCascadedWaveSim.psh", "updateWaveDetailMap" );
   m_shaderFoamSim.SetShaderInfo( "wsCascadedWaveSim.psh", "foamSim" );
   m_shaderUpdateCombinedFoam.SetShaderInfo( "wsCascadedWaveSim.psh", "updateCombinedFoam" );

   m_firstTick = true;

   m_ticksFromLastReposition  = 0;
}

wsCascadedWaveMapSim::~wsCascadedWaveMapSim(void)
{
   //assert( m_terrainHMTexture == NULL );
}

void wsCascadedWaveMapSim::Start( const MapDimensions & mapDims, IWaterSurfaceInfoProvider * waterSurfaceInfoProvider )
{
   m_mapDims                  = mapDims;
   m_waterSurfaceInfoProvider = waterSurfaceInfoProvider;
   m_time                     = 0.0f;

   InitializeFromIni();

   m_ticksFromLastReposition  = 0;
}

void wsCascadedWaveMapSim::InitializeFromIni( )
{
   IniParser   iniParser( wsGetIniPath() );
   m_settings.LayerCount               = iniParser.getInt("CascadedWaveSim:layerCount", 1);
   m_settings.TextureResolution        = iniParser.getInt("CascadedWaveSim:textureResolution", 0);
   m_settings.FirstLayerWorldSize      = iniParser.getFloat("CascadedWaveSim:firstLayerWorldSize", 200);
   m_settings.FirstLayerSimTimeStepHz  = iniParser.getFloat("CascadedWaveSim:firstLayerSimTimeStepHz", 15);
   m_settings.FirstLayerWaterSpeedMul  = iniParser.getFloat("CascadedWaveSim:firstLayerWaterSpeedMul", 0);
   m_settings.LayerDeltaScale          = iniParser.getInt("CascadedWaveSim:layerDeltaScale", 4);
   //m_settings.detailWaveTextureResolution  = iniParser.getInt("CascadedWaveSim:detailWaveTextureResolution", 512);

   assert( m_settings.LayerCount >= 0 && m_settings.LayerCount <= 6 );  // limited to 6 for a couple of reasons (not enough resolution in the R16F to sum all layers being one of them..)
}

void wsCascadedWaveMapSim::Stop( )
{
   OnLostDevice();
   OnDestroyDevice();
}

void wsCascadedWaveMapSim::InitializeTextures()
{
   IDirect3DDevice9 * device = GetD3DDevice();
   HRESULT hr;

   if( m_waveMapLayers.size() == 0 )
   {
      m_waveMapLayers.resize( m_settings.LayerCount, NULL );

      const float c_combinedHeightmapPrecisionMod = 64;     // !!WARNING!! if changing these, change them in the shaders too!
      const float c_combinedHeightmapReduceInflMod = 0.7f;   // !!WARNING!! if changing these, change them in the shaders too!

      int layerCount = (int)m_waveMapLayers.size();
      float CWSVertexHeightOffsetAverages[6];

      CWSVertexHeightOffsetAverages[layerCount-1] = 0.5f / c_combinedHeightmapPrecisionMod;
      for( int i = layerCount-2; i >= 0; i-- )
      {
         CWSVertexHeightOffsetAverages[i] = 0.5f / c_combinedHeightmapPrecisionMod + CWSVertexHeightOffsetAverages[i+1] * (float)m_settings.LayerDeltaScale * c_combinedHeightmapReduceInflMod;
      }


      float worldSize      = m_settings.FirstLayerWorldSize;
      float simTimeStep    = 1.0f / m_settings.FirstLayerSimTimeStepHz;
      float waterSpeedMul  = m_settings.FirstLayerWaterSpeedMul;
      for( size_t i = 0; i < m_waveMapLayers.size(); i++ )
      {
         m_waveMapLayers[i] = new WaveMapLayer(device, m_settings.TextureResolution, worldSize, simTimeStep, waterSpeedMul, (int)i, CWSVertexHeightOffsetAverages[i]);

         worldSize     *= (float)m_settings.LayerDeltaScale;
         simTimeStep   *= (float)m_settings.LayerDeltaScale;
         //waterSpeedMul /= (float)m_settings.LayerDeltaScale;

         V( device->SetPixelShader( m_shaderUpdateDefaultValuesTexture ) );

         //wsSetRenderTarget( 0, m_waveMapLayers[i]->m_waveMapPrevious );
         //wsRenderSimulation( m_settings.TextureResolution, m_settings.TextureResolution, 0 );

         wsSetRenderTarget( 0, m_waveMapLayers[i]->m_waveMap );
         wsSetRenderTarget( 1, m_waveMapLayers[i]->m_foamMap );
         wsRenderSimulation( m_settings.TextureResolution, m_settings.TextureResolution, 0 );
         wsSetRenderTarget( 1, NULL );

         //m_autoDeleteOnLostDeviceList.push_back( m_waveMapLayers[i]->m_waveMapPrevious );
         m_autoDeleteOnLostDeviceList.push_back( m_waveMapLayers[i]->m_waveMap );
         m_autoDeleteOnLostDeviceList.push_back( m_waveMapLayers[i]->m_foamMap );
         m_autoDeleteOnLostDeviceList.push_back( m_waveMapLayers[i]->m_combinedHeights );
         m_autoDeleteOnLostDeviceList.push_back( m_waveMapLayers[i]->m_combinedHNormals );
         m_autoDeleteOnLostDeviceList.push_back( m_waveMapLayers[i]->m_combinedFoam );
         m_autoDeleteOnLostDeviceList.push_back( m_waveMapLayers[i]->m_velocityMap );
         //m_autoDeleteOnLostDeviceList.push_back( m_waveMapLayers[i]->m_displayNormalMap );
      }
   }

   if( m_waveMapTempTexture == NULL )
   {
      //assert( m_waveMapTempTexture1 == NULL );
      V( device->CreateTexture( m_settings.TextureResolution, m_settings.TextureResolution, 1, D3DUSAGE_RENDERTARGET, WAVE_SIM_TEXTURE_FORMAT, D3DPOOL_DEFAULT, &m_waveMapTempTexture, NULL ) );
      m_autoDeleteOnLostDeviceList.push_back( m_waveMapTempTexture );

      if( FAILED ( device->CreateTexture( m_settings.TextureResolution, m_settings.TextureResolution, 1, D3DUSAGE_RENDERTARGET, WAVE_FOAM_TEXTURE_FORMAT, D3DPOOL_DEFAULT, &m_foamSimTempTexture, NULL ) ) )
      {
         V( device->CreateTexture( m_settings.TextureResolution, m_settings.TextureResolution, 1, D3DUSAGE_RENDERTARGET, WAVE_FOAM_TEXTURE_FORMAT_2, D3DPOOL_DEFAULT, &m_foamSimTempTexture, NULL ) );
      }
      m_autoDeleteOnLostDeviceList.push_back( m_foamSimTempTexture );

#if WAVE_COMBINED_USE_AUTOMIPMAP
      V( device->CreateTexture( m_settings.TextureResolution, m_settings.TextureResolution, 0, D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP, WAVE_COMBINED_HEIGHTS_TEXTURE_FORMAT, D3DPOOL_DEFAULT, &m_combinedHeightsTemp, NULL ) );
      V( device->CreateTexture( m_settings.TextureResolution, m_settings.TextureResolution, 0, D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP, WAVE_COMBINED_FOAM_TEXTURE_FORMAT, D3DPOOL_DEFAULT, &m_combinedFoamTemp, NULL ) );
#else
      V( device->CreateTexture( m_settings.TextureResolution, m_settings.TextureResolution, 1, D3DUSAGE_RENDERTARGET, WAVE_COMBINED_HEIGHTS_TEXTURE_FORMAT, D3DPOOL_DEFAULT, &m_combinedHeightsTemp, NULL ) );
      V( device->CreateTexture( m_settings.TextureResolution, m_settings.TextureResolution, 1, D3DUSAGE_RENDERTARGET, WAVE_COMBINED_FOAM_TEXTURE_FORMAT, D3DPOOL_DEFAULT, &m_combinedFoamTemp, NULL ) );
#endif
      V( device->CreateTexture( m_settings.TextureResolution, m_settings.TextureResolution, 1, D3DUSAGE_RENDERTARGET, WAVE_COMBINED_H_NORM_TEXTURE_FORMAT, D3DPOOL_DEFAULT, &m_combinedHNormTemp, NULL ) );
      m_autoDeleteOnLostDeviceList.push_back( m_combinedFoamTemp );
      m_autoDeleteOnLostDeviceList.push_back( m_combinedHeightsTemp );
      m_autoDeleteOnLostDeviceList.push_back( m_combinedHNormTemp );

      V( device->SetPixelShader( m_shaderUpdateDefaultValuesTexture ) );

      wsSetRenderTarget( 0, m_waveMapTempTexture );
      wsSetRenderTarget( 1, m_foamSimTempTexture );
      wsRenderSimulation( m_settings.TextureResolution, m_settings.TextureResolution, 0 );
      wsSetRenderTarget( 1, NULL );

      //wsSetRenderTarget( 0, m_combinedHeightsTemp );
   }
}

HRESULT wsCascadedWaveMapSim::OnCreateDevice( )
{
   HRESULT hr;
   IDirect3DDevice9 * device = GetD3DDevice();

   V( D3DXCreateTextureFromFile( device, wsFindResource(L"Media\\noise1.dds").c_str(), &m_noiseTexture1 ) );
   V( D3DXCreateTextureFromFile( device, wsFindResource(L"Media\\noise2.dds").c_str(), &m_noiseTexture2 ) );
   
   return S_OK;
}

HRESULT wsCascadedWaveMapSim::OnResetDevice( const D3DSURFACE_DESC * pBackBufferSurfaceDesc )
{
   //HRESULT hr;
   //IDirect3DDevice9 * device = GetD3DDevice();

   //V( device->CreateTexture( m_settings.detailWaveTextureResolution, m_settings.detailWaveTextureResolution, 0, D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP, D3DFMT_R16F, D3DPOOL_DEFAULT, &m_waveDetailMapTexture, NULL ) );

   //m_waveDetailMapTexture->SetAutoGenFilterType( D3DTEXF_POINT );

   return S_OK;
}

void wsCascadedWaveMapSim::OnLostDevice( )
{
   for( size_t i = 0; i < m_waveMapLayers.size(); i++ )
   {
      delete m_waveMapLayers[i];
   }
   erase( m_waveMapLayers );

   for( size_t i = 0; i < m_autoDeleteOnLostDeviceList.size(); i++ )
   {
      m_autoDeleteOnLostDeviceList[i]->Release();
   }
   erase( m_autoDeleteOnLostDeviceList );
   m_waveMapTempTexture = NULL;
   //m_waveMapTempTexture1 = NULL;

   m_firstTick = true;

   //SAFE_RELEASE( m_waveDetailMapTexture );
   //SAFE_RELEASE( m_waveMapDefaultValuesTexture );
}

void wsCascadedWaveMapSim::OnDestroyDevice( )
{
   SAFE_RELEASE( m_noiseTexture1 );
   SAFE_RELEASE( m_noiseTexture2 );
}

void wsCascadedWaveMapSim::Tick( float deltaTime, ws3DPreviewCamera * camera )
{
   if( IsKeyClicked(kcReloadAll) )
   {
      InitializeFromIni();

      OnLostDevice();
      OnDestroyDevice();
      OnCreateDevice();
      OnResetDevice( NULL );
   }

   //deltaTime *= 0.08f;

   m_time += deltaTime;

   const D3DXVECTOR3 & camPos = camera->GetPosition();

   m_ticksFromLastReposition++;

   for( int i = 0; i < (int)m_waveMapLayers.size(); i++ )
   {
      WaveMapLayer & wmLayer = *m_waveMapLayers[i];

      // We don't need to get area min/max every tick, but that will be optimized sometime later

      // If out of Z range then don't update at all!
      float minZ, maxZ;
      m_waterSurfaceInfoProvider->GetAreaMinMaxHeight( camPos.x - wmLayer.m_worldSize/2.0f, camPos.y - wmLayer.m_worldSize/2.0f, wmLayer.m_worldSize, wmLayer.m_worldSize, minZ, maxZ );

      // Enlarge Z a bit because water can be higher/lower than the actual terrain, and there's (in general) no loss in doing so
      float sizeZ = maxZ - minZ;
      minZ -= sizeZ * 0.2f;
      maxZ += sizeZ * 0.2f;

      // If too far to be rendered, ignore
      if( !m_firstTick && ((camPos.z+wmLayer.m_worldSize) < minZ || (camPos.z-wmLayer.m_worldSize > maxZ )) )
      {
         wmLayer.m_outOfZRange = true;
         continue;
      }
      wmLayer.m_outOfZRange = false;

      // don't do any repositioning if at least 3 ticks haven't passed! why 3? no reason!
      if( m_ticksFromLastReposition < 3 )
         continue;

      if( wmLayer.RepositionIfNeeded( camera ) )
      {
         m_ticksFromLastReposition = 0;
         wmLayer.m_worldMinZ = minZ;
         wmLayer.m_worldMaxZ = maxZ;
         //m_waterSurfaceInfoProvider->UpdateVelocityTexture( wmLayer.m_worldPosX, wmLayer.m_worldPosY, wmLayer.m_worldSize, wmLayer.m_worldSize, m_mapDims.MinZ-wmLayer.m_worldSize, m_mapDims.MinZ+m_mapDims.SizeZ+wmLayer.m_worldSize, wmLayer.m_velocityMap );
         m_waterSurfaceInfoProvider->UpdateVelocityTexture( wmLayer.m_worldPosX, wmLayer.m_worldPosY, wmLayer.m_worldSize, wmLayer.m_worldSize, wmLayer.m_worldMinZ, wmLayer.m_worldMaxZ, wmLayer.m_velocityMap );
      }
   }

   IDirect3DDevice9 * device = GetD3DDevice();
   HRESULT hr;
   V( device->BeginScene() );

   IDirect3DSurface9 * oldRenderTarget;
   IDirect3DSurface9 * oldDepthStencil;
   V( device->GetRenderTarget( 0, &oldRenderTarget ) );
   V( device->GetDepthStencilSurface( &oldDepthStencil ) );
   V( device->SetDepthStencilSurface( NULL ) );

   V( device->SetVertexShader( NULL ) );

   InitializeTextures();

   WaveMapLayer * lessDetailedLayer = NULL;
   for( int i = (int)m_waveMapLayers.size()-1; i >= 0 ; i-- )
   {
      WaveMapLayer & wmLayer = *m_waveMapLayers[i];

      if( m_firstTick ) wmLayer.m_simTimeAccomulator = wmLayer.m_simTimeStep;

      SimulateLayer( wmLayer, deltaTime, lessDetailedLayer );

      if( m_firstTick ) wmLayer.m_simTimeAccomulator = wmLayer.m_simTimeStep;

      lessDetailedLayer = m_waveMapLayers[i];
      //GetCanvas3D()->DrawBox( D3DXVECTOR3(wmLayer.m_worldPosX, wmLayer.m_worldPosY, wmLayer.m_worldMinZ ), D3DXVECTOR3(wmLayer.m_worldPosX + wmLayer.m_worldSize, wmLayer.m_worldPosY + wmLayer.m_worldSize, wmLayer.m_worldMaxZ ), 0xFF0000FF, 0x80FF0080 );
   }

   m_firstTick = false;

   UpdateWaveDetailMap();

   V( device->SetPixelShader( NULL ) );

   V( device->SetRenderTarget( 0, oldRenderTarget ) );
   V( device->SetRenderTarget( 1, NULL ) );
   V( device->SetRenderTarget( 2, NULL ) );
   V( device->SetRenderTarget( 3, NULL ) );

   V( device->SetDepthStencilSurface( oldDepthStencil ) );

   SAFE_RELEASE( oldRenderTarget );
   SAFE_RELEASE( oldDepthStencil );

   V( device->EndScene() );
}

void wsCascadedWaveMapSim::GetLayerInfo( int index, WaveMapLayerInfo & info )
{
   assert( index >= 0 && index < (int)m_waveMapLayers.size() );
   WaveMapLayer & wmLayer = *m_waveMapLayers[index];

   // If layer is out of range, we do not need to return all the other data but I'll leave it in for testing purposes
   info.OutOfRange      = wmLayer.m_outOfZRange;

   info.Aabb.Min = D3DXVECTOR3( wmLayer.m_worldPosX, wmLayer.m_worldPosY, wmLayer.m_worldMinZ - wmLayer.m_worldSize );
   info.Aabb.Max = D3DXVECTOR3( wmLayer.m_worldPosX + wmLayer.m_worldSize, wmLayer.m_worldPosY + wmLayer.m_worldSize, wmLayer.m_worldMaxZ + wmLayer.m_worldSize );

   //GetCanvas3D()->DrawBox( info.Aabb.Min, info.Aabb.Max, ( !info.OutOfRange ) ? (0xFFFF00FF) : (0xFF705070), 0x00FF0000 );

   //waveMap = wmLayer.m_waveMapCurrent;
   //waveMap = wmLayer.m_velocityMap;
   //waveMap = wmLayer.m_displayNormalMap;
   //info.WaveMap = wmLayer.m_waveMapCombinedHeightBack;
   info.CombinedHNormMap   = wmLayer.m_combinedHNormals;
   info.CombinedHeightMap  = wmLayer.m_combinedHeights;
   info.CombinedFoamMap    = wmLayer.m_combinedFoam;
   info.WaterVelocityMap   = wmLayer.m_velocityMap;

   const float morphFactor = 0.6f;

   info.FadeOutFrom     = wmLayer.m_visibleRange * morphFactor;
   info.FadeOutTo       = wmLayer.m_visibleRange;

   info.DetailUVDiv    = powf( (float)m_settings.LayerDeltaScale, (float)index*0.15f );

   info.TimeLerpK = fmodf( wmLayer.m_simTimeAccomulator / wmLayer.m_simTimeStep, 1.0 );
   assert( info.TimeLerpK >= 0 && info.TimeLerpK <= 1 );

   info.CombinedHeightMapAverage = wmLayer.m_combinedHeightsMapAverage;
}

wsCascadedWaveMapSim::WaveMapLayer::WaveMapLayer( IDirect3DDevice9 * device, int resolution, float worldSize, float simTimeStep, float waterSpeedMul, int level, float combinedHeightsMapAverage )
{
   HRESULT hr;

   static const float      c_visibleRangeRatio = 0.8f; // must be < 1.0 - the lower the number, the lower the visibility, but less updates due to camera movement

   m_level        = level;
   m_simTickCount = 0;
   m_worldPosX    = FLT_MAX;
   m_worldPosY    = FLT_MAX;
   m_worldSize    = worldSize;
   m_visibleRange = worldSize * c_visibleRangeRatio * 0.5f;
   m_worldMinZ    = -FLT_MAX;
   m_worldMaxZ    = FLT_MAX;
   m_outOfZRange  = false;
   m_simTimeAccomulator  = 0;
   m_simTimeStep  = simTimeStep;
   m_repositionOffsetUVX = 0;
   m_repositionOffsetUVY = 0;
   m_waterSpeedMul = waterSpeedMul;
   m_resolution   = resolution;
   m_randUVX      = 0.0f;
   m_randUVY      = 0.0f;
   m_combinedHeightsMapAverage = combinedHeightsMapAverage;

   V( device->CreateTexture( resolution, resolution, 1, D3DUSAGE_RENDERTARGET, WAVE_VELOCITY_MAP_TEXTURE_FORMAT, D3DPOOL_DEFAULT, &m_velocityMap, NULL ) );

   //V( device->CreateTexture( resolution, resolution, 1, D3DUSAGE_RENDERTARGET, WAVE_SIM_TEXTURE_FORMAT, D3DPOOL_DEFAULT, &m_waveMapPrevious, NULL ) );
   V( device->CreateTexture( resolution, resolution, 1, D3DUSAGE_RENDERTARGET, WAVE_SIM_TEXTURE_FORMAT, D3DPOOL_DEFAULT, &m_waveMap, NULL ) );

   if( FAILED ( device->CreateTexture( resolution, resolution, 1, D3DUSAGE_RENDERTARGET, WAVE_FOAM_TEXTURE_FORMAT, D3DPOOL_DEFAULT, &m_foamMap, NULL ) ) )
   {
      V( device->CreateTexture( resolution, resolution, 1, D3DUSAGE_RENDERTARGET, WAVE_FOAM_TEXTURE_FORMAT_2, D3DPOOL_DEFAULT, &m_foamMap, NULL ) );
   }

   
#if WAVE_COMBINED_USE_AUTOMIPMAP
   V( device->CreateTexture( resolution, resolution, 0, D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP, WAVE_COMBINED_HEIGHTS_TEXTURE_FORMAT, D3DPOOL_DEFAULT, &m_combinedHeights, NULL ) );
   V( device->CreateTexture( resolution, resolution, 0, D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP, WAVE_COMBINED_FOAM_TEXTURE_FORMAT, D3DPOOL_DEFAULT, &m_combinedFoam, NULL ) );
#else
   V( device->CreateTexture( resolution, resolution, 1, D3DUSAGE_RENDERTARGET, WAVE_COMBINED_HEIGHTS_TEXTURE_FORMAT, D3DPOOL_DEFAULT, &m_combinedHeights, NULL ) );
   V( device->CreateTexture( resolution, resolution, 1, D3DUSAGE_RENDERTARGET, WAVE_COMBINED_FOAM_TEXTURE_FORMAT, D3DPOOL_DEFAULT, &m_combinedFoam, NULL ) );
#endif
   V( device->CreateTexture( resolution, resolution, 1, D3DUSAGE_RENDERTARGET, WAVE_COMBINED_H_NORM_TEXTURE_FORMAT, D3DPOOL_DEFAULT, &m_combinedHNormals, NULL ) );
}
//
void wsCascadedWaveMapSim::UpdateWaveDetailMap( )
{
   /*
   HRESULT hr;

   D3DSURFACE_DESC desc;
   m_waveDetailMapTexture->GetLevelDesc(0, &desc);
   wsSetRenderTarget( 0, m_waveDetailMapTexture );
   V( GetD3DDevice()->SetPixelShader( m_shaderUpdateWaveDetailMap ) );

   m_shaderUpdateWaveDetailMap.SetFloat( "g_time", (float)fmod(m_time, 1000.0) );

   m_shaderUpdateWaveDetailMap.SetTexture( "g_noiseTexture", m_noiseTexture2, D3DTADDRESS_WRAP, D3DTADDRESS_WRAP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );

   wsRenderSimulation( desc.Width, desc.Height, 0 );
   */
}
//
void wsCascadedWaveMapSim::UpdateCombinedTexture( WaveMapLayer & layer, WaveMapLayer * lessDetailedLayer, wsPixelShader & shader, IDirect3DTexture9 * srcTexture, IDirect3DTexture9 * srcCombinedLoTexture, IDirect3DTexture9 * destTexture )
{
   //
   // This function updates layer.m_combinedHeights which combines current and previous waveSim steps into one texture (x is current, y is previous),
   // and combines upper (less detailed) layer output height values into given layer so that only one layer needs to be sampled during rendering.
   //
   HRESULT hr;
   //float v[4];
   IDirect3DDevice9 * device = GetD3DDevice();

   shader.SetBool( "g_hasLoDetailLayer", lessDetailedLayer != NULL );
   if( lessDetailedLayer != NULL )
   {
      float timeLerpK = fmodf( lessDetailedLayer->m_simTimeAccomulator / lessDetailedLayer->m_simTimeStep, 1.0 );

      shader.SetFloat( "g_loDetailLayerTimeLerpK", timeLerpK );
      shader.SetFloat( "g_loDetailLayerHeightMultiplier", (float)m_settings.LayerDeltaScale );
      shader.SetTexture( "g_loDetailLayerCombinedTexture", srcCombinedLoTexture, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );

      D3DXVECTOR4 convConsts;
      convConsts.x = (layer.m_worldPosX - lessDetailedLayer->m_worldPosX) / lessDetailedLayer->m_worldSize;
      convConsts.y = (layer.m_worldPosY - lessDetailedLayer->m_worldPosY) / lessDetailedLayer->m_worldSize;
      convConsts.z = layer.m_worldSize / lessDetailedLayer->m_worldSize;
      convConsts.w = layer.m_worldSize / lessDetailedLayer->m_worldSize;

      shader.SetVector( "g_loDetailTexCoordsConvConsts", convConsts );
   }

   //////////////////////////////////////////////////////////////////////////
   // Update combined heights map now!
   wsSetRenderTarget( 0, destTexture );
   V( device->SetPixelShader( shader ) );
   shader.SetTexture( "g_combinedPrevTexture", srcTexture );
   shader.SetTexture( "g_waveSimTexture", layer.m_waveMap, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
   shader.SetTexture( "g_foamSimTexture", layer.m_foamMap, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );

   wsRenderSimulation( m_settings.TextureResolution, m_settings.TextureResolution, 0 );

   shader.SetTexture( "g_waveSimTexture", NULL );
   shader.SetTexture( "g_foamSimTexture", NULL );

   //m_shaderUpdateCombinedHeights.SetTexture( "g_waveSimTexturePrev", NULL );
   shader.SetTexture( "g_combinedPrevTexture", NULL );
   shader.SetTexture( "g_loDetailLayerCombinedTexture", NULL );
   //m_shaderUpdateCombinedHeights.SetTexture( "g_loDetailLayerHeightMapPrevTexture", NULL );
   //
   //////////////////////////////////////////////////////////////////////////
}
//
void wsCascadedWaveMapSim::RepositionLayer( WaveMapLayer & layer, IDirect3DTexture9 * srcTexture, IDirect3DTexture9 * destTexture, int ignoreBorderWidth )
{
   HRESULT hr;
   IDirect3DDevice9 * device = GetD3DDevice();

   wsSetRenderTarget( 0, destTexture );
   V( device->SetPixelShader( m_shaderRepositionOnly ) );

   m_shaderRepositionOnly.SetTexture( "g_inputTexture", srcTexture, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_POINT );
   m_shaderRepositionOnly.SetFloatArray( "g_repositionOffsetUV", layer.m_repositionOffsetUVX, layer.m_repositionOffsetUVY );

   wsRenderSimulation( m_settings.TextureResolution, m_settings.TextureResolution, ignoreBorderWidth );

   m_shaderRepositionOnly.SetTexture( "g_inputTexture", NULL );
}
//
void wsCascadedWaveMapSim::UpdateSimpleFlow( WaveMapLayer & layer, IDirect3DTexture9 * srcTexture, IDirect3DTexture9 * destTexture )
{
   HRESULT hr;
   IDirect3DDevice9 * device = GetD3DDevice();

   wsSetRenderTarget( 0, destTexture );
   V( device->SetPixelShader( m_shaderSimpleFlow ) );

   V( m_shaderSimpleFlow.SetFloat( "g_waterSpeedMul", layer.m_waterSpeedMul ) );
   V( m_shaderSimpleFlow.SetTexture( "g_velocitymapTexture", layer.m_velocityMap, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR ) );
   V( m_shaderSimpleFlow.SetTexture( "g_inputTexture", srcTexture, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR ) );

   wsRenderSimulation( m_settings.TextureResolution, m_settings.TextureResolution, 1 );

   V( m_shaderSimpleFlow.SetTexture( "g_inputTexture", NULL ) );
   V( m_shaderSimpleFlow.SetTexture( "g_velocitymapTexture", NULL ) );
}
//
void wsCascadedWaveMapSim::SimulateLayer( WaveMapLayer & layer, float deltaTime, WaveMapLayer * lessDetailedLayer )
{
   HRESULT hr;
   IDirect3DDevice9 * device = GetD3DDevice();

   if( layer.m_outOfZRange )
      return;

   bool updateOutput = false; // should we update output (combined height map and normal map)?
   bool updateSim = false;

   layer.m_simTimeAccomulator += deltaTime;
   if( layer.m_simTimeAccomulator >= layer.m_simTimeStep )
   {
      layer.m_simTimeAccomulator -= layer.m_simTimeStep;
      layer.m_simTimeAccomulator = ::min( layer.m_simTimeAccomulator, 1.0f );
      updateSim = true;
   }

   if( layer.m_repositionOffsetUVX != 0 || layer.m_repositionOffsetUVY != 0 )
   {
      RepositionLayer( layer, layer.m_waveMap, m_waveMapTempTexture, 1 );
      ::swap( layer.m_waveMap, m_waveMapTempTexture );
      RepositionLayer( layer, layer.m_combinedHeights, m_combinedHeightsTemp, 0 );
      ::swap( layer.m_combinedHeights, m_combinedHeightsTemp );
      RepositionLayer( layer, layer.m_combinedHNormals, m_combinedHNormTemp, 0 );
      ::swap( layer.m_combinedHNormals, m_combinedHNormTemp );
      RepositionLayer( layer, layer.m_foamMap, m_foamSimTempTexture, 1 );
      ::swap( layer.m_foamMap, m_foamSimTempTexture );
      RepositionLayer( layer, layer.m_combinedFoam, m_combinedFoamTemp, 0 );
      ::swap( layer.m_combinedFoam, m_combinedFoamTemp );

      layer.m_repositionOffsetUVX = 0;
      layer.m_repositionOffsetUVY = 0;

      updateOutput = true;
   }

   // Do we need to update sim?
   if( updateSim )
   {
      //////////////////////////////////////////////////////////////////////////
      // Do the simulation 
      //
      layer.m_simTickCount++;
      if( layer.m_simTickCount % 4 == 0 )
      {
         layer.m_randUVX = randf(); 
         layer.m_randUVY = randf();
      }
      //
      //////////////////////////////////////////////////////////////////////////
      // First do simple splash stuff
      {
         if( layer.m_accumulatedSplashes.size() != 0 )
         {
            static wsPixelShader psDoSimpleWaveSplash( "wsCascadedWaveSim.psh", "doSimpleWaveSplash" );

            //psDoSimpleWaveSplash.SetTexture( "g_waveSimTexture", layer.m_waveMap, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
            device->SetPixelShader( psDoSimpleWaveSplash );
            //wsSetRenderTarget( 0, m_waveMapTempTexture );
            wsSetRenderTarget( 0, layer.m_waveMap );

            for( size_t spli = 0; spli < layer.m_accumulatedSplashes.size(); spli++ )
            {
               const SimpleSplash & splash = layer.m_accumulatedSplashes[spli];

               psDoSimpleWaveSplash.SetFloatArray( "g_splashUV", splash.wmU, splash.wmV );
               psDoSimpleWaveSplash.SetFloat( "g_splashRadius", splash.wmRadius );

               int fromX   = ::max( 0, -1 + (int) (m_settings.TextureResolution * (splash.wmU - splash.wmRadius)) );
               int fromY   = ::max( 0, -1 + (int) (m_settings.TextureResolution * (splash.wmV - splash.wmRadius)) );
               int toX     = ::min( m_settings.TextureResolution-1, 1 + (int)ceilf( m_settings.TextureResolution * (splash.wmU + splash.wmRadius) ) );
               int toY     = ::min( m_settings.TextureResolution-1, 1 + (int)ceilf( m_settings.TextureResolution * (splash.wmV + splash.wmRadius) ) );

               wsRenderSimulation( fromX, fromY, toX, toY, m_settings.TextureResolution, m_settings.TextureResolution );
            }

            //psDoSimpleWaveSplash.SetTexture( "g_waveSimTexture", NULL );
            //::swap( layer.m_waveMap, m_waveMapTempTexture );

            ::erase( layer.m_accumulatedSplashes );
         }
      }
      //////////////////////////////////////////////////////////////////////////
      // Then do the wave sim
      //
      wsSetRenderTarget( 0, m_waveMapTempTexture );
      V( device->SetPixelShader( m_shaderWaveSim ) );
      //   
      m_shaderWaveSim.SetTexture( "g_velocitymapTexture", layer.m_velocityMap, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
      m_shaderWaveSim.SetTexture( "g_waveSimTexture", layer.m_waveMap, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
      m_shaderWaveSim.SetTexture( "g_noiseTexture", m_noiseTexture1, D3DTADDRESS_WRAP, D3DTADDRESS_WRAP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
      //
      float damping = 1.0f - 1.0f / ( 512.0f / powf(3.0f, (float)layer.m_level) );
      m_shaderWaveSim.SetFloat( "g_waveSimDamping", damping );
      //
      m_shaderWaveSim.SetFloatArray( "g_samplerPixelSize", 1.0f / (float)m_settings.TextureResolution, 1.0f / (float)m_settings.TextureResolution );
      //
      m_shaderWaveSim.SetFloat( "g_waterSpeedMul", layer.m_waterSpeedMul );
      //
      m_shaderWaveSim.SetFloatArray( "g_noiseTexUVAdd", layer.m_randUVX, layer.m_randUVY );
      //
      wsRenderSimulation( m_settings.TextureResolution, m_settings.TextureResolution, 1 );
      m_shaderWaveSim.SetTexture( "g_waveSimTexture", NULL );
      wsSetRenderTarget( 1, NULL );
      //
      ::swap( layer.m_waveMap, m_waveMapTempTexture );
      //////////////////////////////////////////////////////////////////////////
      //
      //////////////////////////////////////////////////////////////////////////
      // Foam sim
      //////////////////////////////////////////////////////////////////////////
      wsSetRenderTarget( 0, m_foamSimTempTexture );
      V( device->SetPixelShader( m_shaderFoamSim ) );
      //   
      m_shaderFoamSim.SetTexture( "g_velocitymapTexture", layer.m_velocityMap, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
      m_shaderFoamSim.SetTexture( "g_foamSimTexture", layer.m_foamMap, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
      m_shaderFoamSim.SetTexture( "g_waveSimTexture", layer.m_waveMap, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
      m_shaderFoamSim.SetTexture( "g_noiseTexture", m_noiseTexture2, D3DTADDRESS_WRAP, D3DTADDRESS_WRAP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
      //
      m_shaderFoamSim.SetFloatArray( "g_samplerPixelSize", 1.0f / (float)m_settings.TextureResolution, 1.0f / (float)m_settings.TextureResolution );
      //
      m_shaderFoamSim.SetFloat( "g_waterSpeedMul", layer.m_waterSpeedMul );
      //
      //m_shaderFoamSim.SetFloatArray( "g_noiseTexUVAdd", randf(), randf() );
      //m_shaderFoamSim.SetFloatArray( "g_noiseTexUVAdd", layer.m_randUVX, layer.m_randUVY );
      m_shaderFoamSim.SetFloatArray( "g_noiseTexUVAdd", fmodf( layer.m_simTickCount / 1000.0f, 1000.0f ), fmodf( layer.m_simTickCount / 1000.0f, 100.0f ) );
      m_shaderFoamSim.SetFloat( "g_layerLevelBasedFoamMul", powf(1.0f + layer.m_level, 1.0f) );
      //
      wsRenderSimulation( m_settings.TextureResolution, m_settings.TextureResolution, 1 );
      m_shaderFoamSim.SetTexture( "g_waveSimTexture", NULL );
      m_shaderFoamSim.SetTexture( "g_foamSimTexture", NULL );
      wsSetRenderTarget( 1, NULL );
      //
      ::swap( layer.m_foamMap, m_foamSimTempTexture );
      //////////////////////////////////////////////////////////////////////////
      //
      //////////////////////////////////////////////////////////////////////////
      //
      UpdateCombinedTexture( layer, lessDetailedLayer, m_shaderUpdateCombinedHeights, layer.m_combinedHeights, 
                              (lessDetailedLayer==0)?(NULL):(lessDetailedLayer->m_combinedHeights), m_combinedHeightsTemp );
      ::swap( layer.m_combinedHeights, m_combinedHeightsTemp );
      //
      UpdateCombinedTexture( layer, lessDetailedLayer, m_shaderUpdateCombinedFoam, layer.m_combinedFoam, 
         (lessDetailedLayer==0)?(NULL):(lessDetailedLayer->m_combinedFoam), m_combinedFoamTemp );
      ::swap( layer.m_combinedFoam, m_combinedFoamTemp );
      //
      {
         static wsPixelShader psUpdateCombinedHNormals( "wsCascadedWaveSim.psh", "updateCombinedHNormals" );
         
         psUpdateCombinedHNormals.SetTexture( "g_combinedHeightMapTexture", layer.m_combinedHeights, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
         psUpdateCombinedHNormals.SetFloatArray( "g_samplerPixelSize", 1.0f / (float)m_settings.TextureResolution, 1.0f / (float)m_settings.TextureResolution );
         device->SetPixelShader( psUpdateCombinedHNormals );
         wsSetRenderTarget( 0, layer.m_combinedHNormals );
         wsRenderSimulation( m_settings.TextureResolution, m_settings.TextureResolution, 0 );
         psUpdateCombinedHNormals.SetTexture( "g_combinedHeightMapTexture", NULL );
      }
      //UpdateCombinedTexture( layer, lessDetailedLayer, m_shaderUpdateCombinedFoam, layer.m_combinedFoam, 
      //   (lessDetailedLayer==0)?(NULL):(lessDetailedLayer->m_combinedFoam), m_combinedFoamTemp );
      //::swap( layer.m_combinedFoam, m_combinedFoamTemp );
      //
      updateOutput = true;
      //////////////////////////////////////////////////////////////////////////
   }

   if( updateOutput )
   {
      //UpdateOutput( layer );
   }

}
//
void wsCascadedWaveMapSim::DoTestSplash( const D3DXVECTOR3 & pos, float radius, float intensity )
{
   for( size_t i = 0; i < m_waveMapLayers.size() ; i++ )
   {
      WaveMapLayer & wmLayer = *m_waveMapLayers[i];

      wsAABB aabb = wmLayer.GetAabb();
      if( !aabb.IntersectSphereSq(pos, radius*radius) )
         continue;

      float layU        = (pos.x - aabb.Min.x) / aabb.Size().x;
      float layV        = (pos.y - aabb.Min.y) / aabb.Size().y;
      float layRadius   = radius / wmLayer.m_worldSize;

      if( layRadius < (1.0f / (wmLayer).m_resolution) )
         continue;

      if( wmLayer.m_accumulatedSplashes.size() > 30 )
         continue;

      wmLayer.m_accumulatedSplashes.push_back( SimpleSplash( layU, layV, layRadius ) );
   }
}
//
wsCascadedWaveMapSim::WaveMapLayer::~WaveMapLayer( )
{
}

bool wsCascadedWaveMapSim::WaveMapLayer::RepositionIfNeeded( ws3DPreviewCamera * camera )
{
   const D3DXVECTOR3 & camPos = camera->GetPosition();
   D3DXVECTOR3 boxCenter = D3DXVECTOR3( m_worldPosX + m_worldSize / 2.0f, m_worldPosY + m_worldSize / 2.0f, 0 );

   D3DXVECTOR3 delta = boxCenter - camPos;
   delta.z = 0;

   float diff = sqrtf( sqr(delta.x) + sqr(delta.y) );

   if( diff < (m_worldSize/2 - m_visibleRange) )
   {
      // No need to reposition
      return false;
   }

   float oldPosX = m_worldPosX;
   float oldPosY = m_worldPosY;

   m_worldPosX = camPos.x - m_worldSize / 2.0f;
   m_worldPosY = camPos.y - m_worldSize / 2.0f;

   float pixelSizeX = m_worldSize / (float)m_resolution;
   float pixelSizeY = m_worldSize / (float)m_resolution;
   m_worldPosX = floorf(m_worldPosX / pixelSizeX) * pixelSizeX;
   m_worldPosY = floorf(m_worldPosY / pixelSizeY) * pixelSizeY;

   m_repositionOffsetUVX = (m_worldPosX - oldPosX) / m_worldSize;
   m_repositionOffsetUVY = (m_worldPosY - oldPosY) / m_worldSize;


   return true;
}

//