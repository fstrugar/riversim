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

#include "ws3DPreviewRenderer.h"

#include "ws3DPreviewCamera.h"

#include "ws3DPreviewLightMgr.h"

#include "wsCanvas.h"

#include "wsGPUDataTools.h"

#include "iniparser\src\IniParser.hpp"
#include "TiledBitmap/TiledBitmap.h"

#include "wsTexSplattingMgr.h"

#include "wsSpringsMgr.h"

// Warning! If changing this, change the c_gridDim in ws3DPreview.vsh!
const int   c_vertexResMultiplier   = 1;

ws3DPreviewRenderer::ws3DPreviewRenderer(void)
{
   m_vsGrid.SetShaderInfo( "ws3DPreview.vsh", "main_terrain" );
   m_vsGridWater.SetShaderInfo( "ws3DPreview.vsh", "main_water" );
   m_vsGridWaterCWSOffset.SetShaderInfo( "ws3DPreview.vsh", "main_water_cwsoffset" );

   m_psTerrainFlat.SetShaderInfo( "ws3DPreviewTerrain.psh", "flatShading" );
   m_psTerrainSplatting.SetShaderInfo( "ws3DPreviewTerrain.psh", "texSplatting" );

   m_psWater.SetShaderInfo( "ws3DPreviewWater.psh", "main_water" );

   m_psWireframe.SetShaderInfo( "wsMisc.psh", "justOutputColor" );

   m_vsWriteWaterDepth.SetShaderInfo( "ws3DPreview.vsh", "outputWaterDepth" );
   m_psWriteWaterDepth.SetShaderInfo( "ws3DPreviewWater.psh", "outputWaterDepth" );

   m_vsVelocityMap.SetShaderInfo( "ws3DPreview.vsh", "velocityMap" );

   m_velocityMapUpdateBackBuffer = NULL;
   m_velocityMapFilterTemp = NULL;

   m_psVelocityMap.SetShaderInfo( "ws3DPreviewCWSSetup.psh", "velocityMap" );
   m_psVelocityMapFilter0.SetShaderInfo( "ws3DPreviewCWSSetup.psh", "velocityMapFilter0" );
   m_psVelocityMapFilter1.SetShaderInfo( "ws3DPreviewCWSSetup.psh", "velocityMapFilter1" );

   m_wireframe             = false;

   m_heightmap             = NULL;
   //m_watermap              = NULL;

   m_rendWaterHeightmap    = NULL;
   m_rendWaterInfomap      = NULL;

   m_terrainHMTexture      = NULL;
   m_terrainNMTexture      = NULL;

   m_waterHMTexture        = NULL;
   m_waterNMTexture        = NULL;
   m_waterVMTexture        = NULL;

   m_gridVertexBuffer      = NULL;
   m_gridIndexBuffer       = NULL;

   m_renderTargetDataTmp      = NULL;
   //m_depthBufferTexture       = NULL;

   m_terrainSplatMapTexture   = NULL;

   m_cameraViewRange       = 0.0f;

   // If changing this you have to change c_gridDim in ws3DPreview.vsh
   m_gridDim               = 32;
   m_quadMinSize           = m_gridDim/2;

   m_useTerrainFlatShading = false;

   //m_useSkybox             = true;

   m_lastSplashTime        = 0.0f;

   memset( &m_mapDims, 0, sizeof(m_mapDims) );

   m_detailWaves.TextureResolution = 0;
   for( int i = 0; i < m_detailWaves.TextureCount; i++ )
   {
      m_detailWaves.NMTextures[i]   = NULL;
   }

   m_mouseLeftButtonDown   = false;
   m_mouseClientX          = 0;
   m_mouseClientY          = 0;
}

ws3DPreviewRenderer::~ws3DPreviewRenderer(void)
{
   assert( m_terrainHMTexture == NULL );
   assert( m_terrainNMTexture == NULL );

   assert( m_waterHMTexture == NULL );
   assert( m_waterNMTexture == NULL );
   assert( m_waterVMTexture == NULL );
}

void ws3DPreviewRenderer::Start( AdVantage::TiledBitmap * heightmap, AdVantage::TiledBitmap * watermap, AdVantage::TiledBitmap *  rendWaterHeightmap, AdVantage::TiledBitmap * rendWaterInfomap, const MapDimensions & mapDims )
{
   assert( m_heightmap == NULL );
   //assert( m_watermap == NULL );

   m_heightmap    = heightmap;
   //m_watermap     = watermap;

   m_rendWaterHeightmap = rendWaterHeightmap;
   m_rendWaterInfomap   = rendWaterInfomap;

   m_rasterWidth  = heightmap->Width();
   m_rasterHeight = heightmap->Height();

   m_mapDims      = mapDims;

   InitializeFromIni();

   m_cascadedWaveMapSim.Start( mapDims, (IWaterSurfaceInfoProvider*) this );

   m_globalTime = 0.0;

   m_mouseLeftButtonDown = false;
}

void ws3DPreviewRenderer::InitializeFromIni( )
{
   IniParser   iniParser( wsGetIniPath() );
   m_waterIgnoreBelowDepth    = iniParser.getFloat("Exporter:waterIgnoreBelowDepth", 0.0f);
   m_waterElevateIfNotIgnored = iniParser.getFloat("Exporter:waterElevateIfNotIgnored", 0.0f);
   m_terrainWaterHeightScale  = iniParser.getFloat("Simulation:terrainHeightScale", 1.0f);
   //m_waterDropoffRateIfBelow  = iniParser.getFloat("Exporter:waterDropoffRateIfBelow", 2.0f);

   m_settings.FoamTextureUVScale             = iniParser.getFloat("Rendering:FoamTextureUVScale", m_settings.FoamTextureUVScale);
   m_settings.DetailWaveUVScale              = iniParser.getFloat("Rendering:DetailWaveUVScale", m_settings.DetailWaveUVScale);
   m_settings.DetailWaveAnimSpeed            = iniParser.getFloat("Rendering:DetailWaveAnimSpeed", m_settings.DetailWaveAnimSpeed);
   m_settings.DetailWaveStrength             = iniParser.getFloat("Rendering:DetailWaveStrength", m_settings.DetailWaveStrength);
   m_detailWaves.TextureResolution           = iniParser.getInt("Rendering:detailWaveTextureResolution", 256);
   m_settings.WaterMinVisibleLimitDepth      = iniParser.getFloat("Rendering:WaterMinVisibleLimitDepth", m_settings.WaterMinVisibleLimitDepth);
   m_settings.TestSplashRadius               = iniParser.getFloat("Rendering:TestSplashRadius", m_settings.TestSplashRadius);
   m_settings.VertexZCWSScale                = iniParser.getFloat("Rendering:VertexZCWSScale", m_mapDims.SizeZ / 128.0f );
   m_settings.VertexZCWSDropoffDepth         = iniParser.getFloat("Rendering:VertexZCWSDropoffDepth", m_mapDims.SizeZ / 100.0f );
   m_settings.VertexZCWSMaxDropoffK          = iniParser.getFloat("Rendering:VertexZCWSMaxDropoffK", m_settings.VertexZCWSMaxDropoffK );
   //m_settings.WaterZOffset                   = iniParser.getFloat("Rendering:WaterZOffset", m_settings.WaterZOffset);

   m_settings.UseProperUnderwaterRendering   = iniParser.getBool("Rendering:UseProperUnderwaterRendering", m_settings.UseProperUnderwaterRendering);
   m_settings.UseVertexWaveZOffset           = iniParser.getBool("Rendering:UseVertexWaveZOffset", m_settings.UseVertexWaveZOffset);

   float pixX = 1.0f / m_heightmap->Width() * m_mapDims.SizeX;
   float pixY = 1.0f / m_heightmap->Height() * m_mapDims.SizeY;
   float avgWorldPix = (pixX + pixY) * 0.5f;

   float tolerance = 0.15f;
   if( ( pixX > avgWorldPix * (1+tolerance) || pixX < avgWorldPix * (1-tolerance) ) || 
      ( pixY > avgWorldPix * (1+tolerance) || pixY < avgWorldPix * (1-tolerance) ) )
   {
      MessageBox( NULL, L"mapDims/heightmapDims x/y scale is not within the suggested tolerance!", L"Warning", MB_OK );
   }

   //float firstLayerWorldSize        = iniParser.getFloat("CascadedWaveSim:FirstLayerWorldSize", 0);
   float simTimeStepHz              = iniParser.getFloat("CascadedWaveSim:firstLayerSimTimeStepHz", 0);
   float waterSpeedMul              = iniParser.getFloat("CascadedWaveSim:firstLayerWaterSpeedMul", 0);

   //m_globalUVSpeedFactor = waterSpeedMul * simTimeStepHz * firstLayerWorldSize / ((m_mapDims.SizeX+m_mapDims.SizeY)*0.5f);
   m_globalUVSpeedFactor = waterSpeedMul * simTimeStepHz;
}

void ws3DPreviewRenderer::Stop( )
{
   m_cascadedWaveMapSim.Stop();

   OnLostDevice();
   OnDestroyDevice();
   m_heightmap    = NULL;
   //m_watermap     = NULL;

   m_mouseLeftButtonDown = false;
}

