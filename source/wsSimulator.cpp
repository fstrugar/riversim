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

#include "wsSimulator.h"

#include "ws3DPreviewCamera.h"

#include "ws3DPreviewLightMgr.h"

#include "iniparser\src\IniParser.hpp"

#include "wsCanvas.h"

#include "wsSpringsMgr.h"

static int c_renderTopBorder        = 85;
static int c_renderBorder           = 5;

wsSimulator::wsSimulator(void)
{
   m_psSimulationDisplay.SetShaderInfo( "wsSimulationDisplay.psh", "main" );
   m_psSimulationDiffuseOnly.SetShaderInfo( "wsSimulation.psh", "simulateDiffuseOnly" );
   m_psSimulationDiffuseAndTheRest.SetShaderInfo( "wsSimulation.psh", "simulateDiffuseAndTheRest" );
   m_psSimulationVelocityPropagateStep.SetShaderInfo( "wsSimulation.psh", "velocityPropagate" );
   m_psSimulationSplash.SetShaderInfo( "wsSimulation.psh", "splashArea" );

   m_heightmapTexture         = NULL;

   m_heightmap    = NULL;
   m_watermap     = NULL;

   m_waterDepthTextureFront      = NULL;
   m_waterDepthTextureBack       = NULL;
   m_waterVelocityTextureFront   = NULL;
   m_waterVelocityTextureBack    = NULL;

   m_springMapTexture            = NULL;

   m_springMarkerTexture         = NULL;

   m_mouseLeftButtonDown      = false;
   m_mouseRightButtonDown     = false;

   m_displayVelocities        = false;

   m_time                     = 0;

   m_mouseSampledWaterDepth      = 0.0f;
   m_mouseSampledWaterVelocityX  = 0.0f;
   m_mouseSampledWaterVelocityY  = 0.0f;
   m_mouseSampledTerrainHeight   = 0.0f;

   memset( &m_BackBufferSurfaceDesc, 0, sizeof(m_BackBufferSurfaceDesc) );

   m_depthSumDisplay          = false;
   m_depthSum                 = 0.0f;
   m_springBoostMode          = false;
}

wsSimulator::~wsSimulator(void)
{
   //Stop();
   assert( m_heightmap == NULL ); // Stop() must have already been called!
}

void wsSimulator::Start( AdVantage::TiledBitmap * heightmap, AdVantage::TiledBitmap * watermap, const MapDimensions & mapDims )
{
   assert( m_heightmap == NULL );
   assert( m_watermap == NULL );

   m_heightmap    = heightmap; 
   m_watermap     = watermap;

   m_rasterWidth  = heightmap->Width();
   m_rasterHeight = heightmap->Height();

   m_mapDims = mapDims;

   InitializeFromIni();

   CenterDisplay();

   m_simTickCount = 0;
   m_springBoostMode          = false;
}

void wsSimulator::InitializeFromIni()
{
   IniParser   iniParser( wsGetIniPath() );
   m_waterIgnoreBelowDepth = iniParser.getFloat("Exporter:waterIgnoreBelowDepth", 10.0f);

   m_simSettings.Absorption               = iniParser.getFloat("Simulation:absorption", m_simSettings.Absorption);
   m_simSettings.Precipitation            = iniParser.getFloat("Simulation:precipitation", m_simSettings.Precipitation);
   m_simSettings.PrecipitationTicks       = iniParser.getInt("Simulation:precipitationTicks", m_simSettings.PrecipitationTicks);
   m_simSettings.Evaporation              = iniParser.getFloat("Simulation:evaporation", m_simSettings.Evaporation);
   m_simSettings.EvaporationTicks         = iniParser.getInt("Simulation:evaporationTicks", m_simSettings.EvaporationTicks);
   m_simSettings.LinVelocityDamping       = iniParser.getFloat("Simulation:linVelocityDamping", m_simSettings.LinVelocityDamping);
   m_simSettings.SqrVelocityDamping       = iniParser.getFloat("Simulation:sqrVelocityDamping", m_simSettings.SqrVelocityDamping);
   m_simSettings.Diffusion                = iniParser.getFloat("Simulation:diffusion", m_simSettings.Diffusion);
   m_simSettings.Acceleration             = iniParser.getFloat("Simulation:acceleration", m_simSettings.Acceleration);

   m_simSettings.DepthBasedFrictionMinDepth  = iniParser.getFloat("Simulation:depthBasedFrictionMinDepth", m_simSettings.DepthBasedFrictionMinDepth);
   m_simSettings.DepthBasedFrictionMaxDepth  = iniParser.getFloat("Simulation:depthBasedFrictionMaxDepth", m_simSettings.DepthBasedFrictionMaxDepth);
   m_simSettings.DepthBasedFrictionAmount    = iniParser.getFloat("Simulation:depthBasedFrictionAmount", m_simSettings.DepthBasedFrictionAmount);

}

void wsSimulator::Stop( )
{
   OnLostDevice();
   OnDestroyDevice();

   m_heightmap    = NULL;
   m_watermap     = NULL;
   
   m_rasterWidth  = 0;
   m_rasterHeight = 0;
}

