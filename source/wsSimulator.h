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

#include "TiledBitmap/TiledBitmap.h"

#include "wsShader.h"

class wsSimulator : protected DxEventReceiver
{
private:

   wsPixelShader                             m_psSimulationDisplay;
   wsPixelShader                             m_psSimulationDiffuseOnly;
   wsPixelShader                             m_psSimulationDiffuseAndTheRest;
   wsPixelShader                             m_psSimulationVelocityPropagateStep;
   wsPixelShader                             m_psSimulationSplash;

   // Input/output storage for the simulation state
   AdVantage::TiledBitmap *                  m_heightmap;
   AdVantage::TiledBitmap *                  m_watermap;

   // Terrain on which the simulation is done
   IDirect3DTexture9 *                       m_heightmapTexture;

   // Used to know where to add water at runtime
   IDirect3DTexture9 *                       m_springMapTexture;

   // Main simulation data
   IDirect3DTexture9 *                       m_waterDepthTextureFront;
   IDirect3DTexture9 *                       m_waterDepthTextureBack;
   IDirect3DTexture9 *                       m_waterVelocityTextureFront;
   IDirect3DTexture9 *                       m_waterVelocityTextureBack;

   // These are used to sample one pixel of simulation info below the mouse pointer
   IDirect3DTexture9 *                       m_waterDepthSampler;
   //IDirect3DTexture9 *                       m_waterVelocitySampler;
   IDirect3DTexture9 *                       m_waterDepthSamplerSysMem;
   //IDirect3DTexture9 *                       m_waterVelocitySamplerSysMem;

   int                                       m_rasterWidth;
   int                                       m_rasterHeight;

   D3DSURFACE_DESC                           m_BackBufferSurfaceDesc;

   IDirect3DTexture9 *                       m_springMarkerTexture;

   float                                     m_displayScale;
   int                                       m_displayOffsetX;
   int                                       m_displayOffsetY;

   bool                                      m_mouseLeftButtonDown;
   bool                                      m_mouseRightButtonDown;
   int                                       m_mouseClientX;
   int                                       m_mouseClientY;
   float                                     m_mouseU;
   float                                     m_mouseV;

   float                                     m_mouseSampledWaterDepth;
   float                                     m_mouseSampledWaterVelocityX;
   float                                     m_mouseSampledWaterVelocityY;
   float                                     m_mouseSampledTerrainHeight;

   bool                                      m_depthSumDisplay;
   double                                    m_depthSum;

   bool                                      m_springBoostMode;

   double                                    m_time;

   int                                       m_simTickCount;

   bool                                      m_displayVelocities;

   MapDimensions                             m_mapDims;
   float                                     m_waterIgnoreBelowDepth;   // in word (MapDims) space!

   struct SimulationSettings
   {
      float          Absorption;
      float          Precipitation;
      int            PrecipitationTicks;
      float          Evaporation;
      int            EvaporationTicks;
      float          LinVelocityDamping;
      float          SqrVelocityDamping;
      float          Diffusion;
      float          Acceleration;

      float          DepthBasedFrictionMinDepth;
      float          DepthBasedFrictionMaxDepth;
      float          DepthBasedFrictionAmount;

      //float          DepthValueMultiplier;
      //float          ZeroVelocityBelowDepth;
      //float          DampenVelocityBelowDepth;
      //float          DampenVelocityBelowDepthAmount;
      //float          ShallowLinVelDampDepth;
      //float          ShallowLinVelDamping;
      //float        WindShake;

      SimulationSettings()
      {
         //DepthValueMultiplier    = 1.0f / 65535.0f;

         Absorption                 = 1.0f;
         Precipitation              = 0.0f;
         PrecipitationTicks         = 1;
         Evaporation                = 0.00f;
         EvaporationTicks           = 0;
         LinVelocityDamping         = 0.99f;
         SqrVelocityDamping         = 0.05f;
         Diffusion                  = 0.06f;
         Acceleration               = 0.4f;

         DepthBasedFrictionMinDepth = 0.0f;
         DepthBasedFrictionMaxDepth = 1.0f;
         DepthBasedFrictionAmount   = 0.9f;

         //WindShake               = 0.00003f;
      }
   };

   SimulationSettings            m_simSettings;

public:
   wsSimulator(void);
   virtual ~wsSimulator(void);
   //
public:
   void                          Start( AdVantage::TiledBitmap * heightmap, AdVantage::TiledBitmap * watermap, const MapDimensions & mapDims );
   void                          Stop( );
   //
   void                          Tick( float deltaTime );
   HRESULT                       Render( );
   //
   void                          MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing );
   //
   virtual HRESULT 				   OnCreateDevice();
   virtual HRESULT               OnResetDevice( const D3DSURFACE_DESC * pBackBufferSurfaceDesc );
   //
protected:
   //
   void                          InitializeFromIni();
   //
   void                          LoadFromWatermapFile();
   void                          SaveToWatermapFile();
   //
   void                          InitializeTextures();
   //
   virtual void                  OnLostDevice( );
   virtual void                  OnDestroyDevice( );
   //
   void                          SetupSimulationPass( ID3DXConstantTable * constantTable, int tick );
   //
   void                          CenterDisplay( );
   void                          LimitDisplay( );
   //
   void                          ExtractMouseSampleData( float & depth, float & velocityX, float & velocityY, float & height );
   void                          ExtractTotalDepthSum( double & totalSum );
   //
public:
   //
   //wsTexSplattingMgr &             GetwsTexSplattingMgr()                      { return m_texSplattingMgr; }
   //
};