void ws3DPreviewRenderer::SampleAndProcessWaterTexture( float * waterheightData, const int waterheightDataPitch, unsigned __int64 * velocitymapData, int velocitymapDataPitch )
{
   //////////////////////////////////////////////////////////////////////////
   // Sample pass
   {
      float * whData = waterheightData;
      unsigned __int64 * vmData = velocitymapData;

      //float maxSpeed = 0;
      for( int y = 0; y < m_rasterHeight; y++ )
      {
         for( int x = 0; x < m_rasterWidth; x++ )
         {
            unsigned short rendWHPixel;
            unsigned int rendWIPixel;

            m_rendWaterHeightmap->GetPixel( x, y, &rendWHPixel );
            m_rendWaterInfomap->GetPixel( x, y, &rendWIPixel );

            whData[x] = (rendWHPixel / 65535.0f);

            float px = ((rendWIPixel >> 0) & 0xFF) / 255.0f;
            float vx = ((rendWIPixel >> 8) & 0xFF) / 255.0f - 0.5f - 0.0019608140f;
            float vy = ((rendWIPixel >> 16) & 0xFF) / 255.0f - 0.5f - 0.0019608140f;

            vmData[x] = (__int64)HalfFloatPack( vx ) | ((__int64)HalfFloatPack( vy ) << 16) | ((__int64)HalfFloatPack( px ) << 32);

            //{ 
            //   __int64 pixel;
            //   m_watermap->GetPixel( x, y, &pixel );
            //   float wvx = HalfFloatUnpack( (unsigned short)pixel );
            //   float wvy = HalfFloatUnpack( (unsigned short)(pixel>>16) );

            //   assert( fabsf( wvx - vx ) < 0.004f );
            //   assert( fabsf( wvy - vy ) < 0.004f );
            //}
         }
         whData += waterheightDataPitch / sizeof(*whData);
         vmData += velocitymapDataPitch / sizeof(*vmData);
      }
   }
   //////////////////////////////////////////////////////////////////////////

}

void ws3DPreviewRenderer::InitializeTextures()
{
   IDirect3DDevice9 * device = GetD3DDevice();

   HRESULT hr;

   // If not loaded, load heightmap texture
   if( m_terrainHMTexture == NULL )
   {
      assert( m_rasterWidth   == m_heightmap->Width() );
      assert( m_rasterHeight  == m_heightmap->Height() );
      assert( m_terrainNMTexture == NULL );

      // these should probably have autogenerate mipmaps but then we also need mip level selection code in vertex shader....
      V( device->CreateTexture( m_rasterWidth, m_rasterHeight, 1, 0, D3DFMT_R32F, D3DPOOL_MANAGED, &m_terrainHMTexture, NULL ) );
      
      //Don't create this here, we'll calculate normal map on the GPU
      //V( device->CreateTexture( m_rasterWidth, m_rasterHeight, 1, 0, D3DFMT_G16R16F, D3DPOOL_MANAGED, &m_terrainNMTexture, NULL ) );

      //////////////////////////////////////////////////////////////////////////
      // Load heightmap
      //
      D3DLOCKED_RECT heightmapLockedRect; 
      V( m_terrainHMTexture->LockRect( 0, &heightmapLockedRect, NULL, 0 ) );
      
      float * heightmapData = (float *)heightmapLockedRect.pBits;

      for( int y = 0; y < m_rasterHeight; y++ )
      {
         for( int x = 0; x < m_rasterWidth; x++ )
         {
            unsigned short pixel;
            m_heightmap->GetPixel( x, y, &pixel );
            heightmapData[x] = pixel / 65535.0f;
         }
         heightmapData += heightmapLockedRect.Pitch / sizeof(*heightmapData);
      }

      //if( m_terrainNMTexture != NULL )
      //{
      //   D3DLOCKED_RECT normalmapLockedRect;
      //   V( m_terrainNMTexture->LockRect( 0, &normalmapLockedRect, NULL, 0 ) );
      //   CreateNormalMap( m_rasterWidth, m_rasterHeight, m_mapDims, (float *)heightmapLockedRect.pBits, heightmapLockedRect.Pitch, (unsigned int *)normalmapLockedRect.pBits, normalmapLockedRect.Pitch );
      //   m_terrainNMTexture->UnlockRect( 0 );
      //}

      m_terrainQuadTree.Create( m_mapDims, m_rasterWidth, m_rasterHeight, (float *)heightmapLockedRect.pBits, heightmapLockedRect.Pitch, m_quadMinSize );

      //heightmapLockedRect->pBits
      m_terrainHMTexture->UnlockRect( 0 );

      //////////////////////////////////////////////////////////////////////////
   }

   // If not loaded, load watermap texture
   if( m_waterHMTexture == NULL )
   {
      assert( m_rasterWidth   == m_heightmap->Width() );
      assert( m_rasterHeight  == m_heightmap->Height() );
      assert( m_waterNMTexture == NULL );

      // these should have autogenerate mipmaps but then I also need mip level selection code in the vertex shader....
      V( device->CreateTexture( m_rasterWidth, m_rasterHeight, 1, 0, D3DFMT_R32F, D3DPOOL_MANAGED, &m_waterHMTexture, NULL ) );
      
      //Don't create this here, we'll calculate normal map on the GPU
      //V( device->CreateTexture( m_rasterWidth, m_rasterHeight, 1, 0, D3DFMT_G16R16F, D3DPOOL_MANAGED, &m_waterNMTexture, NULL ) );
      
      //V( device->CreateTexture( m_rasterWidth, m_rasterHeight, 1, 0, D3DFMT_G16R16F, D3DPOOL_MANAGED, &m_waterVMTexture, NULL ));
      V( device->CreateTexture( m_rasterWidth, m_rasterHeight, 1, 0, D3DFMT_A16B16G16R16F, D3DPOOL_MANAGED, &m_waterVMTexture, NULL ));

      //////////////////////////////////////////////////////////////////////////
      // Load heightmap
      //
      D3DLOCKED_RECT heightmapLockedRect, velocitymapLockedRect;
      V( m_waterHMTexture->LockRect( 0, &heightmapLockedRect, NULL, 0 ) );
      V( m_waterVMTexture->LockRect( 0, &velocitymapLockedRect, NULL, 0 ) );

      SampleAndProcessWaterTexture( (float *)heightmapLockedRect.pBits, heightmapLockedRect.Pitch, (unsigned __int64 *)velocitymapLockedRect.pBits, velocitymapLockedRect.Pitch );

      //if( m_waterNMTexture != NULL )
      //{
      //   D3DLOCKED_RECT normalmapLockedRect;
      //   V( m_waterNMTexture->LockRect( 0, &normalmapLockedRect, NULL, 0 ) );
      //   CreateNormalMap( m_rasterWidth, m_rasterHeight, m_mapDims, (float *)heightmapLockedRect.pBits, heightmapLockedRect.Pitch, (unsigned int *)normalmapLockedRect.pBits, normalmapLockedRect.Pitch );
      //   m_waterNMTexture->UnlockRect( 0 );
      //}

      m_waterQuadTree.Create( m_mapDims, m_rasterWidth, m_rasterHeight, (float *)heightmapLockedRect.pBits, heightmapLockedRect.Pitch, m_quadMinSize );

      //heightmapLockedRect->pBits
      m_waterHMTexture->UnlockRect( 0 );
      m_waterVMTexture->UnlockRect( 0 );
      //////////////////////////////////////////////////////////////////////////
   }

   if( m_terrainNMTexture == NULL || m_waterNMTexture == NULL || m_terrainSplatMapTexture == NULL )
   {
      wsGPUToolsContext gpuTools;

      if( m_terrainNMTexture == NULL )
      {
         V( device->CreateTexture( m_rasterWidth, m_rasterHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G16R16F, D3DPOOL_DEFAULT, &m_terrainNMTexture, NULL ) );
         V( gpuTools.CalculateNormalMap( m_terrainHMTexture, m_terrainNMTexture, m_mapDims.SizeX, m_mapDims.SizeY, m_mapDims.SizeZ ) );
      }

      if( m_waterNMTexture == NULL )
      {
         V( device->CreateTexture( m_rasterWidth, m_rasterHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G16R16F, D3DPOOL_DEFAULT, &m_waterNMTexture, NULL ) );
         V( gpuTools.CalculateNormalMap( m_waterHMTexture, m_waterNMTexture, m_mapDims.SizeX, m_mapDims.SizeY, m_mapDims.SizeZ ) );
      }

      if( m_terrainSplatMapTexture == NULL )
      {
         V( device->CreateTexture( m_rasterWidth, m_rasterHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_terrainSplatMapTexture, NULL ) );
         V( gpuTools.CalculateSplatMap( m_terrainHMTexture, m_terrainNMTexture, m_waterHMTexture, m_waterVMTexture, m_terrainSplatMapTexture ) );
      }

      if( m_detailWaves.NMTextures[0] == NULL )
      {
         int wRes = m_detailWaves.TextureResolution;

         IDirect3DTexture9 * waveHeightmapTemp;
         V( device->CreateTexture( wRes, wRes, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G16R16F, D3DPOOL_DEFAULT, &waveHeightmapTemp, NULL ) );
         // we could use D3DFMT_R16F if all GPUs supported it...

         for( int i = 0; i < m_detailWaves.TextureCount; i++ )
         {
            gpuTools.CalculateDetailWaveMap( waveHeightmapTemp, i / (float)(m_detailWaves.TextureCount) );

            V( device->CreateTexture( wRes, wRes, 0, D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_detailWaves.NMTextures[i], NULL ) );
            m_detailWaves.NMTextures[i]->SetAutoGenFilterType(D3DTEXF_LINEAR); // D3DTEXF_POINT?

            //gpuTools.CalculateNormalMap( m_detailWaves.Textures[i], m_detailWaves.NMTextures[i], (float)wRes, (float)wRes, 64.0f, true, true );
            gpuTools.CalculateNormalMap( waveHeightmapTemp, m_detailWaves.NMTextures[i], (float)wRes, (float)wRes, 32.0f, true, true );
         }

         SAFE_RELEASE( waveHeightmapTemp );
      }
   }

   if( m_renderTargetDataTmp == NULL )
   {
      V( GetD3DDevice()->CreateTexture( m_backBufferSurfaceDesc.Width, m_backBufferSurfaceDesc.Height, 1, D3DUSAGE_RENDERTARGET, m_backBufferSurfaceDesc.Format, D3DPOOL_DEFAULT, &m_renderTargetDataTmp, NULL ) );
      //V( GetD3DDevice()->CreateTexture( m_backBufferSurfaceDesc.Width, m_backBufferSurfaceDesc.Height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G16R16F, D3DPOOL_DEFAULT, &m_depthBufferTexture, NULL ) );
   }

}