void wsSimulator::InitializeTextures()
{
   IDirect3DDevice9 * device = GetD3DDevice();
   HRESULT hr;
   //D3DSURFACE_DESC sdesc;

   // If not loaded, load heightmap texture
   if( m_heightmapTexture == NULL )
   {
      assert( m_rasterWidth   == m_heightmap->Width() );
      assert( m_rasterHeight  == m_heightmap->Height() );

      V( device->CreateTexture( m_rasterWidth, m_rasterHeight, 1, 0, D3DFMT_L16, D3DPOOL_MANAGED, &m_heightmapTexture, NULL ) );

	   //V( m_heightmapTexture->GetLevelDesc( 0, &sdesc ) );

      //////////////////////////////////////////////////////////////////////////
      // Load heightmap
      //
      D3DLOCKED_RECT lockedRect;
      V( m_heightmapTexture->LockRect( 0, &lockedRect, NULL, 0 ) );

      unsigned short * data = (unsigned short *)lockedRect.pBits;

      for( int y = 0; y < m_rasterHeight; y++ )
      {
         for( int x = 0; x < m_rasterWidth; x++ )
         {
            m_heightmap->GetPixel( x, y, &data[x] );
         }
         data += lockedRect.Pitch / sizeof(*data);
      }
      //lockedRect->pBits
      m_heightmapTexture->UnlockRect( 0 );
      //////////////////////////////////////////////////////////////////////////
   }

   if( m_waterDepthTextureFront == NULL )
   {
      assert( m_waterDepthTextureBack == NULL );
      assert( m_waterVelocityTextureFront == NULL );
      assert( m_waterVelocityTextureBack == NULL );

      V( device->CreateTexture( m_rasterWidth, m_rasterHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &m_waterDepthTextureFront, NULL ) );
      V( device->CreateTexture( m_rasterWidth, m_rasterHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &m_waterDepthTextureBack, NULL ) );

      V( device->CreateTexture( m_rasterWidth, m_rasterHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G16R16F, D3DPOOL_DEFAULT, &m_waterVelocityTextureFront, NULL ) );
      V( device->CreateTexture( m_rasterWidth, m_rasterHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G16R16F, D3DPOOL_DEFAULT, &m_waterVelocityTextureBack, NULL ) );

      V( device->CreateTexture( 1, 1, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A32B32G32R32F, D3DPOOL_DEFAULT, &m_waterDepthSampler, NULL ) );
      //V( device->CreateTexture( 1, 1, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G16R16F, D3DPOOL_DEFAULT, &m_waterVelocitySampler, NULL ) );

	   //V( m_waterDepthTextureFront->GetLevelDesc( 0, &sdesc ) );
	   //V( m_waterDepthTextureBack->GetLevelDesc( 0, &sdesc ) );
	   //V( m_waterVelocityTextureFront->GetLevelDesc( 0, &sdesc ) );
	   //V( m_waterVelocityTextureBack->GetLevelDesc( 0, &sdesc ) );

      //////////////////////////////////////////////////////////////////////////
      // Initialize values
      //
      LoadFromWatermapFile();
      //////////////////////////////////////////////////////////////////////////
   }

   if( m_springMapTexture == NULL )
   {
      IDirect3DTexture9 * tempTex = NULL;

      V( device->CreateTexture( m_rasterWidth, m_rasterHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &m_springMapTexture, NULL ) );
      V( device->CreateTexture( m_rasterWidth, m_rasterHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &tempTex, NULL ) );

      static wsPixelShader psClearToColor( "wsMisc.psh", "justOutputColor" );

      V( wsSetRenderTarget( 0, m_springMapTexture ) );
      V( device->SetPixelShader( psClearToColor ) );
      V( psClearToColor.SetFloatArray( "g_justOutputColorValue", 0.0f, 0.0f, 0.0f, 0.0f ) );

      wsRenderSimulation( m_rasterWidth, m_rasterHeight );

      static wsPixelShader psAddSpring( "wsSimulation.psh", "addSpring" );

      V( device->SetPixelShader( psAddSpring ) );

      const vector<wsSpringsMgr::Spring> & springs = wsSpringsMgr::Instance().GetSprings();
      for( size_t i = 0; i < springs.size(); i++ )
      {
         const wsSpringsMgr::Spring & spring = springs[i];

         float springX = floorf( spring.x * m_rasterWidth + 0.5f ) / (float)m_rasterWidth;
         float springY = floorf( spring.y * m_rasterHeight + 0.5f ) / (float)m_rasterHeight;

         V( wsSetRenderTarget( 0, tempTex ) );

         V( psAddSpring.SetTexture( "g_depthmapTexture", m_springMapTexture ) );

         V( psAddSpring.SetFloatArray( "g_springPos", springX, springY ) );
         V( psAddSpring.SetFloat( "g_springRadius", spring.radius ) );
         V( psAddSpring.SetFloat( "g_springNormalizedQuantity", spring.normalizedQuantity ) );

         V( psAddSpring.SetFloatArray( "g_rasterSize", (float)m_rasterWidth, (float)m_rasterHeight ) );

         wsRenderSimulation( m_rasterWidth, m_rasterHeight );
         
         V( psAddSpring.SetTexture( "g_depthmapTexture", NULL ) );

         ::swap( m_springMapTexture, tempTex );
        
      }

      SAFE_RELEASE( tempTex );
   }


}
//
void wsSimulator::ExtractTotalDepthSum( double & totalSum )
{
   IDirect3DDevice9 * device = GetD3DDevice();
   HRESULT hr;

   IDirect3DTexture9 * waterDepthSysMem;

   V( device->CreateTexture( m_rasterWidth, m_rasterHeight, 1, 0, D3DFMT_R32F, D3DPOOL_SYSTEMMEM, &waterDepthSysMem, NULL ) );

   V( wsGetRenderTargetData( m_waterDepthTextureFront, waterDepthSysMem ) );

   D3DLOCKED_RECT lockedRectDepth;
   V( waterDepthSysMem->LockRect( 0, &lockedRectDepth, NULL, 0 ) );
   //
   float * dataDepth       = (float *)lockedRectDepth.pBits;

   totalSum = 0;

   for( int y = 0; y < m_rasterHeight; y++ )
   {
      for( int x = 0; x < m_rasterWidth; x++ )
      {
         totalSum += dataDepth[x];
      }
      dataDepth += lockedRectDepth.Pitch / sizeof(*dataDepth);
   }
   //
   V( waterDepthSysMem->UnlockRect(0) );
   //
   SAFE_RELEASE( waterDepthSysMem );
}
//
void wsSimulator::ExtractMouseSampleData( float & depth, float & velocityX, float & velocityY, float & height )
{
   HRESULT hr;

   V( wsGetRenderTargetData( m_waterDepthSampler, m_waterDepthSamplerSysMem ) );
   //V( wsGetRenderTargetData( m_waterVelocitySampler, m_waterVelocitySamplerSysMem ) );

   D3DLOCKED_RECT lockedRectDepth;//, lockedRectVel;
   V( m_waterDepthSamplerSysMem->LockRect( 0, &lockedRectDepth, NULL, 0 ) );
   //V( m_waterVelocitySamplerSysMem->LockRect( 0, &lockedRectVel, NULL, 0 ) );
   //
   float * dataDepth       = (float *)lockedRectDepth.pBits;
   depth       = dataDepth[0];
   velocityX   = dataDepth[1];
   velocityY   = dataDepth[2];
   height      = dataDepth[3];
   /*
   unsigned int * dataVel  = (unsigned int *)lockedRectVel.pBits;
   //
   depth = *dataDepth;
   velocityX = HalfFloatUnpack( (unsigned short)(*dataVel) );
   velocityY = HalfFloatUnpack( (unsigned short)((*dataVel)>>16) );
   //
   */
   V( m_waterDepthSamplerSysMem->UnlockRect(0) );
   //V( m_waterVelocitySamplerSysMem->UnlockRect(0) );
}
//
void wsSimulator::LoadFromWatermapFile()
{
   IDirect3DDevice9 * device = GetD3DDevice();
   HRESULT hr;
   //
   IDirect3DTexture9 * depthTemp;
   IDirect3DTexture9 * velTemp;
   V( device->CreateTexture( m_rasterWidth, m_rasterHeight, 1, 0, D3DFMT_R32F, D3DPOOL_SYSTEMMEM, &depthTemp, NULL ) );
   V( device->CreateTexture( m_rasterWidth, m_rasterHeight, 1, 0, D3DFMT_G16R16F, D3DPOOL_SYSTEMMEM, &velTemp, NULL ) );
   //
   //V( device->GetRenderTargetData(m_waterDepthTextureFront, depthTemp) );
   //
   {
      D3DLOCKED_RECT lockedRectVel;
      V( velTemp->LockRect( 0, &lockedRectVel, NULL, 0 ) );
      //
      D3DLOCKED_RECT lockedRectDepth;
      V( depthTemp->LockRect( 0, &lockedRectDepth, NULL, 0 ) );
      //
      float * dataDepth       = (float *)lockedRectDepth.pBits;
      unsigned int * dataVel  = (unsigned int *)lockedRectVel.pBits;
      //
      for( int y = 0; y < m_rasterHeight; y++ )
      {
         for( int x = 0; x < m_rasterWidth; x++ )
         {
            __int64  pixel;
            m_watermap->GetPixel( x, y, &pixel );

            unsigned int val  = (unsigned int)(pixel >> 32);
            dataDepth[x]      = reinterpret_cast<float&>(val);

            dataVel[x]        = (unsigned int)(pixel & 0xFFFFFFFF);

            //dataDepth[x] = 0.0f; //(x + y) * 0.0001f;
            //dataVel[x] = HalfFloatPack( 0.0f ) | (HalfFloatPack( 0.0f ) << 16);
         }
         dataDepth += lockedRectDepth.Pitch / sizeof(*dataDepth);
         dataVel += lockedRectVel.Pitch / sizeof(*dataVel);
      }
      //
      velTemp->UnlockRect( 0 );
      depthTemp->UnlockRect( 0 );
   }

   //
   V( device->UpdateTexture(depthTemp, m_waterDepthTextureBack) );
   V( device->UpdateTexture(velTemp, m_waterVelocityTextureBack) );
   
   depthTemp->AddDirtyRect( NULL );
   velTemp->AddDirtyRect( NULL );

   V( device->UpdateTexture(depthTemp, m_waterDepthTextureFront) );
   V( device->UpdateTexture(velTemp, m_waterVelocityTextureFront) );
   //
   SAFE_RELEASE( velTemp );
   SAFE_RELEASE( depthTemp );
}
//
void wsSimulator::SaveToWatermapFile()
{
   if( m_watermap == NULL )
      return;

   IDirect3DDevice9 * device = GetD3DDevice();
   HRESULT hr;

   IDirect3DSurface9 * depthTemp;
   IDirect3DSurface9 * velTemp;
   V( device->CreateOffscreenPlainSurface( m_rasterWidth, m_rasterHeight, D3DFMT_R32F, D3DPOOL_SYSTEMMEM, &depthTemp, NULL ) );
   V( device->CreateOffscreenPlainSurface( m_rasterWidth, m_rasterHeight, D3DFMT_G16R16F, D3DPOOL_SYSTEMMEM, &velTemp, NULL ) );
   //
   IDirect3DSurface9 * depthTextureSurf;
   IDirect3DSurface9 * velTextureSurf;
   m_waterDepthTextureFront->GetSurfaceLevel( 0, &depthTextureSurf );
   m_waterVelocityTextureFront->GetSurfaceLevel( 0, &velTextureSurf );
   V( device->GetRenderTargetData(depthTextureSurf, depthTemp) );
   V( device->GetRenderTargetData(velTextureSurf, velTemp) );
   depthTextureSurf->Release();
   velTextureSurf->Release();
   //
   D3DLOCKED_RECT lockedRectVel;
   V( velTemp->LockRect( &lockedRectVel, NULL, 0 ) );
   //
   D3DLOCKED_RECT lockedRectDepth;
   V( depthTemp->LockRect( &lockedRectDepth, NULL, 0 ) );
   //
   float * dataDepth       = (float *)lockedRectDepth.pBits;
   unsigned int * dataVel  = (unsigned int *)lockedRectVel.pBits;
   //
   for( int y = 0; y < m_rasterHeight; y++ )
   {
      for( int x = 0; x < m_rasterWidth; x++ )
      {
         __int64  pixel = (((__int64)reinterpret_cast<unsigned int&>(dataDepth[x])) << 32) | dataVel[x];
         m_watermap->SetPixel( x, y, &pixel );
         //dataDepth[x] = 0.0f; //(x + y) * 0.0001f;
         //dataVel[x] = HalfFloatPack( 0.0f ) | (HalfFloatPack( 0.0f ) << 16);
      }
      dataDepth += lockedRectDepth.Pitch / sizeof(*dataDepth);
      dataVel += lockedRectVel.Pitch / sizeof(*dataVel);
   }
   //
   velTemp->UnlockRect( );
   depthTemp->UnlockRect( );
   //
   SAFE_RELEASE( velTemp );
   SAFE_RELEASE( depthTemp );

   {  
      //V( device->GetRenderTargetData(m_waterVelocityTextureFront, velTemp) );
      //
      // Velocity
   }
}
//
HRESULT wsSimulator::OnCreateDevice( )
{
   HRESULT hr;
   IDirect3DDevice9 * device = GetD3DDevice();

   V( D3DXCreateTextureFromFile( device, wsFindResource(L"Media\\springMarker.dds").c_str(), &m_springMarkerTexture ) );

   V( device->CreateTexture( 1, 1, 1, 0, D3DFMT_A32B32G32R32F, D3DPOOL_SYSTEMMEM, &m_waterDepthSamplerSysMem, NULL ) );
   //V( device->CreateTexture( 1, 1, 1, 0, D3DFMT_G16R16F, D3DPOOL_SYSTEMMEM, &m_waterVelocitySamplerSysMem, NULL ) );

   return S_OK;
}

