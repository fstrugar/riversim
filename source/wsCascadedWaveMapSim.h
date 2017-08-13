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

#pragma warning (disable : 4995)

#include "DxEventNotifier.h"
#include "wsCommon.h"

#include "wsShader.h"

#include "TiledBitmap/TiledBitmap.h"

#include "wsAABB.h"

class ws3DPreviewCamera;

class IWaterSurfaceInfoProvider
{
protected:
   virtual ~IWaterSurfaceInfoProvider()   {}

public:
   virtual void      GetAreaMinMaxHeight( float fromX, float fromY, float sizeX, float sizeY, float & minZ, float & maxZ )          = 0;
   virtual void      UpdateVelocityTexture( float fromX, float fromY, float sizeX, float sizeY, float minZ, float maxZ, IDirect3DTexture9 * velocityMap )   = 0;
};

// Maybe try using lower dim (0.5 * TextureResolution) for foam?

#define WAVE_VELOCITY_MAP_TEXTURE_FORMAT     (D3DFMT_A8R8G8B8)       // R is x, G is y, B is preturb, A is hasWater
#define WAVE_SIM_TEXTURE_FORMAT              (D3DFMT_G16R16)         // both channels used; lower precision unfortunately not acceptable
#define WAVE_FOAM_TEXTURE_FORMAT             (D3DFMT_R16F)           // only .x used
#define WAVE_FOAM_TEXTURE_FORMAT_2           (D3DFMT_G16R16F)        // second option in case format above is not supported

#define WAVE_COMBINED_USE_AUTOMIPMAP (0)
#define WAVE_COMBINED_HEIGHTS_TEXTURE_FORMAT (D3DFMT_G16R16)         // if a graphics card is unable to filtered vertex-fetch from D3DFMT_G16R16 then use D3DFMT_G32R32F
#define WAVE_COMBINED_H_NORM_TEXTURE_FORMAT  (D3DFMT_A8R8G8B8)
#define WAVE_COMBINED_FOAM_TEXTURE_FORMAT    (D3DFMT_G16R16)

class wsCascadedWaveMapSim : protected DxEventReceiver
{
   friend class ws3DPreviewRenderer;

public:
   struct WaveMapLayerInfo
   {
      bool                    OutOfRange;
      IDirect3DTexture9 *     CombinedHeightMap;
      IDirect3DTexture9 *     CombinedHNormMap;
      IDirect3DTexture9 *     CombinedFoamMap;
      IDirect3DTexture9 *     WaterVelocityMap;
      wsAABB                  Aabb;
      float                   FadeOutFrom;
      float                   FadeOutTo;
      float                   TimeLerpK;
      float                   DetailUVDiv;
      float                   CombinedHeightMapAverage;
   };

private:
   struct SimpleSplash
   {
      float             wmU;
      float             wmV;
      float             wmRadius;

      SimpleSplash()    {}
      SimpleSplash( float u, float v, float radius ) : wmU(u), wmV(v), wmRadius( radius ) {}
   };

   class WaveMapLayer
   {

   public:
      IDirect3DTexture9 *                       m_waveMap;
      IDirect3DTexture9 *                       m_foamMap;

      IDirect3DTexture9 *                       m_combinedHeights;
      IDirect3DTexture9 *                       m_combinedHNormals;
      IDirect3DTexture9 *                       m_combinedFoam;

      IDirect3DTexture9 *                       m_velocityMap;
      //IDirect3DTexture9 *                       m_displayNormalMap;

      int                                       m_resolution;

      int                                       m_simTickCount;

      int                                       m_level;

      float                                     m_worldPosX;
      float                                     m_worldPosY;
      float                                     m_worldSize;

      float                                     m_worldMinZ;
      float                                     m_worldMaxZ;
      
      float                                     m_visibleRange;
      bool                                      m_outOfZRange;

      float                                     m_simTimeAccomulator;
      float                                     m_simTimeStep;
      float                                     m_waterSpeedMul;

      float                                     m_repositionOffsetUVX;
      float                                     m_repositionOffsetUVY;

      float                                     m_randUVX;
      float                                     m_randUVY;

      float                                     m_combinedHeightsMapAverage;

      std::vector<SimpleSplash>                 m_accumulatedSplashes;

      WaveMapLayer( ) { assert( false ); }

   public:
      WaveMapLayer( IDirect3DDevice9 * device, int resolution, float worldSize, float simTimeStep, float waterSpeedMul, int level, float combinedHeightsMapAverage );
      ~WaveMapLayer( );

      bool                             RepositionIfNeeded( ws3DPreviewCamera * camera );