void ws3DPreviewRenderer::CreateBlockMesh( )
{
   // This mesh is used to render terrain and water. It contains x and y, while z will be sampled in the vertex shader

   HRESULT hr;
   IDirect3DDevice9 * pDevice = GetD3DDevice();

   assert( m_gridIndexBuffer == NULL );
   assert( m_gridVertexBuffer == NULL );

   int gridDim = m_gridDim * c_vertexResMultiplier;

   int totalVertices = (gridDim+1) * (gridDim+1);
   assert( totalVertices <= 65535 );
   V( pDevice->CreateVertexBuffer( sizeof(PositionVertex) * totalVertices, 0, PositionVertex::FVF, D3DPOOL_MANAGED, &m_gridVertexBuffer, NULL ) );

   int totalIndices = gridDim * gridDim * 2 * 3;
   V( pDevice->CreateIndexBuffer( sizeof(unsigned short) * totalIndices, 0, D3DFMT_INDEX16, D3DPOOL_MANAGED, &m_gridIndexBuffer, NULL ) );

   int vertDim = gridDim + 1;

   {
      // Make a grid of (gridDim+1) * (gridDim+1) vertices

      PositionVertex * vertices = NULL;
      V( m_gridVertexBuffer->Lock(0, sizeof(PositionVertex) * totalVertices, (void**)&vertices, 0 ) );

      for( int y = 0; y < vertDim; y++ )
         for( int x = 0; x < vertDim; x++ )
            vertices[x + vertDim * y] = PositionVertex( x / (float)(gridDim), y / (float)(gridDim), 0 );

      V( m_gridVertexBuffer->Unlock() );
   }

   {
      // Make indices for the gridDim * gridDim triangle grid, but make it as a combination of 4 subquads so that they
      // can be rendered separately when needed!

      unsigned short * indices = NULL;
      V( m_gridIndexBuffer->Lock(0, sizeof(unsigned short) * totalIndices, (void**)&indices, 0 ) );

      int index = 0;

      int halfd = (vertDim/2);
      int fulld = gridDim;

      // Top left part
      for( int y = 0; y < halfd; y++ )
      {
         for( int x = 0; x < halfd; x++ )
         {
            indices[index++] = (unsigned short)(x + vertDim * y);         indices[index++] = (unsigned short)((x+1) + vertDim * y);     indices[index++] = (unsigned short)(x + vertDim * (y+1));
            indices[index++] = (unsigned short)((x+1) + vertDim * y);     indices[index++] = (unsigned short)((x+1) + vertDim * (y+1)); indices[index++] = (unsigned short)(x + vertDim * (y+1));
         }
      }
      m_gridIndexEndTL = index;

      // Top right part
      for( int y = 0; y < halfd; y++ )
      {
         for( int x = halfd; x < fulld; x++ )
         {
            indices[index++] = (unsigned short)(x + vertDim * y);         indices[index++] = (unsigned short)((x+1) + vertDim * y);     indices[index++] = (unsigned short)(x + vertDim * (y+1));
            indices[index++] = (unsigned short)((x+1) + vertDim * y);     indices[index++] = (unsigned short)((x+1) + vertDim * (y+1)); indices[index++] = (unsigned short)(x + vertDim * (y+1));
         }
      }
      m_gridIndexEndTR = index;

      // Bottom left part
      for( int y = halfd; y < fulld; y++ )
      {
         for( int x = 0; x < halfd; x++ )
         {
            indices[index++] = (unsigned short)(x + vertDim * y);         indices[index++] = (unsigned short)((x+1) + vertDim * y);     indices[index++] = (unsigned short)(x + vertDim * (y+1));
            indices[index++] = (unsigned short)((x+1) + vertDim * y);     indices[index++] = (unsigned short)((x+1) + vertDim * (y+1)); indices[index++] = (unsigned short)(x + vertDim * (y+1));
         }
      }
      m_gridIndexEndBL = index;

      // Bottom right part
      for( int y = halfd; y < fulld; y++ )
      {
         for( int x = halfd; x < fulld; x++ )
         {
            indices[index++] = (unsigned short)(x + vertDim * y);         indices[index++] = (unsigned short)((x+1) + vertDim * y);     indices[index++] = (unsigned short)(x + vertDim * (y+1));
            indices[index++] = (unsigned short)((x+1) + vertDim * y);     indices[index++] = (unsigned short)((x+1) + vertDim * (y+1)); indices[index++] = (unsigned short)(x + vertDim * (y+1));
         }
      }
      m_gridIndexEndBR = index;

      V( m_gridIndexBuffer->Unlock() );
   }

}

void ws3DPreviewRenderer::ForceCreateDevice()
{
   HRESULT hr;

   V( OnCreateDevice() );
   V( m_cascadedWaveMapSim.OnCreateDevice() );
}

void ws3DPreviewRenderer::ForceResetDevice( const D3DSURFACE_DESC * pBackBufferSurfaceDesc )
{
   OnResetDevice( pBackBufferSurfaceDesc );
   m_cascadedWaveMapSim.OnResetDevice( pBackBufferSurfaceDesc );
}

HRESULT ws3DPreviewRenderer::OnCreateDevice( )
{
   HRESULT hr;
   IDirect3DDevice9 * device = GetD3DDevice();

   //V( m_skyBox.OnCreateDevice() );

   CreateBlockMesh();

   V( D3DXCreateTextureFromFile( device, wsFindResource(L"Media\\foam.dds").c_str(), &m_foamMapTexture ) );

   //V( D3DXCreateTextureFromFile( device, wsFindResource(L"Media\\noise2.dds").c_str(), &m_terrainSplatMapTexture ) );

   //V( GetD3DDevice()->CreateTexture( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height, 1, 0, D3DUSAGE_RENDERTARGET, pBackBufferSurfaceDesc->Format, D3DPOOL_DEFAULT, &m_renderTargetDataTmp, NULL ) );

   return S_OK;
}

HRESULT ws3DPreviewRenderer::OnResetDevice( const D3DSURFACE_DESC * pBackBufferSurfaceDesc )
{
   //HRESULT hr;

   m_backBufferSurfaceDesc = *pBackBufferSurfaceDesc;

   //IDirect3DSurface9 * depthStencilSurface;
   //V( GetD3DDevice()->GetDepthStencilSurface( &depthStencilSurface ) );
   //V( depthStencilSurface->GetDesc( &m_depthBufferSurfaceDesc ) );
   //V( depthStencilSurface->Release() );

   return S_OK;
}

void ws3DPreviewRenderer::OnLostDevice( )
{
   SAFE_RELEASE( m_terrainNMTexture );
   SAFE_RELEASE( m_waterNMTexture );

   SAFE_RELEASE( m_velocityMapUpdateBackBuffer );
   SAFE_RELEASE( m_velocityMapFilterTemp );
   SAFE_RELEASE( m_renderTargetDataTmp );
   //SAFE_RELEASE( m_depthBufferTexture );

   for( int i = 0; i < m_detailWaves.TextureCount; i++ )
   {
      SAFE_RELEASE( m_detailWaves.NMTextures[i] );
   }

   SAFE_RELEASE( m_terrainSplatMapTexture );
}

void ws3DPreviewRenderer::OnDestroyDevice( )
{
   SAFE_RELEASE( m_terrainHMTexture );
   SAFE_RELEASE( m_terrainNMTexture );

   SAFE_RELEASE( m_waterHMTexture );
   SAFE_RELEASE( m_waterVMTexture );

   //SAFE_RELEASE( m_terrainSplatMapTexture );

   SAFE_RELEASE( m_gridVertexBuffer );
   SAFE_RELEASE( m_gridIndexBuffer );

   SAFE_RELEASE( m_foamMapTexture );

   //m_skyBox.OnDestroyDevice();
}

void ws3DPreviewRenderer::Tick( ws3DPreviewCamera * camera, float deltaTime )
{
   if( IsKeyClicked(kcReloadAll) )
   {
      InitializeFromIni();

      OnDestroyDevice();
      OnLostDevice();
      OnCreateDevice();
   }

   InitializeTextures();

   //m_terrainQuadTree.DebugDrawAllNodes();
   m_globalTime += deltaTime;

   m_cameraViewRange = camera->GetViewRange();

   //////////////////////////////////////////////////////////////////////////
   // Update wavemap simulator
   m_cascadedWaveMapSim.Tick( deltaTime, camera );
   //////////////////////////////////////////////////////////////////////////

   //////////////////////////////////////////////////////////////////////////
   // Do splash
   //if( IsKeyClicked( kcDoSimpleSplash ) )
   
   if( m_mouseLeftButtonDown )
   {
      const float minTimeBetweenSplashes = 0.04f;
      if( m_lastSplashTime + minTimeBetweenSplashes < m_globalTime )
      {
         D3DXVECTOR3 mousePosN( (float)m_mouseClientX, (float)m_mouseClientY, 0 );
         D3DXVECTOR3 mousePosF( (float)m_mouseClientX, (float)m_mouseClientY, 1 );

         D3DXVECTOR3 camPos;// = camera->GetPosition();
         D3DXVECTOR3 camDir;// = camera->GetDirection();

         D3DVIEWPORT9 viewport;
         GetD3DDevice()->GetViewport( &viewport );

         D3DXMATRIX world; D3DXMatrixIdentity( &world );
         D3DXMATRIX view   = *camera->GetViewMatrix();
         D3DXMATRIX proj   = *camera->GetProjMatrix();

         D3DXVec3Unproject( &camPos, &mousePosN, &viewport, &proj, &view, &world );
         D3DXVec3Unproject( &camDir, &mousePosF, &viewport, &proj, &view, &world );
         camDir -= camPos;
         D3DXVec3Normalize( &camDir, &camDir );

         m_lastSplashTime = (float)m_globalTime;

         float maxDist = camera->GetViewRange();
         D3DXVECTOR3 hitPos;
         if( m_waterQuadTree.IntersectRay( camPos, camDir, maxDist, hitPos ) )
         {
            m_cascadedWaveMapSim.DoTestSplash( hitPos, m_settings.TestSplashRadius, 0.0f );
         }
      }
   }
   //////////////////////////////////////////////////////////////////////////


   if( IsKeyClicked( kcToggleWireframe ) )
      m_wireframe = !m_wireframe;
   //
   if( IsKeyClicked( kcToggleSimpleRender ) )
      m_useTerrainFlatShading = !m_useTerrainFlatShading;
   //
}

