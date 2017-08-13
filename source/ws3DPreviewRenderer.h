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

#include "wsQuadTree.h"
#include "wsCascadedWaveMapSim.h"

#include "wsShader.h"

class ws3DPreviewCamera;
class ws3DPreviewLightMgr;

class ws3DPreviewRenderer : protected DxEventReceiver, protected IWaterSurfaceInfoProvider
{
private:

   struct DetailWaves
   {
      static const int                       TextureCount   = 16;
      int                                    TextureResolution;

      IDirect3DTexture9 *                    NMTextures[TextureCount];
   };

   struct Settings
   {
      float                                  FoamTextureUVScale;

      float                                  DetailWaveUVScale;
      float                                  DetailWaveAnimSpeed;
      float                                  DetailWaveStrength;
      float                                  WaterMinVisibleLimitDepth;
      float                                  TestSplashRadius;
      float                                  VertexZCWSScale;
      float                                  VertexZCWSDropoffDepth;
      float                                  VertexZCWSMaxDropoffK;
      //float                                  WaterZOffset;

      bool                                   UseProperUnderwaterRendering;
      bool                                   UseVertexWaveZOffset;

      Settings()
      {
         FoamTextureUVScale            = 512.0f;
         DetailWaveUVScale             = 192.0f;
         DetailWaveAnimSpeed           = 0.5f;
         DetailWaveStrength            = 1.0f;
         WaterMinVisibleLimitDepth     = 0.5f;
         TestSplashRadius              = 15.0f;
         //WaterZOffset                  = 1.0f;
         VertexZCWSScale               = 0.0f;
         VertexZCWSDropoffDepth        = 10.0f;
         VertexZCWSMaxDropoffK         = 0.2f;

         UseProperUnderwaterRendering  = false;
         UseVertexWaveZOffset          = true;
      }
   };

   Settings                                  m_settings;

   DetailWaves                               m_detailWaves;

   wsVertexShader                            m_vsGrid;
   wsVertexShader                            m_vsGridWater;
   wsVertexShader                            m_vsGridWaterCWSOffset;

   wsPixelShader                             m_psTerrainFlat;
   wsPixelShader                             m_psTerrainSplatting;
   wsPixelShader                             m_psWater;

   wsVertexShader                            m_vsVelocityMap;
   wsPixelShader                             m_psVelocityMap;
   wsPixelShader                             m_psVelocityMapFilter0;
   wsPixelShader                             m_psVelocityMapFilter1;

   wsPixelShader                             m_psWireframe;

   wsVertexShader                            m_vsWriteWaterDepth;
   wsPixelShader                             m_psWriteWaterDepth;

   bool                                      m_wireframe;
   bool                                      m_useTerrainFlatShading;

   AdVantage::TiledBitmap *                  m_heightmap;
   //AdVantage::TiledBitmap *                  m_watermap;

   AdVantage::TiledBitmap *                  m_rendWaterHeightmap;
   AdVantage::TiledBitmap *                  m_rendWaterInfomap;

   IDirect3DTexture9 *                       m_terrainHMTexture;
   IDirect3DTexture9 *                       m_terrainNMTexture;

   IDirect3DTexture9 *                       m_waterHMTexture;
   IDirect3DTexture9 *                       m_waterNMTexture;
   IDirect3DTexture9 *                       m_waterVMTexture;

   IDirect3DVertexBuffer9 *                  m_gridVertexBuffer;
   IDirect3DIndexBuffer9 *                   m_gridIndexBuffer;
   int                                       m_gridDim;                 // mesh = (m_gridDim+1) * (m_gridDim+1) vertices, and m_gridDim * m_gridDim quads
   int                                       m_quadMinSize;
   int                                       m_gridIndexEndTL, m_gridIndexEndTR, m_gridIndexEndBL, m_gridIndexEndBR;

   IDirect3DSurface9 *                       m_velocityMapUpdateBackBuffer;
   IDirect3DTexture9 *                       m_velocityMapFilterTemp;

   IDirect3DTexture9 *                       m_renderTargetDataTmp;
   //IDirect3DTexture9 *                       m_depthBufferTexture;

   IDirect3DTexture9 *                       m_foamMapTexture;

   IDirect3DTexture9 *                       m_terrainSplatMapTexture;