HRESULT wsSimulator::OnResetDevice( const D3DSURFACE_DESC * pBackBufferSurfaceDesc )
{
   m_BackBufferSurfaceDesc = *pBackBufferSurfaceDesc;

   CenterDisplay();

   return S_OK;
}

void wsSimulator::OnLostDevice( )
{
   memset( &m_BackBufferSurfaceDesc, 0, sizeof(m_BackBufferSurfaceDesc) );

   SaveToWatermapFile( );

   SAFE_RELEASE( m_waterDepthTextureFront );
   SAFE_RELEASE( m_waterDepthTextureBack );
   SAFE_RELEASE( m_waterVelocityTextureFront );
   SAFE_RELEASE( m_waterVelocityTextureBack );
   SAFE_RELEASE( m_waterDepthSampler );
   //SAFE_RELEASE( m_waterVelocitySampler );
   SAFE_RELEASE( m_springMapTexture );
}

void wsSimulator::OnDestroyDevice( )
{
   SAFE_RELEASE( m_heightmapTexture );

   SAFE_RELEASE( m_springMarkerTexture );

   SAFE_RELEASE( m_waterDepthSamplerSysMem );
   //SAFE_RELEASE( m_waterVelocitySamplerSysMem );
}
//
void wsSimulator::SetupSimulationPass( ID3DXConstantTable * constantTable, int tick )
{
   wsSetTexture( m_heightmapTexture, "g_heightmapTexture", constantTable );
   wsSetTexture( m_waterDepthTextureFront, "g_depthmapTexture", constantTable );
   wsSetTexture( m_waterVelocityTextureFront, "g_velocitymapTexture", constantTable );
   wsSetTexture( m_springMapTexture, "g_springmapTexture", constantTable );

   wsSetRenderTarget( 0, m_waterDepthTextureBack );
   wsSetRenderTarget( 1, m_waterVelocityTextureBack );

   // HRESULT hr;

   IDirect3DDevice9 * device = GetD3DDevice();

   int currPrecTick = tick;

   float prec = (currPrecTick < m_simSettings.PrecipitationTicks)?(m_simSettings.Precipitation):(-m_simSettings.Evaporation);
   //prec *= m_simSettings.DepthValueMultiplier;
   
   constantTable->SetFloat( device, "g_springBoost",              (m_springBoostMode)?(10.0f):(1.0f) );
   constantTable->SetFloat( device, "g_absorption",               m_simSettings.Absorption );
   constantTable->SetFloat( device, "g_precipitation",            prec );
   constantTable->SetFloat( device, "g_linVelocityDamping",       m_simSettings.LinVelocityDamping );
   constantTable->SetFloat( device, "g_sqrVelocityDamping",       m_simSettings.SqrVelocityDamping );
   constantTable->SetFloat( device, "g_diffusion",                m_simSettings.Diffusion );
   constantTable->SetFloat( device, "g_acceleration",             m_simSettings.Acceleration );

   constantTable->SetFloat( device, "g_depthBasedFrictionMinDepth",  m_simSettings.DepthBasedFrictionMinDepth );
   constantTable->SetFloat( device, "g_depthBasedFrictionMaxDepth",  m_simSettings.DepthBasedFrictionMaxDepth );
   constantTable->SetFloat( device, "g_depthBasedFrictionAmount",    m_simSettings.DepthBasedFrictionAmount );

   //constantTable->SetFloat( device, "g_zeroVelocityBelowDepth",   m_simSettings.ZeroVelocityBelowDepth * m_simSettings.DepthValueMultiplier);
   //constantTable->SetFloat( device, "g_shallowLinVelDampDepth",   m_simSettings.ShallowLinVelDampDepth * m_simSettings.DepthValueMultiplier);
   //constantTable->SetFloat( device, "g_shallowLinVelDamping",     m_simSettings.ShallowLinVelDamping);

   constantTable->SetFloat( device, "g_worldZSize",              m_mapDims.SizeZ );

   float v[2];
   
   v[0] = 1.0f / (float)m_rasterWidth;    v[1] = 1.0f / (float)m_rasterHeight;
   constantTable->SetFloatArray( device, "g_samplerPixelSize", v, 2 );
   
   // v[0] = (float)m_rasterWidth;           v[1] = (float)m_rasterHeight;
   // V( constantTable->SetFloatArray( device, "g_samplerPixelDims", v, 2 ) );
}
//
void wsSimulator::CenterDisplay( )
{
   if( m_BackBufferSurfaceDesc.Height == 0 || m_rasterWidth == 0 )
      return;

   int totalHeight  = m_BackBufferSurfaceDesc.Height - c_renderTopBorder - c_renderBorder * 2;
   int totalWidth   = m_BackBufferSurfaceDesc.Width - c_renderBorder * 2;
   m_displayScale    = ::min( totalHeight / (float)m_rasterHeight, totalWidth / (float)m_rasterWidth );

   int renderWidth    = (int)(m_rasterWidth * m_displayScale);
   int renderHeight   = (int)(m_rasterHeight * m_displayScale);

   m_displayOffsetX  = (m_BackBufferSurfaceDesc.Width - renderWidth) / 2;
   m_displayOffsetY  = (m_BackBufferSurfaceDesc.Height - renderHeight - c_renderTopBorder) / 2 + c_renderTopBorder;
}
//
void wsSimulator::LimitDisplay( )
{

}
//
void wsSimulator::Tick( float deltaTime )
{
   if( IsKeyClicked(kcReloadAll) )
   {
      InitializeFromIni();

      OnDestroyDevice();
      OnCreateDevice();
   }

   if( IsKeyClicked(kcSpringBoostMode) )
   {
      m_springBoostMode = !m_springBoostMode;
   }

   if( IsKeyClicked( kcToggleShowVelocitiesOrDepth ) )
      m_displayVelocities = !m_displayVelocities;

   m_time += deltaTime;
   m_simTickCount++;

   LimitDisplay();

   //int totalHeight  = m_BackBufferSurfaceDesc.Height - c_renderTopBorder - c_renderBorder * 2;
   //int totalWidth   = m_BackBufferSurfaceDesc.Width - c_renderBorder * 2;

   int renderWidth    = (int)(m_rasterWidth * m_displayScale);
   int renderHeight   = (int)(m_rasterHeight * m_displayScale);

   m_mouseU = (m_mouseClientX - m_displayOffsetX) / (float)renderWidth;
   m_mouseV = (m_mouseClientY - m_displayOffsetY) / (float)renderHeight;

   if( m_depthSumDisplay )
      GetCanvas2D()->DrawText( 6, m_BackBufferSurfaceDesc.Height - 36, L"total water depth sum: (%.4lf)", m_depthSum );

   GetCanvas2D()->DrawText( 6, m_BackBufferSurfaceDesc.Height - 20, L"mouse pos (%.4f, %.4f) : velocity (%.4f, %.4f), depth (%.6f), terrain height (%.1f), terrain+water height (%.6f)", m_mouseU, m_mouseV, m_mouseSampledWaterVelocityX, m_mouseSampledWaterVelocityY, m_mouseSampledWaterDepth, m_mouseSampledTerrainHeight, m_mouseSampledTerrainHeight + m_mouseSampledWaterDepth );

   if( m_springBoostMode )
      GetCanvas2D()->DrawText( 6, m_BackBufferSurfaceDesc.Height - 52, L"SPRING 10x BOOST MODE ACTIVE" );

   if( deltaTime != 0.0f )
   {
      IDirect3DDevice9 * device = GetD3DDevice();

      HRESULT hr;
      V( device->BeginScene() );

      IDirect3DSurface9 * oldRenderTarget;
      IDirect3DSurface9 * oldDepthStencil;
      device->GetRenderTarget( 0, &oldRenderTarget );
      device->GetDepthStencilSurface( &oldDepthStencil );
      device->SetDepthStencilSurface( NULL );

      InitializeTextures();

      if( m_mouseLeftButtonDown )
      {
         V( device->SetPixelShader( m_psSimulationSplash ) );

         SetupSimulationPass(m_psSimulationSplash.GetConstantTable(), 0);

         m_psSimulationSplash.SetFloatArray( "g_splashUVPos", m_mouseU, m_mouseV );
         m_psSimulationSplash.SetFloat( "g_splashIntensity", 0.5f );
         m_psSimulationSplash.SetFloat( "g_splashRadius", 0.005f );

         wsRenderSimulation( m_rasterWidth, m_rasterHeight, 1 );

         ::swap( m_waterDepthTextureFront, m_waterDepthTextureBack );
         ::swap( m_waterVelocityTextureFront, m_waterVelocityTextureBack );

         m_psSimulationSplash.SetTexture( "g_heightmapTexture", NULL );
         m_psSimulationSplash.SetTexture( "g_depthmapTexture", NULL );
         m_psSimulationSplash.SetTexture( "g_velocitymapTexture", NULL );
      }

      int currPrecTick = m_simSettings.PrecipitationTicks + m_simSettings.EvaporationTicks;

      //Sleep( 0 );
      for( int tick = 0; tick < currPrecTick; tick++ )
      {
         /////////////////////////////////////////////////////////////////////////
         //
         V( device->SetPixelShader( m_psSimulationVelocityPropagateStep ) );
         //
         SetupSimulationPass( m_psSimulationVelocityPropagateStep.GetConstantTable(), tick );
         //
         wsRenderSimulation( m_rasterWidth, m_rasterHeight, 1 );
         //
         ::swap( m_waterDepthTextureFront, m_waterDepthTextureBack );
         ::swap( m_waterVelocityTextureFront, m_waterVelocityTextureBack );
         //
         m_psSimulationVelocityPropagateStep.SetTexture( "g_heightmapTexture", NULL );
         m_psSimulationVelocityPropagateStep.SetTexture( "g_depthmapTexture", NULL );
         m_psSimulationVelocityPropagateStep.SetTexture( "g_velocitymapTexture", NULL );
         //
         /////////////////////////////////////////////////////////////////////////
         //
         V( device->SetPixelShader( m_psSimulationDiffuseAndTheRest ) );
         //
         SetupSimulationPass( m_psSimulationDiffuseAndTheRest.GetConstantTable(), tick );
         wsRenderSimulation( m_rasterWidth, m_rasterHeight, 1 );
         ::swap( m_waterDepthTextureFront, m_waterDepthTextureBack );
         ::swap( m_waterVelocityTextureFront, m_waterVelocityTextureBack );
         //
         m_psSimulationDiffuseAndTheRest.SetTexture( "g_heightmapTexture", NULL );
         m_psSimulationDiffuseAndTheRest.SetTexture( "g_depthmapTexture", NULL );
         m_psSimulationDiffuseAndTheRest.SetTexture( "g_velocitymapTexture", NULL );
         //
         /////////////////////////////////////////////////////////////////////////
         //
         // don't use this for simplicity sake....
         const int diffuseSteps = 0;
         for( int d = 0; d < diffuseSteps; d++ )
         {
            V( device->SetPixelShader( m_psSimulationDiffuseOnly ) );
            //
            SetupSimulationPass( m_psSimulationDiffuseOnly.GetConstantTable(), tick );
            wsRenderSimulation( m_rasterWidth, m_rasterHeight, 1 );
            ::swap( m_waterDepthTextureFront, m_waterDepthTextureBack );
            ::swap( m_waterVelocityTextureFront, m_waterVelocityTextureBack );
            //
            m_psSimulationDiffuseOnly.SetTexture( "g_heightmapTexture", NULL );
            m_psSimulationDiffuseOnly.SetTexture( "g_depthmapTexture", NULL );
            m_psSimulationDiffuseOnly.SetTexture( "g_velocitymapTexture", NULL );
         }
         //
         /////////////////////////////////////////////////////////////////////////
      }

      if( m_simTickCount % 8 == 0 )
      {
         static wsPixelShader psSampleSimPixel( "wsSimulation.psh", "sampleSimPixel" );

         V( device->SetPixelShader( psSampleSimPixel ) );

         V( psSampleSimPixel.SetFloatArray( "g_simPixelSampleUV", m_mouseU, m_mouseV ) );
         V( psSampleSimPixel.SetFloat( "g_worldZSize", m_mapDims.SizeZ ) );

         V( psSampleSimPixel.SetTexture( "g_heightmapTexture", m_heightmapTexture ) );
         V( psSampleSimPixel.SetTexture( "g_depthmapTexture", m_waterDepthTextureFront ) );
         V( psSampleSimPixel.SetTexture( "g_velocitymapTexture", m_waterVelocityTextureFront ) );

         wsSetRenderTarget( 0, m_waterDepthSampler );
         //wsSetRenderTarget( 1, m_waterVelocitySampler );
         V( device->SetRenderTarget( 1, NULL ) );

         wsRenderSimulation( 1, 1, 0 );

         V( psSampleSimPixel.SetTexture( "g_heightmapTexture", NULL ) );
         V( psSampleSimPixel.SetTexture( "g_depthmapTexture", NULL ) );
         V( psSampleSimPixel.SetTexture( "g_velocitymapTexture", NULL ) );

         ExtractMouseSampleData( m_mouseSampledWaterDepth, m_mouseSampledWaterVelocityX, m_mouseSampledWaterVelocityY, m_mouseSampledTerrainHeight );
      }

      if( IsKeyClicked(kcToggleCalculateTotalWaterSum) )
      {
         m_depthSumDisplay = !m_depthSumDisplay;
         m_depthSum = 0.0f;
      }

      if( m_depthSumDisplay && (m_simTickCount % 100 == 0) )
      {
         ExtractTotalDepthSum( m_depthSum );
      }

      device->SetPixelShader( NULL );

      V( device->SetRenderTarget( 0, oldRenderTarget ) );
      V( device->SetRenderTarget( 1, NULL ) );
      V( device->SetRenderTarget( 2, NULL ) );
      V( device->SetRenderTarget( 3, NULL ) );

      device->SetDepthStencilSurface( oldDepthStencil );

      SAFE_RELEASE( oldRenderTarget );
      SAFE_RELEASE( oldDepthStencil );

      V( device->EndScene() );
   }

   if( IsKeyClicked(kcReloadAll) )
   {
      SAFE_RELEASE( m_springMapTexture );
   }
}