void ws3DPreviewRenderer::RenderNode( const wsQuadTree::NodeSelection & nodeSel, wsQuadTree::LODSelectionInfo & selInfo, ID3DXConstantTable * vsConsts, ID3DXConstantTable * psConsts )
{
   IDirect3DDevice9* device = GetD3DDevice();
   HRESULT hr;

   //GetCanvas3D()->DrawBox( D3DXVECTOR3(worldX, worldY, m_mapDims.MinZ), D3DXVECTOR3(worldX+worldSizeX, worldY+worldSizeY, m_mapDims.MaxZ()), 0xFF0000FF, 0x00FF0080 );
   bool drawFull = nodeSel.TL && nodeSel.TR && nodeSel.BL && nodeSel.BR;

   //if( psConsts != NULL ) // hack-detect wireframe
   //   nodeSel.pNode->DebugDrawAABB( drawFull?0xFF0000FF:0x8000FF00 );

   float v[4];

   v[0] = (nodeSel.pNode->WorldMaxX-nodeSel.pNode->WorldMinX); v[1] = (nodeSel.pNode->WorldMaxY-nodeSel.pNode->WorldMinY); v[2] = 0.0f; v[3] = 0.0f;
   V( vsConsts->SetFloatArray( device, "g_gridScale", v, 4 ) );

   v[0] = nodeSel.pNode->WorldMinX; v[1] = nodeSel.pNode->WorldMinY; v[2] = (nodeSel.pNode->WorldMinZ + nodeSel.pNode->WorldMaxZ) * 0.5f; v[3] = 0.0f;
   V( vsConsts->SetFloatArray( device, "g_gridOffset", v, 4 ) );

   v[0] = m_mapDims.MaxX();   v[1] = m_mapDims.MaxY();
   V( vsConsts->SetFloatArray( device, "g_gridWorldMax", v, 2 ) );

   selInfo.GetMorphConsts(&nodeSel, v);
   vsConsts->SetFloatArray( device, "g_morphConsts", v, 4 );

   int gridDim = m_gridDim * c_vertexResMultiplier;

   int totalVertices = (gridDim+1) * (gridDim+1);
   int totalIndices = gridDim * gridDim * 2 * 3;
   if( drawFull )
   {
      V( device->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, totalVertices, 0, totalIndices/3 ) );
   }
   else
   {
      int halfd = ((gridDim+1)/2) * ((gridDim+1)/2) * 2;

      // can be optimized by combining calls
      if( nodeSel.TL )
         V( device->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, totalVertices, 0, halfd ) );
      if( nodeSel.TR ) 
         V( device->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, totalVertices, m_gridIndexEndTL, halfd ) );
      if( nodeSel.BL ) 
         V( device->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, totalVertices, m_gridIndexEndTR, halfd ) );
      if( nodeSel.BR ) 
         V( device->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, totalVertices, m_gridIndexEndBL, halfd ) );
   }
  
}

void ws3DPreviewRenderer::SetupGlobalVSConsts( ID3DXConstantTable * consts, const D3DXMATRIX & viewProj, const D3DXVECTOR3 & camPos, ws3DPreviewLightMgr * lightMgr, int textureWidth, int textureHeight )
{
   HRESULT hr;
   IDirect3DDevice9* device = GetD3DDevice();
   //float fogStart	= pCamera->GetViewRange() * 0.75f;
   //float fogEnd	= pCamera->GetViewRange() * 0.98f;
   //D3DXVECTOR4 fogConsts( fogEnd / ( fogEnd - fogStart ), -1.0f / ( fogEnd - fogStart ), 0, 1.0f/fogEnd );

   V( consts->SetMatrix( device, "g_viewProjection", &viewProj ) );

   if( lightMgr != NULL )
   {
      D3DXVECTOR4 diffLightDir = D3DXVECTOR4( lightMgr->GetDirectionalLightDir(), 1.0f );
      V( consts->SetVector( device, "g_diffuseLightDir", &diffLightDir ) );
   }

   D3DXVECTOR4 camPos4 = D3DXVECTOR4( camPos, 1.0f );

   consts->SetVector( device, "g_cameraPos", &camPos4 );
   //V( consts->SetVector( pDevice, "g_fogConsts", &fogConsts ) );

   float v[4];

   v[0] = m_mapDims.SizeX; v[1] = m_mapDims.SizeY; v[2] = m_mapDims.SizeZ; v[3] = 0.0f;
   V( consts->SetFloatArray( device, "g_terrainScale", v, 4 ) );

   v[0] = m_mapDims.MinX; v[1] = m_mapDims.MinY; v[2] = m_mapDims.MinZ; v[3] = 0.0f;
   V( consts->SetFloatArray( device, "g_terrainOffset", v, 4 ) );

   v[0] = (textureWidth-1.0f) / (float)textureWidth;    v[1] = (textureHeight-1.0f) / (float)textureHeight;
   consts->SetFloatArray( device, "g_samplerWorldToTextureScale", v, 2 );

   v[0] = 1.0f / (float)textureWidth;    v[1] = 1.0f / (float)textureHeight;
   consts->SetFloatArray( device, "g_samplerPixelSize", v, 2 );

}

void ws3DPreviewRenderer::RenderNodes( const wsQuadTree::NodeSelection * selectionArray, int selectionCount, wsQuadTree::LODSelectionInfo & selInfo, ID3DXConstantTable * vsConsts, ID3DXConstantTable * psConsts )
{
   for( int i = 0; i < selectionCount; i++ )
   {
      RenderNode( selectionArray[i], selInfo, vsConsts, psConsts );
   }
}

double frac( double a )
{
   return fmod( a, 1.0 );
}

static void wsCalculateFakeTriFlowConsts( double currentTime, float speedMod, float outDeltas[3], float outAlphas[3], float rateOfSwap )
{
   float nk = 3.0f;
   float nkk = 0.5f / nk;

   //float rateOfSwap = 1.0f;

   currentTime *= rateOfSwap;
   speedMod /= rateOfSwap;

   float t1 = (float)frac( currentTime );
   float a1 = 1 - saturate( (nkk - t1) / nkk ) - saturate( (t1 - (1-nkk) ) / nkk );
   float t2 = (float)frac( (currentTime) + 1.0f / nk );
   float a2 = 1 - saturate( (nkk - t2) / nkk ) - saturate( (t2 - (1-nkk) ) / nkk );
   float t3 = (float)frac( (currentTime) + 2.0f / nk );
   float a3 = 1 - saturate( (nkk - t3) / nkk ) - saturate( (t3 - (1-nkk) ) / nkk );

   outDeltas[0] = (t1-0.5f) * speedMod;   
   outDeltas[1] = (t2-0.5f) * speedMod;   
   outDeltas[2] = (t3-0.5f) * speedMod;
   outAlphas[0] = a1;   
   outAlphas[1] = a2;   
   outAlphas[2] = a3;

   float alphaTot = a1+a2+a3;

   outAlphas[0] /= alphaTot;
   outAlphas[1] /= alphaTot;
   outAlphas[2] /= alphaTot;
}

static float wsGetSpeedFactor( float globalUVSpeedFactor, IDirect3DTexture9 * texture, float uvMul )
{
   D3DSURFACE_DESC surfDesc;
   texture->GetLevelDesc( 0, &surfDesc );

   assert( surfDesc.Width == surfDesc.Height );

   return globalUVSpeedFactor * uvMul;
}