   int                                       m_rasterWidth;
   int                                       m_rasterHeight;

   MapDimensions                             m_mapDims;
   float                                     m_waterIgnoreBelowDepth;      // in word (MapDims) space!
   float                                     m_waterElevateIfNotIgnored;   // in word (MapDims) space!
   float                                     m_terrainWaterHeightScale;
   //float                                     m_waterDropoffRateIfBelow;

   float                                     m_cameraViewRange;

   float                                     m_globalUVSpeedFactor;

   double                                    m_globalTime;

   wsQuadTree                                m_terrainQuadTree;
   wsQuadTree                                m_waterQuadTree;
   wsCascadedWaveMapSim                      m_cascadedWaveMapSim;

   //bool                                      m_useSkybox;

   D3DSURFACE_DESC                           m_backBufferSurfaceDesc;

   float                                     m_lastSplashTime;

   bool                                      m_mouseLeftButtonDown;
   int                                       m_mouseClientX;
   int                                       m_mouseClientY;

public:
   ws3DPreviewRenderer(void);
   virtual ~ws3DPreviewRenderer(void);
   //
public:
   void                          Start( AdVantage::TiledBitmap * heightmap, AdVantage::TiledBitmap * watermap, AdVantage::TiledBitmap *  rendWaterHeightmap, AdVantage::TiledBitmap * rendWaterInfomap, const MapDimensions & mapDims );
   void                          Stop( );
   //
   void                          InitializeFromIni( );
   //
   void                          Tick( ws3DPreviewCamera * camera, float deltaTime );
   HRESULT                       Render( ws3DPreviewCamera * camera, ws3DPreviewLightMgr * lightMgr, float deltaTime );
   //
   void                          ForceCreateDevice( );
   void                          ForceResetDevice( const D3DSURFACE_DESC * pBackBufferSurfaceDesc );
   //
protected:
   //
   virtual HRESULT 				   OnCreateDevice();
   virtual HRESULT               OnResetDevice( const D3DSURFACE_DESC * pBackBufferSurfaceDesc );
   //
   void                          RenderNode( const wsQuadTree::NodeSelection & nodeSel, wsQuadTree::LODSelectionInfo & selInfo, ID3DXConstantTable * vsConsts, ID3DXConstantTable * psConsts );
   void                          RenderNodes( const wsQuadTree::NodeSelection * selectionArray, int selectionCount, wsQuadTree::LODSelectionInfo & selInfo, ID3DXConstantTable * vsConsts, ID3DXConstantTable * psConsts );
   void                          RenderNodesWithCWM( const wsQuadTree::NodeSelection * selectionArray, int selectionCount, wsQuadTree::LODSelectionInfo & selInfo, ws3DPreviewCamera * camera, ID3DXConstantTable * vsConsts, ID3DXConstantTable * psConsts );
   //
   void                          SetupGlobalVSConsts( ID3DXConstantTable * consts, const D3DXMATRIX & viewProj, const D3DXVECTOR3 & camPos, ws3DPreviewLightMgr * lightMgr, int textureWidth, int textureHeight );
   //
   void                          SampleAndProcessWaterTexture( float * waterheightData, const int waterheightDataPitch, unsigned __int64 * velocitymapData, int velocitymapDataPitch );
   void                          InitializeTextures();
   //
   void                          CreateBlockMesh();
   //
   void                          RenderSprings( ws3DPreviewCamera * camera, ws3DPreviewLightMgr * lightMgr, float deltaTime );
   //
   virtual void                  OnLostDevice( );
   virtual void                  OnDestroyDevice( );
   //
   virtual void                  GetAreaMinMaxHeight( float fromX, float fromY, float sizeX, float sizeY, float & minZ, float & maxZ )       { m_terrainQuadTree.GetAreaMinMaxHeight( fromX, fromY, sizeX, sizeY, minZ, maxZ); }
   virtual void                  UpdateVelocityTexture( float fromX, float fromY, float sizeX, float sizeY, float minZ, float maxZ, IDirect3DTexture9 * velocityMap );
   //
public:
   //
   void                          MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing );
   //wsTexSplattingMgr &             GetwsTexSplattingMgr()                      { return m_texSplattingMgr; }
   //
};