static void RenderToScreen( int fromX, int fromY, int width, int height )
{
   IDirect3DDevice9* device = DxEventNotifier::GetD3DDevice();

   TransofmedTexturedVertex vertices[4];

   float uminx = 0; //viewOffX / (float)(twidth+1);
   float uminy = 0; //viewOffY / (float)(theight+1);
   float umaxx = 1; //uminx + aspx;
   float umaxy = 1; //uminy + aspy;

   vertices[0] = TransofmedTexturedVertex( -0.5f, -0.5f,                0.5f, 1.0f, uminx, uminy );
   vertices[1] = TransofmedTexturedVertex( width - 0.5f, -0.5f,         0.5f, 1.0f, umaxx, uminy );
   vertices[2] = TransofmedTexturedVertex( -0.5f, height - 0.5f,        0.5f, 1.0f, uminx, umaxy );
   vertices[3] = TransofmedTexturedVertex( width - 0.5f, height - 0.5f, 0.5f, 1.0f, umaxx, umaxy );

   for( int i = 0; i < 4; i++ )
   {
      vertices[i].x += fromX;
      vertices[i].y += fromY;
   }

   device->SetRenderState( D3DRS_ZENABLE, FALSE );
   device->SetFVF( TransofmedTexturedVertex::FVF );

   device->SetRenderState( D3DRS_ZENABLE, FALSE );
   device->SetRenderState( D3DRS_LIGHTING, FALSE );

   device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, vertices, sizeof( TransofmedTexturedVertex ) );

   device->SetRenderState( D3DRS_LIGHTING, FALSE );
   device->SetRenderState( D3DRS_ZENABLE, TRUE );
}