      inline wsAABB                    GetAabb()   { return wsAABB( D3DXVECTOR3( m_worldPosX, m_worldPosY, m_worldMinZ - m_worldSize ), D3DXVECTOR3( m_worldPosX + m_worldSize, m_worldPosY + m_worldSize, m_worldMaxZ + m_worldSize ) ); }
   };

   struct Settings
   {
      int                              LayerCount;
      int                              TextureResolution;
      float                            FirstLayerWorldSize;
      float                            FirstLayerSimTimeStepHz;
      float                            FirstLayerWaterSpeedMul;
      int                              LayerDeltaScale;
      //int                              detailWaveTextureResolution;
   };

private:

   MapDimensions                             m_mapDims;
   Settings                                  m_settings;

   vector<WaveMapLayer*>                     m_waveMapLayers;

   IDirect3DTexture9 *                       m_waveMapTempTexture;
   IDirect3DTexture9 *                       m_foamSimTempTexture;

   IDirect3DTexture9 *                       m_combinedHeightsTemp;
   IDirect3DTexture9 *                       m_combinedHNormTemp;
   IDirect3DTexture9 *                       m_combinedFoamTemp;

   IWaterSurfaceInfoProvider *               m_waterSurfaceInfoProvider;

   wsPixelShader                             m_shaderWaveSim;
   wsPixelShader                             m_shaderSimpleFlow;
   wsPixelShader                             m_shaderUpdateDefaultValuesTexture;
   wsPixelShader                             m_shaderRepositionOnly;
   wsPixelShader                             m_shaderUpdateCombinedHeights;
   wsPixelShader                             m_shaderUpdateWaveDetailMap;

   wsPixelShader                             m_shaderFoamSim;
   wsPixelShader                             m_shaderUpdateCombinedFoam;

   IDirect3DTexture9 *                       m_noiseTexture1;
   IDirect3DTexture9 *                       m_noiseTexture2;

   bool                                      m_firstTick;

   int                                       m_ticksFromLastReposition;

   double                                    m_time;

   //IDirect3DTexture9 *                       m_waveDetailMapTexture;

   std::vector<IDirect3DTexture9 * >         m_autoDeleteOnLostDeviceList;

public:
   wsCascadedWaveMapSim(void);
   virtual ~wsCascadedWaveMapSim(void);
   //
public:
   void                          Start( const MapDimensions & mapDims, IWaterSurfaceInfoProvider * waterSurfaceInfoProvider );
   void                          Stop( );
   //
   void                          InitializeFromIni( );
   //
   void                          Tick( float deltaTime, ws3DPreviewCamera * camera );
   //
   int                           GetLayerCount()                              { return (int)m_waveMapLayers.size(); }
   void                          GetLayerInfo( int index, WaveMapLayerInfo & info );
   //
   int                           GetTextureResolution()                       { return m_settings.TextureResolution; }
   float                         GetLayerDeltaScale()                         { return (float)m_settings.LayerDeltaScale; }
   //
   void                          DoTestSplash( const D3DXVECTOR3 & pos, float radius, float intensity );
   //
   //IDirect3DTexture9 *           GetWaveDetailMapTexture()                    { return m_waveDetailMapTexture; }
   //int                           GetdetailWaveTextureResolution()                 { return m_settings.detailWaveTextureResolution; }
   //
private:
   //
   virtual void                  OnLostDevice();
   virtual void                  OnDestroyDevice();
   virtual HRESULT 				   OnCreateDevice();
   virtual HRESULT               OnResetDevice( const D3DSURFACE_DESC * pBackBufferSurfaceDesc );
   //
   void                          SimulateLayer( WaveMapLayer & layer, float deltaTime, WaveMapLayer * lessDetailedLayer );
   void                          UpdateCombinedTexture( WaveMapLayer & layer, WaveMapLayer * lessDetailedLayer, wsPixelShader & shader, IDirect3DTexture9 * srcTexture, IDirect3DTexture9 * srcCombinedLoTexture, IDirect3DTexture9 * destTexture );
   void                          RepositionLayer( WaveMapLayer & layer, IDirect3DTexture9 * srcTexture, IDirect3DTexture9 * destTexture, int ignoreBorderWidth );
   void                          UpdateSimpleFlow( WaveMapLayer & layer, IDirect3DTexture9 * srcTexture, IDirect3DTexture9 * destTexture );
   //
   void                          UpdateWaveDetailMap( );
   //
   void                          InitializeTextures();
   //wsTexSplattingMgr &             GetwsTexSplattingMgr()                      { return m_texSplattingMgr; }
   //
};