void ws3DPreviewRenderer::RenderNodesWithCWM( const wsQuadTree::NodeSelection * selectionArray, int selectionCount, wsQuadTree::LODSelectionInfo & selInfo, ws3DPreviewCamera * camera, ID3DXConstantTable * vsConsts, ID3DXConstantTable * psConsts )
{
   HRESULT hr;
   IDirect3DDevice9* device = GetD3DDevice();
   
   int waveMapSimRes = m_cascadedWaveMapSim.GetTextureResolution();

   if( psConsts != NULL )
   {
      float v[2];
      v[0] = 1.0f / (float)waveMapSimRes; v[1] = 1.0f / (float)waveMapSimRes;
      psConsts->SetFloatArray( device, "g_heightMapPixelSize", v, 2 );
      psConsts->SetFloat( device, "g_globalTime", (float)m_globalTime );
      //psConsts->SetVector( device, "g_waterDiffuseColor", D3DXVECTOR4(  ) );
   }

   //const float detailMapUVMultBase = 512.0f; 

   const int layerCount = m_cascadedWaveMapSim.GetLayerCount();

   float layerDeltaScale = m_cascadedWaveMapSim.GetLayerDeltaScale();

   //////////////////////////////////////////////////////////////////////////
   // ** DEBUG STUFF **
   static int ignoreLayerID = -1;
   const float debugColorAlpha = 0.1f;
   D3DXVECTOR4 diagColors[6] = { D3DXVECTOR4( 1.0f, 0.0f, 0.0f, debugColorAlpha ), 
                                 D3DXVECTOR4( 1.0f, 1.0f, 0.0f, debugColorAlpha ),
                                 D3DXVECTOR4( 0.0f, 1.0f, 0.0f, debugColorAlpha ),
                                 D3DXVECTOR4( 0.0f, 1.0f, 1.0f, debugColorAlpha ),
                                 D3DXVECTOR4( 1.0f, 0.0f, 1.0f, debugColorAlpha ),
                                 D3DXVECTOR4( 0.0f, 0.0f, 1.0f, debugColorAlpha )
                                 };
   //////////////////////////////////////////////////////////////////////////

   for( int i = layerCount-1; i >= 0; i-- )
   {
      //////////////////////////////////////////////////////////////////////////
      // ** DEBUG STUFF **
      if( ignoreLayerID != -1 )
         if( ignoreLayerID != i )
            continue;
      if( psConsts != NULL )
         psConsts->SetVector( device, "g_debugColor", &diagColors[i] );
      //////////////////////////////////////////////////////////////////////////

      D3DXMATRIX viewProj;
      // offset every layer a bit below the higher detailed one so that it doesn't protrude due to CWSVertexHeightOffset
      // also, offset all layers a bit above terrain to eliminate z-fighting in the distance
      {
         float terrZModMul = 1.005f;
         float terrZModAdd = 0.01f;

         float layerZModMul = 0.015f;
         float layerZModAdd = 0.002f;

         float finalZModMul = terrZModMul + layerZModMul * (layerCount - i - 1);
         float finalZModAdd = terrZModAdd + layerZModAdd * (layerCount - i - 1);

         const D3DXMATRIX & view = *camera->GetViewMatrix();
         D3DXMATRIX proj;
         camera->GetZOffsettedProjMatrix(proj, finalZModMul, finalZModAdd);
         viewProj = view * proj;
         V( vsConsts->SetMatrix( device, "g_viewProjection", &viewProj ) );
      }

      wsCascadedWaveMapSim::WaveMapLayerInfo layerInfo;
      m_cascadedWaveMapSim.GetLayerInfo( i, layerInfo );
      if( layerInfo.OutOfRange )
         continue;

      float drawFrom = -1.0;
      if( i != 0 )
      {
         wsCascadedWaveMapSim::WaveMapLayerInfo lowerLayerInfo;
         m_cascadedWaveMapSim.GetLayerInfo( i-1, lowerLayerInfo );
         drawFrom = lowerLayerInfo.FadeOutFrom * 0.95f;
      }

      // Vertex shader consts
      D3DXVECTOR3 aabbSize = layerInfo.Aabb.Size();
      D3DXVECTOR4 waterTexConsts( layerInfo.Aabb.Min.x, layerInfo.Aabb.Min.y, aabbSize.x, aabbSize.y );
      vsConsts->SetVector( device, "g_waterTexConsts", &waterTexConsts );
      D3DXVECTOR4 fadeConsts( layerInfo.FadeOutTo / ( layerInfo.FadeOutTo - layerInfo.FadeOutFrom ), -1.0f / ( layerInfo.FadeOutTo - layerInfo.FadeOutFrom ), drawFrom, 0.0f );
      vsConsts->SetVector( device, "g_fadeConsts", &fadeConsts );

      // Setup CWSVertexHeightOffset stuff
      {
         float levelMul = powf( layerDeltaScale * 0.8f, (float)i );

         vsConsts->SetFloat( device, "g_CWSVertexHeightOffsetAverage", layerInfo.CombinedHeightMapAverage );
         vsConsts->SetFloat( device, "g_CWSVertexHeightOffsetScale", (m_settings.VertexZCWSScale) * levelMul );
         vsConsts->SetFloat( device, "g_CWSVertexDropoffK", m_settings.VertexZCWSMaxDropoffK );
         vsConsts->SetFloat( device, "g_CWSVertexDropoffDepth", m_settings.VertexZCWSDropoffDepth );

         wsSetVertexTexture( layerInfo.CombinedHeightMap, "g_CWSCombinedHeightMapTexture", vsConsts );
         wsSetVertexTexture( m_terrainHMTexture, "g_surfaceTerrainHMVertexTexture", vsConsts );
         vsConsts->SetFloat( device, "g_CWSTimeLerpK", layerInfo.TimeLerpK );
         vsConsts->SetFloat( device, "g_waterMinVisibleLimitDepth", m_settings.WaterMinVisibleLimitDepth );
         //vsConsts->SetFloat( device, "g_waterZOffset", m_settings.WaterZOffset );
      }

      // Pixel shader consts
      if( psConsts != NULL )
      {
         D3DXMATRIX view = *camera->GetViewMatrix();
         D3DXMATRIX proj = *camera->GetProjMatrix();
         //view._41 = 0.0; view._42 = 0.0; view._43 = 0.0;
         viewProj = view * proj;

         psConsts->SetMatrix( device, "g_viewProjection", &viewProj );
         psConsts->SetMatrix( device, "g_view", &view );
         psConsts->SetMatrix( device, "g_proj", &proj );
         //psConsts->SetFloat( device, "g_detailMapUVMult", detailMapUVMultBase / layerInfo.DetailUVDiv );
         psConsts->SetFloat( device, "g_timeLerpK", layerInfo.TimeLerpK );
         psConsts->SetFloat( device, "g_waterMinVisibleLimitDepth", m_settings.WaterMinVisibleLimitDepth );

         wsSetTexture( layerInfo.WaterVelocityMap, "g_waterVelocityTexture", psConsts, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
         wsSetTexture( layerInfo.CombinedHeightMap, "g_combinedHeightMapTexture", psConsts, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
         wsSetTexture( layerInfo.CombinedHNormMap, "g_combinedHNormalMapTexture", psConsts, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
         wsSetTexture( layerInfo.CombinedFoamMap, "g_combinedFoamMapTexture", psConsts, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
         wsSetTexture( m_foamMapTexture, "g_foamMapTexture", psConsts, D3DTADDRESS_WRAP, D3DTADDRESS_WRAP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );

         float levelDetailMapUVScale = powf( layerDeltaScale, (float)i * 0.7f );
         float trueLevelScaleDiff = powf( layerDeltaScale, (float)i );

         // Foam
         {
            float deltas[3];
            float alphas[3];

            const float foamUVScale = m_settings.FoamTextureUVScale * levelDetailMapUVScale; //512.0f;
            psConsts->SetFloat( device, "g_foamMapUVScale", foamUVScale );

            float speedFactor = wsGetSpeedFactor( m_globalUVSpeedFactor / trueLevelScaleDiff, m_foamMapTexture, foamUVScale );
            
            wsCalculateFakeTriFlowConsts( m_globalTime, speedFactor, deltas, alphas, 1.0f );

            psConsts->SetFloatArray( device, "g_foamFakeFlowTime", deltas, 3 );
            psConsts->SetFloatArray( device, "g_foamFakeFlowAlpha", alphas, 3 );
         }

         // DetailWave
         {
            // hack the detailWaveUVScale so that higher levels have lower DetailWaveUVScale, but then also slow them down accordingly
            //float layerHack = powf( layerDeltaScale * 0.8f, (float)i );
            const float detailWaveUVScale = m_settings.DetailWaveUVScale * levelDetailMapUVScale;
            const float waveAnimSpeedMod = m_settings.DetailWaveAnimSpeed * levelDetailMapUVScale;

            float deltas[3];
            float alphas[3];

            psConsts->SetFloat( device, "g_detailWaveUVScale", detailWaveUVScale );
            psConsts->SetFloat( device, "g_detailWaveStrength", m_settings.DetailWaveStrength );

            float speedFactor = wsGetSpeedFactor( m_globalUVSpeedFactor / trueLevelScaleDiff, m_detailWaves.NMTextures[0], detailWaveUVScale );
            wsCalculateFakeTriFlowConsts( m_globalTime, speedFactor, deltas, alphas, 0.4f );

            psConsts->SetFloatArray( device, "g_detailWaveFakeFlowTime", deltas, 3 );
            psConsts->SetFloatArray( device, "g_detailWaveFakeFlowAlpha", alphas, 3 );

            float currentTime = (float)fmod( m_globalTime * waveAnimSpeedMod, 1.0 );
            float detailWaveLerpK = fmodf( currentTime * m_detailWaves.TextureCount, 1.0f );
            int frameA = (int)(currentTime * m_detailWaves.TextureCount); 
            int frameB = (int)(currentTime * m_detailWaves.TextureCount + 1) % m_detailWaves.TextureCount;
            psConsts->SetFloat( device, "g_detailWaveLerpK", detailWaveLerpK );
            wsSetTexture( m_detailWaves.NMTextures[frameA], "g_detailWaveA", psConsts, D3DTADDRESS_WRAP, D3DTADDRESS_WRAP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
            wsSetTexture( m_detailWaves.NMTextures[frameB], "g_detailWaveB", psConsts, D3DTADDRESS_WRAP, D3DTADDRESS_WRAP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
         }
      }

      for( int i = 0; i < selectionCount; i++ )
      {
         const wsQuadTree::Node & node = *selectionArray[i].pNode;
         wsAABB naabb( D3DXVECTOR3(node.WorldMinX, node.WorldMinY, node.WorldMinZ), D3DXVECTOR3(node.WorldMaxX, node.WorldMaxY, node.WorldMaxZ) );

         if( !naabb.Intersects( layerInfo.Aabb ) )
            continue;

         // if too close for the layer to be visible (higher detailed layer covers the area completely) then don't draw it
         float maxDistanceFromCamera = sqrtf( naabb.MaxDistanceFromPointSq( camera->GetPosition() ) );
         if( maxDistanceFromCamera < drawFrom )
            continue;

         {
            RenderNode( selectionArray[i], selInfo, vsConsts, psConsts );
         }
      }
   }

   if( psConsts )
   {
      // De-select textures that we render to later to prevent DX debug runtime warnings...
      wsSetTexture( NULL, "g_waterVelocityTexture", psConsts, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
      wsSetTexture( NULL, "g_combinedHeightMapTexture", psConsts, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
      wsSetTexture( NULL, "g_combinedFoamMapTexture", psConsts, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
      wsSetTexture( NULL, "g_waveDetailMapTexture", psConsts, D3DTADDRESS_WRAP, D3DTADDRESS_WRAP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
      wsSetTexture( NULL, "g_combinedHNormalMapTexture", psConsts );
   }
   wsSetVertexTexture( NULL, "g_CWSCombinedHeightMapTexture", vsConsts );
   wsSetVertexTexture( NULL, "g_surfaceTerrainHMVertexTexture", vsConsts );
}

HRESULT ws3DPreviewRenderer::Render( ws3DPreviewCamera * camera, ws3DPreviewLightMgr * lightMgr, float deltaTime )
{
   HRESULT hr;
   IDirect3DDevice9* device = GetD3DDevice();

   //////////////////////////////////////////////////////////////////////////
   // Camera and matrices
   const D3DXMATRIX & view = *camera->GetViewMatrix();
   const D3DXMATRIX & proj = *camera->GetProjMatrix();
   //
   // offset for wireframe!
   D3DXMATRIX offProjWireframe;
   camera->GetZOffsettedProjMatrix(offProjWireframe, 1.001f, 0.01f);
   //
   D3DXMATRIX viewProj = view * proj;
   D3DXMATRIX offViewProjWireframe = view * offProjWireframe;
   //
   D3DXPLANE planes[6];
   camera->GetFrustumPlanes( planes );
   //////////////////////////////////////////////////////////////////////////

   //////////////////////////////////////////////////////////////////////////
   // Setup mesh
   V( device->SetStreamSource(0, m_gridVertexBuffer, 0, sizeof(PositionVertex) ) );
   V( device->SetIndices(m_gridIndexBuffer) );
   V( device->SetFVF( PositionVertex::FVF ) );

   //////////////////////////////////////////////////////////////////////////
   // This will store the selection of quads that we want to render 
   const int maxSelCount = 2048;
   wsQuadTree::NodeSelection nodeSelection[maxSelCount];
   wsQuadTree::LODSelectionInfo lodSelectionInfo;
   int selCount = 0;
   //////////////////////////////////////////////////////////////////////////


   //////////////////////////////////////////////////////////////////////////
   //////////////////////////////////////////////////////////////////////////
   // Render terrain
   //
   // Select terrain quads!
   selCount = m_terrainQuadTree.LODSelect( nodeSelection, maxSelCount, lodSelectionInfo, planes, camera->GetPosition(), camera->GetViewRange() * 0.95f );
   //
   //////////////////////////////////////////////////////////////////////////
   // Render terrain pre-pass to m_depthBufferTexture - outputs only distance-to-camera 
   // value. Also writes to Z buffer so will save some pixel processing in the next 
   // (regular) terrain rendering pass.
   // Could be done from the regular terrain rendering pass using MRT too...
   /*
   {
      V( device->SetVertexShader( m_vsWriteWaterDepth ) );
      SetupGlobalVSConsts( m_vsWriteWaterDepth.GetConstantTable(), viewProj, camera->GetPosition(), NULL, m_rasterWidth, m_rasterHeight );
      
      m_vsWriteWaterDepth.SetTexture( "g_surfaceWaterHMVertexTexture", m_waterHMTexture, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
      m_vsWriteWaterDepth.SetTexture( "g_surfaceHMVertexTexture", m_terrainHMTexture, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR );

      IDirect3DSurface9 * oldRenderTarget;
      V( device->GetRenderTarget( 0, &oldRenderTarget ) );

      wsSetRenderTarget( 0, m_depthBufferTexture );

      V( device->Clear( 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB( 0, 0, 0, 0 ), 1.0f, 0 ) );

      V( device->SetPixelShader( m_psWriteWaterDepth ) );

      RenderNodes( nodeSelection, selCount, lodSelectionInfo, m_vsWriteWaterDepth.GetConstantTable(), NULL ); // RENDER!

      V( device->SetRenderTarget( 0, oldRenderTarget ) );

      SAFE_RELEASE( oldRenderTarget );
   }
   */
   //
   //////////////////////////////////////////////////////////////////////////
   // Now render the actual terrain mesh
   //
   // Choose the shader
   wsPixelShader & psTerrain = (m_useTerrainFlatShading)?(m_psTerrainFlat):(m_psTerrainSplatting);
   //
   V( device->SetVertexShader( m_vsGrid ) );
   SetupGlobalVSConsts( m_vsGrid.GetConstantTable(), viewProj, camera->GetPosition(), lightMgr, m_rasterWidth, m_rasterHeight );
   //
   m_vsGrid.SetTexture( "g_surfaceHMVertexTexture", m_terrainHMTexture, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
   m_vsGrid.SetTexture( "g_surfaceNMVertexTexture", m_terrainNMTexture, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
   m_vsGrid.SetTexture( "g_surfaceWaterHMVertexTexture", m_waterHMTexture, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
   //
   //psTerrain.SetFloatArray( device, "g_fogColor", c_fogColor, 4 );
   //
   V( device->SetPixelShader( psTerrain ) );
   psTerrain.SetFloatArray( "g_lightColorDiffuse", c_lightColorDiffuse, 4 );
   psTerrain.SetFloatArray( "g_lightColorAmbient", c_lightColorAmbient, 4 );
   //
   if( !m_useTerrainFlatShading )
   {
      wsTexSplattingMgr::Instance().SetupShaderParams( psTerrain.GetConstantTable() );
      psTerrain.SetTexture( "g_splatMapTexture", m_terrainSplatMapTexture, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
      psTerrain.SetFloatArray( "g_rasterSize", (float)m_rasterWidth, (float)m_rasterHeight );
   }
   //

   psTerrain.SetFloatArray( "g_clipConsts", 0.0f, 0.0f );

   if( m_settings.UseProperUnderwaterRendering )
   {
      const float zDrawMargin = m_mapDims.SizeZ / 500.0f;
      psTerrain.SetFloatArray( "g_clipConsts", +zDrawMargin, 1.0f );
      {
         IDirect3DSurface9 * oldRenderTarget;
         V( device->GetRenderTarget( 0, &oldRenderTarget ) );
         wsSetRenderTarget( 0, m_renderTargetDataTmp );
         RenderNodes( nodeSelection, selCount, lodSelectionInfo, m_vsGrid.GetConstantTable(), psTerrain.GetConstantTable() ); // RENDER!
         V( device->SetRenderTarget( 0, oldRenderTarget ) );
         SAFE_RELEASE( oldRenderTarget );
      }
      //psTerrain.SetFloatArray( "g_clipConsts", +zDrawMargin, -1.0f );
      psTerrain.SetFloatArray( "g_clipConsts", +zDrawMargin, 0.0f );    // we have to draw everything for the case when camera is submerged (uggg)
   }

   RenderNodes( nodeSelection, selCount, lodSelectionInfo, m_vsGrid.GetConstantTable(), psTerrain.GetConstantTable() ); // RENDER!
   //
   if( m_wireframe )
   {
      V( m_vsGrid.SetMatrix( "g_viewProjection", offViewProjWireframe ) );

      device->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
      device->SetPixelShader( m_psWireframe );
      m_psWireframe.SetFloatArray( "g_justOutputColorValue", 0.0f, 0.0f, 0.0f, 1.0f );
      RenderNodes( nodeSelection, selCount, lodSelectionInfo, m_vsGrid.GetConstantTable(), NULL );   // RENDER!
      device->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
   }
   //////////////////////////////////////////////////////////////////////////
   //////////////////////////////////////////////////////////////////////////

   //////////////////////////////////////////////////////////////////////////
   // Get the rendered terrain - used for rendering refraction
   if( !m_settings.UseProperUnderwaterRendering )
   {
      // Get rendered stuff to be used in water pass for water refraction/alpha.
      IDirect3DSurface9 * rendTarget, * tmpSurface;
      V( device->GetRenderTarget(0, &rendTarget) );
      V( m_renderTargetDataTmp->GetSurfaceLevel(0, &tmpSurface) );
      V( device->StretchRect( rendTarget, NULL, tmpSurface, NULL, D3DTEXF_POINT ) );
      V( tmpSurface->Release() );
      V( rendTarget->Release() );
   }

   //////////////////////////////////////////////////////////////////////////
   // Render water
   //
   {
      const bool useCWSVertexOffset = m_settings.UseVertexWaveZOffset;
      wsVertexShader & vertexShader = (useCWSVertexOffset)?(m_vsGridWaterCWSOffset):(m_vsGridWater);

      // Select water quads!
      selCount = m_waterQuadTree.LODSelect( nodeSelection, maxSelCount, lodSelectionInfo, planes, camera->GetPosition(), camera->GetViewRange() * 0.95f );
      //
      V( device->SetVertexShader( vertexShader ) );
      V( device->SetPixelShader( m_psWater ) );
      SetupGlobalVSConsts( vertexShader.GetConstantTable(), viewProj, camera->GetPosition(), lightMgr, m_rasterWidth, m_rasterHeight );
      device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
      device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
      device->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
      //device->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
      //
      float rtWidth = (float)m_backBufferSurfaceDesc.Width;
      float rtHeight = (float)m_backBufferSurfaceDesc.Height;
      m_psWater.SetVector( "g_VPOS_2_ScreenUV", D3DXVECTOR4( 1.0f / rtWidth, 1.0f / rtHeight, - 0.5f / rtWidth, - 0.5f / rtHeight ) );
      m_psWater.SetTexture( "g_waterBackgroundTexture", m_renderTargetDataTmp, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
      //m_psWater.SetTexture( "g_waterDepthTexture", m_depthBufferTexture );
      vertexShader.SetTexture( "g_surfaceHMVertexTexture", m_waterHMTexture, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
      vertexShader.SetTexture( "g_surfaceNMVertexTexture", m_waterNMTexture, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR );

      if( lightMgr->IsUsingSkybox() )
      {
         m_psWater.SetFloat( "g_usingSkyCubemap", 1.0f );
         m_psWater.SetTexture( "g_skyCubeMapTexture", lightMgr->GetSkyboxTexture(), D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
      }
      else
      {
         m_psWater.SetFloat( "g_usingSkyCubemap", 0.0f );
         m_psWater.SetTexture( "g_skyCubeMapTexture", NULL );
      }

      //device->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
      RenderNodesWithCWM( nodeSelection, selCount, lodSelectionInfo, camera, vertexShader.GetConstantTable(), m_psWater.GetConstantTable() ); // RENDER!
      m_psWater.SetTexture( "g_waterBackgroundTexture", NULL );
      //m_psWater.SetTexture( "g_waterDepthTexture", NULL );
      if( m_wireframe )
      {
         V( vertexShader.SetMatrix( "g_viewProjection", offViewProjWireframe ) );

         device->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
         device->SetPixelShader( m_psWireframe );
         m_psWireframe.SetFloatArray( "g_justOutputColorValue", 1.0f, 0.0f, 0.5f, 1.0f );
         //RenderNodes( nodeSelection, selCount, lodSelectionInfo, vertexShader.GetConstantTable(), NULL); // RENDER!
         RenderNodesWithCWM( nodeSelection, selCount, lodSelectionInfo, camera, vertexShader.GetConstantTable(), NULL ); // RENDER!

         device->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
      }
      device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
      device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
      device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
      //device->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
   }
   //////////////////////////////////////////////////////////////////////////
  
   // DbgDrawText( 4, 52, L"triangles: %dk, test sphere bodies: %d", nTriangles/1024, m_Spheres.size() );

   //wsSetTexture( NULL, "g_heightmapTexture", m_VSCTGrid );
   //wsSetTexture( m_waterDepthTextureFront, "g_depthmapTexture", m_PSCTSimulationDisplay );
   //wsSetTexture( m_waterVelocityTextureFront, "g_velocitymapTexture", m_PSCTSimulationDisplay );

   RenderSprings( camera, lightMgr, deltaTime );

   V( device->SetStreamSource(0, NULL, 0, 0 ) );
   V( device->SetIndices(NULL) );

   V( device->SetVertexShader( NULL ) );
   V( device->SetPixelShader( NULL ) );

   // Render a texture to screen for debugging purposes
   /*
   {
      //IDirect3DTexture9 * waveMap = m_cascadedWaveMapSim.m_waveDetailMapTexture; //m_waveMapLayers[0]->m_velocityMap;
      float currentTime = (float)fmod( m_globalTime * 0.2f, 1.0 );
      int frameA = (int)(currentTime * m_detailWaves.TextureCount); 
      IDirect3DTexture9 * waveMap = m_detailWaves.NMTextures[frameA];

      GetCanvas2D()->DrawText( 10, GetCanvas2D()->GetHeight() - 20, L"frame: %d", frameA );

      {
         IDirect3DSurface9 * waveMapSurf;
         IDirect3DSurface9 * renderTargetSurf;
         device->GetRenderTarget(0, &renderTargetSurf);
         waveMap->GetSurfaceLevel(0, &waveMapSurf);

         int rOff = 10;
         int rSize = 256;
         int rDiff = 0;
         
         RECT rect;
         rect.top = rOff; rect.bottom = rect.top + rSize; rect.left = rOff; rect.right = rect.left + rSize;
         V( device->StretchRect( waveMapSurf, NULL, renderTargetSurf, &rect, D3DTEXF_POINT ) );
         rect.top = rOff+rSize+rDiff; rect.bottom = rect.top + rSize; rect.left = rOff+rSize+rDiff; rect.right = rect.left + rSize;
         V( device->StretchRect( waveMapSurf, NULL, renderTargetSurf, &rect, D3DTEXF_POINT ) );
         rect.top = rOff; rect.bottom = rect.top + rSize; rect.left = rOff+rSize+rDiff; rect.right = rect.left + rSize;
         V( device->StretchRect( waveMapSurf, NULL, renderTargetSurf, &rect, D3DTEXF_POINT ) );
         rect.top = rOff+rSize+rDiff; rect.bottom = rect.top + rSize; rect.left = rOff; rect.right = rect.left + rSize;
         V( device->StretchRect( waveMapSurf, NULL, renderTargetSurf, &rect, D3DTEXF_POINT ) );

         SAFE_RELEASE(renderTargetSurf);
         SAFE_RELEASE(waveMapSurf);
      }
   }
   */

   return S_OK;
}
//
void ws3DPreviewRenderer::RenderSprings( ws3DPreviewCamera * camera, ws3DPreviewLightMgr * lightMgr, float deltaTime )
{
   //HRESULT hr;
   IDirect3DDevice9* device = GetD3DDevice();
   //static wsPixelShader psAddSpring( "wsSimulation.psh", "addSpring" );

   D3DVIEWPORT9 viewport;
   device->GetViewport( &viewport );

   const float renderHeight = m_mapDims.SizeZ * 0.05f;
   int texResX = m_heightmap->Width();
   int texResY = m_heightmap->Height();

   //D3DXMATRIX viewProj = (*camera->GetViewMatrix()) * (*camera->GetProjMatrix());
   D3DXMATRIX world; D3DXMatrixIdentity( &world );
   D3DXMATRIX view   = *camera->GetViewMatrix();
   D3DXMATRIX proj   = *camera->GetProjMatrix();

   const float visibilityRange = (m_mapDims.SizeX + m_mapDims.SizeY) * 0.15f;

   const D3DXVECTOR3 & camPos = camera->GetPosition();

   const vector<wsSpringsMgr::Spring> & springs = wsSpringsMgr::Instance().GetSprings();
   for( size_t i = 0; i < springs.size(); i++ )
   {
      const wsSpringsMgr::Spring & spring = springs[i];

      float springWorldX = m_mapDims.MinX + spring.x * m_mapDims.SizeX;
      float springWorldY = m_mapDims.MinY + spring.y * m_mapDims.SizeY;
      float springWorldSizeX = spring.radius * m_mapDims.SizeX / (float)texResX;
      float springWorldSizeY = spring.radius * m_mapDims.SizeY / (float)texResY;

      float minZ, maxZ;
      m_terrainQuadTree.GetAreaMinMaxHeight( springWorldX - springWorldSizeX*0.5f, springWorldY - springWorldSizeY*0.5f, springWorldSizeX, springWorldSizeY, minZ, maxZ );

      D3DXVECTOR3 test = D3DXVECTOR3( springWorldX, springWorldY, (minZ+maxZ)*0.5f );

      test -= camPos;
      if( D3DXVec3Length(&test) > visibilityRange )
         continue;

      GetCanvas3D()->DrawBox( D3DXVECTOR3(springWorldX - springWorldSizeX*0.5f, springWorldY - springWorldSizeY*0.5f, minZ),
                              D3DXVECTOR3(springWorldX + springWorldSizeX*0.5f, springWorldY + springWorldSizeY*0.5f, maxZ + renderHeight), 0xFF80FF00, 0x00000000 );



      D3DXVECTOR3 topCenter = D3DXVECTOR3( springWorldX, springWorldY, maxZ + renderHeight );
      D3DXVECTOR3 topA = D3DXVECTOR3(springWorldX - springWorldSizeX*0.5f, springWorldY - springWorldSizeY*0.5f, maxZ + renderHeight);
      D3DXVECTOR3 topB = D3DXVECTOR3(springWorldX + springWorldSizeX*0.5f, springWorldY - springWorldSizeY*0.5f, maxZ + renderHeight);
      D3DXVECTOR3 topC = D3DXVECTOR3(springWorldX - springWorldSizeX*0.5f, springWorldY + springWorldSizeY*0.5f, maxZ + renderHeight);
      D3DXVECTOR3 topD = D3DXVECTOR3(springWorldX + springWorldSizeX*0.5f, springWorldY + springWorldSizeY*0.5f, maxZ + renderHeight);

      D3DXVec3Project( &topCenter, &topCenter, &viewport, &proj, &view, &world );
      D3DXVec3Project( &topA, &topA, &viewport, &proj, &view, &world );
      D3DXVec3Project( &topB, &topB, &viewport, &proj, &view, &world );
      D3DXVec3Project( &topC, &topC, &viewport, &proj, &view, &world );
      D3DXVec3Project( &topD, &topD, &viewport, &proj, &view, &world );

      D3DXVECTOR3 m0 = topA - topD;
      D3DXVECTOR3 m1 = topB - topC;
      if( (::max(D3DXVec3Length(&m0), D3DXVec3Length(&m1)) < (viewport.Width+viewport.Height)*0.03f) || (topCenter.z > 1.0) )
         continue;

      //int sx = (int)((topA.x + topB.x + topC.x + topD.x) * 0.25f - 130);
      //int sy = (int)::max( ::max(topA.y, topB.y), ::max(topC.y, topD.y) ) + 16;
      int sx = (int)(topCenter.x - 130);
      int sy = (int)(topCenter.y + 16);

      GetCanvas2D()->DrawText( sx, sy, L"Spring no %02d, water quantity: %.3f, radius: %.1f", i, spring.quantity, spring.radius );
   }
}
//
void ws3DPreviewRenderer::UpdateVelocityTexture( float fromX, float fromY, float sizeX, float sizeY, float minZ, float maxZ, IDirect3DTexture9 * velocityMap )
{
   HRESULT hr;
   IDirect3DDevice9* device = GetD3DDevice();
   D3DXMATRIX view, proj;
   float v[2];

   D3DSURFACE_DESC vmSurfDesc;
   V( velocityMap->GetLevelDesc(0, &vmSurfDesc) );

   V( device->BeginScene() );

   //D3DVIEWPORT9 vp;
   //device->GetViewport(&vp);

   //GetCanvas3D()->DrawBox( D3DXVECTOR3(fromX, fromY, minZ), D3DXVECTOR3(fromX + sizeX, fromY + sizeY, maxZ), 0xFFFFFFFF, 0x800000FF );

   float zn = 1.0f;

   // Half pixel offset so that velocity texture matches terrain/water when sampled
   fromX += (1.0f / (float)vmSurfDesc.Width) * sizeX * 0.5f;
   fromY += (1.0f / (float)vmSurfDesc.Height) * sizeY * 0.5f;

   D3DXVECTOR3 eye( fromX + sizeX * 0.5f, fromY + sizeY * 0.5f, maxZ + zn );
   D3DXVECTOR3 lookAt( fromX + sizeX * 0.5f, fromY + sizeY * 0.5f, minZ );
   D3DXVECTOR3 up( 0, -1, 0 );

   D3DXMatrixLookAtLH( &view, &eye, &lookAt, &up );
   D3DXMatrixOrthoLH( &proj, sizeX, sizeY, zn, maxZ - minZ + zn );

   D3DXMATRIX viewProj = view * proj;

   const int maxSelCount = 2048;
   wsQuadTree::NodeSelection nodeSelection[maxSelCount];
   wsQuadTree::LODSelectionInfo lodSelectionInfo;
   D3DXPLANE planes[6];
   wsGetFrustumPlanes(planes, viewProj);
   int selCount = m_terrainQuadTree.LODSelect( nodeSelection, maxSelCount, lodSelectionInfo, planes, eye, m_cameraViewRange * 0.95f );

   //for( int i = 0; i < selCount; i++ )
   //{
   //   nodeSelection[i].pNode->DebugDrawAABB(0xFF00FF00);
   //}

   {
      // Create depth and temp buffers if needed!

      if( m_velocityMapUpdateBackBuffer != NULL )
      {
         D3DSURFACE_DESC surfDesc;
         V( m_velocityMapUpdateBackBuffer->GetDesc( &surfDesc ) );
         if( (surfDesc.Width != vmSurfDesc.Width) || (surfDesc.Height != vmSurfDesc.Height) )
         {
            SAFE_RELEASE( m_velocityMapUpdateBackBuffer );
         }
      }
      if( m_velocityMapUpdateBackBuffer == NULL )
      {
         V( device->CreateDepthStencilSurface( vmSurfDesc.Width, vmSurfDesc.Height, D3DFMT_D16, D3DMULTISAMPLE_NONE, 0, TRUE, &m_velocityMapUpdateBackBuffer, NULL ) );
      }

      if( m_velocityMapFilterTemp != NULL )
      {
         D3DSURFACE_DESC surfDesc;
         V( m_velocityMapFilterTemp->GetLevelDesc( 0, &surfDesc ) );
         if( (surfDesc.Width != vmSurfDesc.Width) || (surfDesc.Height != vmSurfDesc.Height) )
         {
            SAFE_RELEASE( m_velocityMapFilterTemp );
         }
      }
      if( m_velocityMapFilterTemp == NULL )
      {
         V( device->CreateTexture( vmSurfDesc.Width, vmSurfDesc.Height, 1, D3DUSAGE_RENDERTARGET, WAVE_VELOCITY_MAP_TEXTURE_FORMAT, D3DPOOL_DEFAULT, &m_velocityMapFilterTemp, NULL ) );
      }

   }

   //////////////////////////////////////////////////////////////////////////
   // Backup current render target and set the texture as one
   IDirect3DSurface9 * oldRenderTarget;
   IDirect3DSurface9 * oldDepthStencil;
   V( device->GetRenderTarget( 0, &oldRenderTarget ) );
   V( device->GetDepthStencilSurface( &oldDepthStencil ) );
   V( device->SetDepthStencilSurface( m_velocityMapUpdateBackBuffer ) );

   wsSetRenderTarget( 0, velocityMap );

   V( device->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB( 0, 128, 128, 0 ), 1.0f, 0 ) ); //D3DCOLOR_ARGB( 0, 45, 50, 170 )

   //////////////////////////////////////////////////////////////////////////
   // Setup mesh
   V( device->SetStreamSource(0, m_gridVertexBuffer, 0, sizeof(PositionVertex) ) );
   V( device->SetIndices(m_gridIndexBuffer) );
   V( device->SetFVF( PositionVertex::FVF ) );

   //////////////////////////////////////////////////////////////////////////
   // Render terrain so that edges between water and terrain are correctly marked
   V( device->SetVertexShader( m_vsVelocityMap ) );
   V( device->SetPixelShader( m_psVelocityMap ) );
   SetupGlobalVSConsts( m_vsVelocityMap.GetConstantTable(), viewProj, eye, NULL, m_rasterWidth, m_rasterHeight );
   m_vsVelocityMap.SetTexture( "g_surfaceHMVertexTexture", m_terrainHMTexture, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
   m_vsVelocityMap.SetTexture( "g_surfaceVMVertexTexture", NULL, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
   device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ZERO );
   device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
   device->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
   RenderNodes( nodeSelection, selCount, lodSelectionInfo, m_vsVelocityMap.GetConstantTable(), NULL ); // RENDER!
   device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
   device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
   device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
   //V( device->SetRenderState( 
   //////////////////////////////////////////////////////////////////////////

   //////////////////////////////////////////////////////////////////////////
   // Render water
   V( device->SetVertexShader( m_vsVelocityMap ) );
   V( device->SetPixelShader( m_psVelocityMap ) );
   SetupGlobalVSConsts( m_vsVelocityMap.GetConstantTable(), viewProj, eye, NULL, m_rasterWidth, m_rasterHeight );
   //device->SetRenderState( D3DRS_ZENABLE, TRUE );
   //device->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
   //
   m_vsVelocityMap.SetTexture( "g_surfaceHMVertexTexture", m_waterHMTexture, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
   m_vsVelocityMap.SetTexture( "g_surfaceVMVertexTexture", m_waterVMTexture, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
   RenderNodes( nodeSelection, selCount, lodSelectionInfo, m_vsVelocityMap.GetConstantTable(), NULL ); // RENDER!
   //////////////////////////////////////////////////////////////////////////

   //////////////////////////////////////////////////////////////////////////
   // Apply filters
   V( device->SetVertexShader( NULL ) );
   //
   // First filter
   V( device->SetPixelShader( m_psVelocityMapFilter0 ) );
   //
   wsSetRenderTarget( 0, m_velocityMapFilterTemp );
   m_psVelocityMapFilter0.SetTexture( "g_velocitymapTexture", velocityMap );
   //
   v[0] = (m_mapDims.SizeX / (float)m_rasterWidth) / sizeX;    v[1] = (m_mapDims.SizeY / (float)m_rasterHeight) / sizeY;
   m_psVelocityMapFilter0.SetFloatArray( "g_samplerPixelSize", v, 2 );
   //
   wsRenderSimulation( vmSurfDesc.Width, vmSurfDesc.Height );
   //
   m_psVelocityMapFilter0.SetTexture( "g_velocitymapTexture", NULL );
   //
   //
   // Second filter
   V( device->SetPixelShader( m_psVelocityMapFilter1 ) );
   //
   wsSetRenderTarget( 0, velocityMap );
   m_psVelocityMapFilter1.SetTexture( "g_velocitymapTexture", m_velocityMapFilterTemp );
   //
   v[0] = 1.0f / (float)vmSurfDesc.Width;    v[1] = 1.0f / (float)vmSurfDesc.Height;
   m_psVelocityMapFilter0.SetFloatArray( "g_samplerPixelSize", v, 2 );
   //
   wsRenderSimulation( vmSurfDesc.Width, vmSurfDesc.Height );
   //
   m_psVelocityMapFilter1.SetTexture( "g_velocitymapTexture", NULL );
   //
   //////////////////////////////////////////////////////////////////////////

   // Restore device settings
   V( device->SetStreamSource(0, NULL, 0, 0 ) );
   V( device->SetIndices(NULL) );
   V( device->SetPixelShader( NULL ) );
   V( device->SetRenderTarget( 0, oldRenderTarget ) );
   V( device->SetDepthStencilSurface( oldDepthStencil ) );

   device->SetRenderState( D3DRS_ZENABLE, TRUE );
   //device->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );

   SAFE_RELEASE( oldRenderTarget );
   SAFE_RELEASE( oldDepthStencil );

   V( device->EndScene() );
}
//
void ws3DPreviewRenderer::MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing )
{
   switch( msg )
   {
   case WM_LBUTTONDOWN:
      m_mouseLeftButtonDown = true;
      *pbNoFurtherProcessing = true;
      break;
   case WM_LBUTTONUP:
      m_mouseLeftButtonDown = false;
      *pbNoFurtherProcessing = true;
      break;
   case WM_MOUSEMOVE:
      m_mouseClientX = ( short )LOWORD( lParam );
      m_mouseClientY = ( short )HIWORD( lParam );
      break;
   case WM_KILLFOCUS:
      m_mouseLeftButtonDown = false;
      break;
   }
}
//