static wsPixelShader renderTextureToScreenShader( "wsSimulationDisplay.psh", "textureToScreen" );

static void RenderTextureToScreenBegin( bool alpha, IDirect3DTexture9 * texture )
{
   IDirect3DDevice9* device = DxEventNotifier::GetD3DDevice();

   device->SetRenderState( D3DRS_ZENABLE, FALSE );
   device->SetFVF( TransofmedTexturedVertex::FVF );

   device->SetRenderState( D3DRS_ZENABLE, FALSE );
   device->SetRenderState( D3DRS_LIGHTING, FALSE );

   if( alpha )
   {
      device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
      device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
      device->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
   }

   device->SetPixelShader( renderTextureToScreenShader );

   renderTextureToScreenShader.SetTexture( "g_inputTexture", texture, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
}

static void U32_TO_FOURFLOATCOLORS( unsigned int c, float v[4] )
{
   v[0] = ( ( c >> 16 ) & 0xFF ) / 255.0f;
   v[1] = ( ( c >> 8 ) & 0xFF ) / 255.0f;
   v[2] = ( c & 0xFF ) / 255.0f;
   v[3] = ( ( c >> 24 ) & 0xFF ) / 255.0f;
}

static void RenderTextureToScreenDo( float fromX, float fromY, float width, float height, unsigned int color )
{
   IDirect3DDevice9* device = DxEventNotifier::GetD3DDevice();

   TransofmedTexturedVertex vertices[4];

   float uminx = 0; //viewOffX / (float)(twidth+1);
   float uminy = 0; //viewOffY / (float)(theight+1);
   float umaxx = 1; //uminx + aspx;
   float umaxy = 1; //uminy + aspy;

   vertices[0] = TransofmedTexturedVertex( -0.5f, -0.5f,                0.5f, 1.0f, uminx, uminy );
   vertices[1] = TransofmedTexturedVertex( width - 0.5f, -0.5f,         0.5f, 1.0f, umaxx, uminy );
   vertices[2] = TransofmedTexturedVertex( -0.5f, height - 0.5f,        0.5f, 1.0f, uminx, umaxy );
   vertices[3] = TransofmedTexturedVertex( width - 0.5f, height - 0.5f, 0.5f, 1.0f, umaxx, umaxy );

   for( int i = 0; i < 4; i++ )
   {
      vertices[i].x += fromX;
      vertices[i].y += fromY;
   }

   float v[4]; U32_TO_FOURFLOATCOLORS( color, v );
   renderTextureToScreenShader.SetFloatArray( "g_inputColor", v, 4 );

   device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, vertices, sizeof( TransofmedTexturedVertex ) );
}

static void RenderTextureToScreenEnd( )
{
   IDirect3DDevice9* device = DxEventNotifier::GetD3DDevice();

   device->SetRenderState( D3DRS_LIGHTING, FALSE );
   device->SetRenderState( D3DRS_ZENABLE, TRUE );

   device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
   device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
   device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
}

HRESULT wsSimulator::Render( )
{
   HRESULT hr;

   IDirect3DDevice9* device = GetD3DDevice();

   int renderWidth    = (int)(m_rasterWidth * m_displayScale);
   int renderHeight   = (int)(m_rasterHeight * m_displayScale);

   device->SetPixelShader( m_psSimulationDisplay );

   m_psSimulationDisplay.SetTexture( "g_heightmapTexture", m_heightmapTexture );
   m_psSimulationDisplay.SetTexture( "g_depthmapTexture", m_waterDepthTextureFront );
   m_psSimulationDisplay.SetTexture( "g_velocitymapTexture", m_waterVelocityTextureFront );
   m_psSimulationDisplay.SetTexture( "g_heightmapLinFilteredTexture", m_heightmapTexture, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR );
   m_psSimulationDisplay.SetTexture( "g_depthmapLinFilteredTexture", m_waterDepthTextureFront, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR );

   V( m_psSimulationDisplay.SetBool( "g_displayVelocities", m_displayVelocities ) );
   V( m_psSimulationDisplay.SetFloat( "g_waterIgnoreBelowDepth", m_waterIgnoreBelowDepth ) );

   m_psSimulationDisplay.SetFloat( "g_worldZSize", m_mapDims.SizeZ );
   m_psSimulationDisplay.SetFloat( "g_currentZoom", m_displayScale );

   RenderToScreen( m_displayOffsetX, m_displayOffsetY, renderWidth, renderHeight );

   m_psSimulationDisplay.SetTexture( "g_depthmapTexture", NULL );
   m_psSimulationDisplay.SetTexture( "g_velocitymapTexture", NULL );
   m_psSimulationDisplay.SetTexture( "g_heightmapTexture", NULL );
   m_psSimulationDisplay.SetTexture( "g_heightmapLinFilteredTexture", NULL );
   m_psSimulationDisplay.SetTexture( "g_depthmapLinFilteredTexture", NULL );

   //m_psSimulationDisplay.SetFloatArray( "g_simPixelSampleUV", m_mouseU, m_mouseV );

   RenderTextureToScreenBegin( true, m_springMarkerTexture );

   float period = 10.0;
   int alpha   = clamp( (int)((sin( m_time * period ) * 2 + 0.1f ) * 255), 0, 255 );
   int alphaCur= clamp( (int)((sin( m_time * period * 1.5f ) ) * 128), 0, 128 ) + 127;
   int col     = clamp( (int)((sin( m_time * period - PI * 0.5 ) * 2 ) * 255), 0, 255 );

   const vector<wsSpringsMgr::Spring> & springs = wsSpringsMgr::Instance().GetSprings();
   for( size_t i = 0; i < springs.size(); i++ )
   {
      const wsSpringsMgr::Spring & spring = springs[i];

      float springX = floorf( spring.x * m_rasterWidth + 0.5f ) / (float)m_rasterWidth;
      float springY = floorf( spring.y * m_rasterHeight + 0.5f ) / (float)m_rasterHeight;

      float r = spring.radius * m_displayScale + 1.0f;
      float x = (springX) * renderWidth + m_displayOffsetX - r;
      float y = (springY) * renderHeight + m_displayOffsetY - r;

      RenderTextureToScreenDo( x, y, r*2, r*2, (alpha << 24) | (col << 16) /*| (col << 8)*/  );

      if( r > 7.0f && (x >= 0) && (x <= m_BackBufferSurfaceDesc.Width) && (y >= 0) && (y <= m_BackBufferSurfaceDesc.Height) )
      {
         GetCanvas2D()->DrawText( (int)x-120, (int)y-16, L"Spring no %02d, water quantity: %.3f, radius: %.1f", i, spring.quantity, spring.radius );
      }
   }

   {
      float mouseU = floorf( m_mouseU * m_rasterWidth ) / (float)m_rasterWidth;
      float mouseV = floorf( m_mouseV * m_rasterHeight ) / (float)m_rasterHeight;
      float x = (mouseU) * renderWidth + m_displayOffsetX;
      float y = (mouseV) * renderHeight + m_displayOffsetY;
      RenderTextureToScreenDo( x - 0.0f * m_displayScale, y - 0.0f * m_displayScale, m_displayScale, m_displayScale, (alphaCur << 24) | (col << 8) | (col << 0)  );

      float vx    = m_mouseSampledWaterVelocityX * 2;
      float vy    = m_mouseSampledWaterVelocityY * 2;
      float spd   = sqrtf( vx*vx + vy*vy );
      if( spd != 0 )
      {
         float nvx = vx / spd * 2;
         float nvy = vy / spd * 2;
         GetCanvas2D()->DrawLine( x + 0.5f * m_displayScale, y + 0.5f * m_displayScale, x + nvx * m_displayScale + 0.5f * m_displayScale, y + nvy * m_displayScale + 0.5f * m_displayScale, 0x80808080 );
         GetCanvas2D()->DrawLine( x + 0.5f * m_displayScale, y + 0.5f * m_displayScale, x + vx * m_displayScale + 0.5f * m_displayScale, y + vy * m_displayScale + 0.5f * m_displayScale, 0xFFFF4040 );
      }

   }

   RenderTextureToScreenEnd( );

   device->SetPixelShader( NULL );

   return S_OK;
}
//
void wsSimulator::MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing )
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
      case WM_RBUTTONDBLCLK:
         m_mouseRightButtonDown = false;
         *pbNoFurtherProcessing = true;
         CenterDisplay();
         break;
      case WM_RBUTTONDOWN:
         m_mouseRightButtonDown = true;
         *pbNoFurtherProcessing = true;
         m_mouseClientX = ( short )LOWORD( lParam );
         m_mouseClientY = ( short )HIWORD( lParam );
         break;
      case WM_RBUTTONUP:
         m_mouseRightButtonDown = false;
         *pbNoFurtherProcessing = true;
         break;
      case WM_MOUSEMOVE:
         if( m_mouseRightButtonDown )
         {
            int deltaX = (( short )LOWORD( lParam )) - m_mouseClientX;
            int deltaY = (( short )HIWORD( lParam )) - m_mouseClientY;
            m_displayOffsetX += deltaX;
            m_displayOffsetY += deltaY;
         }
         m_mouseClientX = ( short )LOWORD( lParam );
         m_mouseClientY = ( short )HIWORD( lParam );
         break;
      case WM_KILLFOCUS:
         m_mouseLeftButtonDown = false;
         m_mouseRightButtonDown = false;
         break;
      case WM_MOUSEWHEEL:
         int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
         float prevScale = m_displayScale;

         m_displayScale *= (zDelta > 0)?(1.1f):(0.9f);

         m_displayScale = clamp( m_displayScale, 0.1f, 20.0f );
         
         m_displayOffsetX = (int)(m_mouseClientX - ((m_displayScale)*(m_mouseClientX - m_displayOffsetX)) / (prevScale));
         m_displayOffsetY = (int)(m_mouseClientY - ((m_displayScale)*(m_mouseClientY - m_displayOffsetY)) / (prevScale));

         break;
   }
}
//
