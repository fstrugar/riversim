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
#include "DXUTgui.h"

#include "wsMain.h"

#include "ws3DPreviewRenderer.h"
#include "ws3DPreviewCamera.h"
#include "ws3DPreviewLightMgr.h"
#include "wsTexSplattingMgr.h"

#include "wsCanvas.h"

#include "wsSimulator.h"

#include "wsSpringsMgr.h"

#include "TiledBitmap/TiledBitmap.h"

#include "iniparser\src\IniParser.hpp"

#include "wsShader.h"

#include "wsExporter.h"

#include <time.h>

bool                       g_bHelpText             = false;
bool                       g_bPause                = false;

ws3DPreviewCamera *        g_pPreviewCamera        = NULL;
ws3DPreviewRenderer *      g_pPreviewRenderer      = NULL;
ws3DPreviewLightMgr *      g_pPreviewLightMgr      = NULL;

wsSimulator *              g_pSimulator            = NULL;

AdVantage::TiledBitmap *   g_heightmap             = NULL;
AdVantage::TiledBitmap *   g_simWatermap           = NULL;

AdVantage::TiledBitmap *   g_rendWaterHeightmap    = NULL;
AdVantage::TiledBitmap *   g_rendWaterInfomap      = NULL;

bool                       g_deviceCreated         = false;
bool                       g_deviceReset           = false;
D3DSURFACE_DESC            g_backBufferSrufaceDesc;

ExporterSettings           g_exporterSettings;

float                      g_initialCameraViewRange = 1.0f;

POINT                      g_OldCursorPos;
POINT                      g_CursorCenter;
POINT                      g_CursorDelta;

bool                       g_Keys[kcLASTVALUE];            
bool                       g_KeyUps[kcLASTVALUE];
bool                       g_KeyDowns[kcLASTVALUE];

ApplicationMode            g_appMode            = AM_InvalidValue;

bool                       g_hasCursorCapture   = false;
HWND                       g_hwndMain           = NULL;

string                     g_heightmapPath            = "";
string                     g_simWatermapPath          = "";
string                     g_iniFilePath              = "";

string                     g_rendWaterHeightmapPath   = "";
string                     g_rendWaterInfomapPath     = "";

MapDimensions              g_mapDims;

string                     g_cmdLine;

wstring                    g_sWindowTitle       = L"RiverSimulator 0.91";

string                     g_initialWorkingDir  = "";

string                     g_projectDir         = "";

extern CDXUTDialog         g_HUD;

void ReleaseCursorCapture( );

const char *   wsGetIniPath() { return g_iniFilePath.c_str(); }

POINT GetCursorDelta()
{
   return g_CursorDelta;
}

float	WrapAngle( float angle )
{
   while( angle > PI )  angle -= (float)PI*2;
   while( angle < -PI ) angle += (float)PI*2;
   return angle;
}

extern void wsDrawCanvas2D();

void     wsDisplaySwitchingModeInfo()
{
   GetCanvas2D()->CleanQueued();

   int lineYPos = g_backBufferSrufaceDesc.Height / 2 - 16;
   GetCanvas2D()->DrawText( g_backBufferSrufaceDesc.Width/2 - 48, lineYPos, L"SWITCHING MODE" );
   lineYPos += 16;
   GetCanvas2D()->DrawText( g_backBufferSrufaceDesc.Width/2 - 55, lineYPos, L"this can take a while" );

   // Render the scene
   if( SUCCEEDED( DxEventNotifier::GetD3DDevice()->BeginScene() ) )
   {
      DxEventNotifier::GetD3DDevice()->Clear( 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB( 0, 0, 48, 0 ), 1.0f, 0 );

      wsDrawCanvas2D();
      DxEventNotifier::GetD3DDevice()->EndScene();
   }

   DxEventNotifier::GetD3DDevice()->Present( NULL, NULL, NULL, NULL );
}

void     wsSwitchToMode( ApplicationMode newMode )
{
   if( newMode == g_appMode )
      return;

   ReleaseCursorCapture();

   g_appMode = newMode;

   if( g_pSimulator != NULL )
   {
      wsDisplaySwitchingModeInfo();

      g_pSimulator->Stop();
      delete g_pSimulator;
      g_pSimulator = NULL;

      wsExportRenderData( g_heightmap, g_simWatermap, g_rendWaterHeightmap, g_rendWaterInfomap, g_mapDims, g_exporterSettings );
   }

   if( g_pPreviewRenderer != NULL )
   {
      wsDisplaySwitchingModeInfo();

      g_pPreviewRenderer->Stop();
      delete g_pPreviewRenderer;
      g_pPreviewRenderer = NULL;
   }

   if( newMode == AM_Simulation )
   {
      g_pSimulator = new wsSimulator();
      g_pSimulator->Start( g_heightmap, g_simWatermap, g_mapDims );
      
      if( g_deviceCreated )   
         g_pSimulator->OnCreateDevice();
      if( g_deviceReset )
         g_pSimulator->OnResetDevice( &g_backBufferSrufaceDesc );
   }

   if( newMode == AM_Preview )
   {
      //// !!TEMPTEMPTEMP!!
      //wsExportRenderData( g_heightmap, g_simWatermap, g_rendWaterHeightmap, g_rendWaterInfomap, g_mapDims, g_exporterSettings );

      g_pPreviewRenderer = new ws3DPreviewRenderer();
      g_pPreviewRenderer->Start( g_heightmap, g_simWatermap, g_rendWaterHeightmap, g_rendWaterInfomap, g_mapDims );

      if( g_deviceCreated )   
         g_pPreviewRenderer->ForceCreateDevice();
      if( g_deviceReset )
         g_pPreviewRenderer->ForceResetDevice( &g_backBufferSrufaceDesc );
   }
}

string wsAddProjectDir( const string & fileName )
{
   if( fileName.size() == 0 )
      return fileName;
   if( fileName.size() == 1 )
      return g_projectDir + fileName;
   if( fileName[1] == ':' || (fileName[0]=='\\' && fileName[1]=='\\') || (fileName[0]=='/' && fileName[1]=='/') )
      return fileName;
   return g_projectDir + fileName;
}

bool  wsInitialize()
{
   TCHAR szDirectory[MAX_PATH] = L"";

   if(!::GetCurrentDirectory(sizeof(szDirectory) - 1, szDirectory))
   {
      MessageBox(NULL, L"Error trying to get working directory", L"Error", MB_OK );
      return false;
   }

   srand( (unsigned)time(NULL) );

   g_initialWorkingDir = simple_narrow( wstring(szDirectory) );
   g_initialWorkingDir += "\\";

   g_iniFilePath = "";
   if( g_cmdLine != "" )
   {
      if( wsFileExists((g_initialWorkingDir + g_cmdLine).c_str()) )
      {
         g_iniFilePath = g_initialWorkingDir + g_cmdLine;
      }
      else if( wsFileExists(g_cmdLine.c_str()) )
      {
         g_iniFilePath = g_cmdLine;
      }
   }

   if( g_iniFilePath == "" )
   {
      OPENFILENAME ofn;			ZeroMemory( &ofn, sizeof(ofn) );
      wchar_t sFile[MAX_PATH];	ZeroMemory( &sFile, sizeof(sFile) );
      ofn.lStructSize = sizeof(ofn);
      ofn.lpstrFilter = L"RiverSimulator project ini file (.rsini)\0*.rsini\0\0";
      ofn.lpstrFile = sFile;
      ofn.nMaxFile = sizeof(sFile);
      ofn.lpstrTitle = L"Select map file";
      ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

      if ( GetOpenFileName( &ofn ) )
      {
         g_iniFilePath = simple_narrow( wstring(ofn.lpstrFile) );
      }
      else
      {
         return false;
      }
   }

   IniParser   iniParser;
   if( !iniParser.Open( g_iniFilePath.c_str() ) )
   {
      MessageBoxA( NULL, Format( "Error trying to open main config file '%s'.", g_iniFilePath.c_str() ).c_str(), "Error", MB_OK );
      goto wsInitializeError;
   }

   {
      string nameOnly;
      wsSplitFilePath<string>( g_iniFilePath, g_projectDir, nameOnly );
   }

   g_heightmapPath            = wsAddProjectDir( iniParser.getString( "Main:heightmapPath", "" ) );
   g_simWatermapPath          = wsAddProjectDir( iniParser.getString( "Main:simWatermapPath", "" ) );

   g_rendWaterHeightmapPath   = wsAddProjectDir( iniParser.getString( "Main:rendWaterHeightmapPath", "" ) );
   g_rendWaterInfomapPath     = wsAddProjectDir( iniParser.getString( "Main:rendWaterInfomapPath", "" ) );

   g_mapDims.MinX    = iniParser.getFloat( "Main:mapDims_MinX", -50.0 );
   g_mapDims.MinY    = iniParser.getFloat( "Main:mapDims_MinY", -50.0 );
   g_mapDims.MinZ    = iniParser.getFloat( "Main:mapDims_MinZ", 0.0 );

   g_mapDims.SizeX   = iniParser.getFloat( "Main:mapDims_SizeX", 100.0 );
   g_mapDims.SizeY   = iniParser.getFloat( "Main:mapDims_SizeY", 100.0 );
   g_mapDims.SizeZ   = iniParser.getFloat( "Main:mapDims_SizeZ", 20.0 );

   g_exporterSettings.WaterIgnoreBelowDepth        = iniParser.getFloat("Exporter:waterIgnoreBelowDepth", 0.0f);
   g_exporterSettings.WaterIgnoreBelowDepthSpec    = iniParser.getFloat("Exporter:waterIgnoreBelowDepthSpec", 0.0f);
   g_exporterSettings.WaterElevateIfNotIgnored     = iniParser.getFloat("Exporter:waterElevateIfNotIgnored", 0.0f);

   g_exporterSettings.DepthBasedFrictionMinDepth   = iniParser.getFloat("Simulation:depthBasedFrictionMinDepth", 0.0f );
   g_exporterSettings.DepthBasedFrictionMaxDepth   = iniParser.getFloat("Simulation:depthBasedFrictionMaxDepth", 1.0f );
   g_exporterSettings.DepthBasedFrictionAmount     = iniParser.getFloat("Simulation:depthBasedFrictionAmount", 0.9f );

   g_initialCameraViewRange                        = iniParser.getFloat("Rendering:cameraViewRange", 100000.0f );

   g_heightmap = AdVantage::TiledBitmap::Open( g_heightmapPath.c_str(), true );

   bool startInSimulation = iniParser.getBool( "Main:startInSimulation", true );
   
   if( g_heightmap == NULL )
   {
      MessageBox(NULL, L"Cannot open heightmap", L"Error", MB_OK );
      goto wsInitializeError;
   }
   if( g_heightmap->PixelFormat() != AdVantage::TBPF_Format16BitGrayScale )
   {
      MessageBox(NULL, L"Invalid heightmap pixel format", L"Error", MB_OK );
      goto wsInitializeError;
   }


   //////////////////////////////////////////////////////////////////////////
   // Simulation state watermap
   if( !wsFileExists( g_simWatermapPath.c_str() ) )
   {
      int width   = g_heightmap->Width();
      int height  = g_heightmap->Height();
      g_simWatermap = AdVantage::TiledBitmap::Create( g_simWatermapPath.c_str(), AdVantage::TBPF_Format64BitD32VY16VX16, width, height );

      // Setup initial values
      for( int y = 0; y < height; y++ )
         for( int x = 0; x < width; x++ )
         {
            float depth          = 0.0f;
            unsigned short velx  = HalfFloatPack( 0.0f );
            unsigned short vely  = HalfFloatPack( 0.0f );
            __int64  pixel = (((__int64)*((unsigned int*) &depth)) << 32) | ((__int64)vely << 16) | velx;
            g_simWatermap->SetPixel( x, y, &pixel );
          }
      startInSimulation = true;
   }
   else
   {
      g_simWatermap = AdVantage::TiledBitmap::Open( g_simWatermapPath.c_str(), false );
      if( g_simWatermap->PixelFormat() != AdVantage::TBPF_Format64BitD32VY16VX16 )
      {
         MessageBox(NULL, L"Invalid watermap pixel format", L"Error", MB_OK );
         goto wsInitializeError;
      }
   }
   //////////////////////////////////////////////////////////////////////////


   //////////////////////////////////////////////////////////////////////////
   // Watermaps used for rendering (these are calculated by simulator and used by renderer)
   //
   if( !wsFileExists( g_rendWaterHeightmapPath.c_str() ) )
   {
      int width   = g_heightmap->Width();
      int height  = g_heightmap->Height();
      g_rendWaterHeightmap = AdVantage::TiledBitmap::Create( g_rendWaterHeightmapPath.c_str(), AdVantage::TBPF_Format16BitGrayScale, width, height );

      // Setup initial values
      unsigned short defaultValue = 0;
      for( int y = 0; y < height; y++ )
         for( int x = 0; x < width; x++ )
            g_rendWaterHeightmap->SetPixel( x, y, &defaultValue );
      startInSimulation = true;
   }
   else
   {
      g_rendWaterHeightmap = AdVantage::TiledBitmap::Open( g_rendWaterHeightmapPath.c_str(), false );
      if( g_rendWaterHeightmap->PixelFormat() != AdVantage::TBPF_Format16BitGrayScale )
      {
         MessageBox(NULL, L"Invalid water render heightmap pixel format", L"Error", MB_OK );
         goto wsInitializeError;
      }
   }
   //
   if( !wsFileExists( g_rendWaterInfomapPath.c_str() ) )
   {
      int width   = g_heightmap->Width();
      int height  = g_heightmap->Height();
      g_rendWaterInfomap = AdVantage::TiledBitmap::Create( g_rendWaterInfomapPath.c_str(), AdVantage::TBPF_Format32BitARGB, width, height );

      // Setup initial values
      unsigned int defaultValue = 0x00808000;   // values are X8R8G8B8 where R & G are velocities, and B is perturbance
      for( int y = 0; y < height; y++ )
         for( int x = 0; x < width; x++ )
            g_rendWaterInfomap->SetPixel( x, y, &defaultValue );
      startInSimulation = true;
   }
   else
   {
      g_rendWaterInfomap = AdVantage::TiledBitmap::Open( g_rendWaterInfomapPath.c_str(), false );
      if( g_rendWaterInfomap->PixelFormat() != AdVantage::TBPF_Format32BitARGB )
      {
         MessageBox(NULL, L"Invalid water render infomap pixel format", L"Error", MB_OK );
         goto wsInitializeError;
      }
   }
   //
   //////////////////////////////////////////////////////////////////////////
   if( g_simWatermap == NULL )
   {
      MessageBox(NULL, L"Cannot open simWatermap", L"Error", MB_OK );
      goto wsInitializeError;
   }

   if( g_rendWaterHeightmap == NULL )
   {
      MessageBox(NULL, L"Cannot open rendWaterHeightmap", L"Error", MB_OK );
      goto wsInitializeError;
   }

   if( g_rendWaterInfomap == NULL )
   {
      MessageBox(NULL, L"Cannot open rendWaterInfomap", L"Error", MB_OK );
      goto wsInitializeError;
   }
   //////////////////////////////////////////////////////////////////////////

   wsSpringsMgr::Instance().Initialize( g_mapDims );
   wsTexSplattingMgr::Instance().Initialize( );

   g_pPreviewCamera     = new ws3DPreviewCamera();
   g_pPreviewLightMgr   = new ws3DPreviewLightMgr();

   wsSwitchToMode( (startInSimulation)?(AM_Simulation):(AM_Preview) );

   return true;

wsInitializeError:

   SAFE_DELETE( g_heightmap );
   SAFE_DELETE( g_simWatermap );
   SAFE_DELETE( g_rendWaterHeightmap );
   SAFE_DELETE( g_rendWaterInfomap );

   return false;
}

void     wsGetProjectDefaultCameraParams( D3DXVECTOR3 & pos, const D3DXVECTOR3 & dir, float & viewRange )
{
   D3DXVECTOR3 bmin( g_mapDims.MinX, g_mapDims.MinY, g_mapDims.MinZ );
   D3DXVECTOR3 bmax( g_mapDims.MaxX(), g_mapDims.MaxY(), g_mapDims.MaxZ() );

   D3DXVECTOR3 bcent = (bmin + bmax) * 0.5f;
   D3DXVECTOR3 bsize = bmax - bmin;
   
   float size = D3DXVec3Length( &bsize );

   pos = bcent - dir * (size * 0.7f);

   viewRange = g_initialCameraViewRange;
}

void     wsDeinitialize()
{
   if( g_pPreviewCamera != NULL )
   {
      delete g_pPreviewCamera;
      g_pPreviewCamera = NULL;
   }

   if( g_pPreviewLightMgr != NULL )
   {
      delete g_pPreviewLightMgr;
      g_pPreviewLightMgr = NULL;
   }

   if( g_pSimulator != NULL )
   {
      g_pSimulator->Stop();
      delete g_pSimulator;
      g_pSimulator = NULL;

      wsExportRenderData( g_heightmap, g_simWatermap, g_rendWaterHeightmap, g_rendWaterInfomap, g_mapDims, g_exporterSettings  );
   }

   if( g_pPreviewRenderer != NULL )
   {
      g_pPreviewRenderer->Stop();
      delete g_pPreviewRenderer;
      g_pPreviewRenderer = NULL;
   }

   SAFE_DELETE( g_heightmap );
   SAFE_DELETE( g_simWatermap );
   SAFE_DELETE( g_rendWaterHeightmap );
   SAFE_DELETE( g_rendWaterInfomap );

   wsSpringsMgr::Instance().Deinitialize();
   wsTexSplattingMgr::Instance().Deinitialize();
}

//
KeyCommands	GetKeyBind( WPARAM vKey, LPARAM lParam )
{
   switch( vKey )
   {
   case 'W': 	               return kcForward;
   case 'S': 	               return kcBackward;
   case 'A': 	               return kcLeft;
   case 'D': 	               return kcRight;
   case 'Q': 	               return kcUp;
   case 'Z': 	               return kcDown;
   case VK_SHIFT:
   case VK_LSHIFT:
   case VK_RSHIFT:      return kcShiftKey;
   case VK_LCONTROL:
   case VK_RCONTROL:
   case VK_CONTROL:     return kcControlKey;
      //case 'Z': 	               return kcCameraSpeedDown;
      //case 'X': 	               return kcCameraSpeedUp;
   case 'C': 	               return kcViewRangeDown;
   case 'V': 	               return kcViewRangeUp;
   case 'B': 	               return kcSpringBoostMode;
   //case '1':                  return kcAddSphereObject;
   //case '2':                  return kcAddBoxObject;
   //case '3':                  return kcAddManyObjects;
   //case VK_BACKSPACE:   return kcCleanAllObjects;
   case 'I':            return kcToggleCalculateTotalWaterSum;
   case VK_F1:          return kcToggleHelpText;
   case VK_F8:          return kcToggleSimpleRender;
   case VK_F9:          return kcToggleWireframe;
   case VK_F11:         
      {
         if( IsKey( kcControlKey ) )
            return kcReloadAll;
         else
            return kcReloadShadersOnly;
      }

   case VK_NUMPAD0:     return kcSetLight0;
   case VK_NUMPAD1:     return kcSetLight1;
   case VK_NUMPAD2:     return kcSetLight2;
   case VK_NUMPAD3:     return kcSetLight3;
   case VK_NUMPAD4:     return kcSetLight4;
   case VK_NUMPAD5:     return kcSetLight5;
   case VK_NUMPAD6:     return kcSetLight6;
   case VK_NUMPAD7:     return kcSetLight7;
   case VK_NUMPAD8:     return kcSetLight8;
   case VK_NUMPAD9:     return kcSetLight9;

   case 'P':            return kcPause;
   case 'O':            return kcOneFrameStep;

   case VK_F5:          return kcToggleAppMode;
   case VK_F6:          return kcToggleShowVelocitiesOrDepth;

   case VK_SPACE:       return kcDoSimpleSplash;

   default: return kcLASTVALUE;
   }
}
//
bool IsKey( KeyCommands key )
{
   return g_Keys[key];
}
//
bool IsKeyClicked( KeyCommands key )
{
   return g_KeyDowns[key];
}
//
bool IsKeyReleased( KeyCommands key )
{
   return g_KeyUps[key];
}
//
void SetCursorCapture( )
{
   if( g_hasCursorCapture ) return;
   g_hasCursorCapture = true;

   g_HUD.EnableMouseInput( false );

   GetCursorPos( &g_OldCursorPos );
   RECT dwr;
   //GetWindowRect( GetDesktopWindow(), &dwr );
   GetWindowRect( g_hwndMain, &dwr );
   ShowCursor( FALSE );
   g_CursorCenter.x = (dwr.left + dwr.right) / 2; 
   g_CursorCenter.y = (dwr.top + dwr.bottom) / 2;
   SetCursorPos( g_CursorCenter.x, g_CursorCenter.y );
   SetCapture( g_hwndMain );

   //GetWindowRect( hWnd, &dwr );
   //ClipCursor( &dwr );
}
//
void ReleaseCursorCapture( )
{
   if( !g_hasCursorCapture ) return;
   g_hasCursorCapture = false;

   SetCursorPos( g_OldCursorPos.x, g_OldCursorPos.y );
   ShowCursor( TRUE );
   //ClipCursor( NULL );
   ReleaseCapture();

   g_HUD.EnableMouseInput( true );
}
//
void OnGotFocus( HWND hWnd )
{
   for( int i = 0; i < kcLASTVALUE; i++ )
      g_Keys[i] = false;
}
//
void OnLostFocus( HWND hWnd )
{
   ReleaseCursorCapture();
}
//
void wsMsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing )
{
   if( g_appMode == AM_Simulation )
   {
      g_pSimulator->MsgProc( hWnd, msg, wParam, lParam, pbNoFurtherProcessing );
   }

   if( g_appMode == AM_Preview )
   {
      g_pPreviewRenderer->MsgProc( hWnd, msg, wParam, lParam, pbNoFurtherProcessing );
   }

   if( *pbNoFurtherProcessing )
      return;

   KeyCommands key;

   switch( msg )
   {
   case WM_SETFOCUS:
      *pbNoFurtherProcessing = true;
      OnGotFocus( hWnd );
      break;
   case WM_KILLFOCUS:
      *pbNoFurtherProcessing = true;
      OnLostFocus( hWnd );
      break;
   case WM_KEYDOWN:
      if( wParam == VK_ESCAPE )
      {
         PostQuitMessage( 0 );
         //return 0;
      }
      key = GetKeyBind( wParam, lParam ) ;
      if( key != kcLASTVALUE ) 
      { 
         g_Keys[key]		= true;
         g_KeyDowns[key] = true;
      }
      break;
   case WM_KEYUP:
      key = GetKeyBind( wParam, lParam ) ;
      if( key != kcLASTVALUE ) 
      { 
         g_Keys[key]		= false;
         g_KeyUps[key]	= true;
      }
      break;
   case WM_RBUTTONDOWN:
      *pbNoFurtherProcessing = true;
      if( g_hasCursorCapture )
         ReleaseCursorCapture( );
      else
         SetCursorCapture( );
      break;

   }
}

#pragma warning (disable : 4996)

std::wstring Format(const wchar_t * fmtString, ...)
{
   va_list args;
   va_start(args, fmtString);

   int nBuf;
   wchar_t szBuffer[512];

   nBuf = _vsnwprintf(szBuffer, sizeof(szBuffer) / sizeof(WCHAR), fmtString, args);
   assert(nBuf < sizeof(szBuffer));//Output truncated as it was > sizeof(szBuffer)

   va_end(args);

   return std::wstring(szBuffer);
}

std::string Format(const char * fmtString, ...)
{
   va_list args;
   va_start(args, fmtString);

   int nBuf;
   char szBuffer[512];

   nBuf = _vsnprintf(szBuffer, sizeof(szBuffer) / sizeof(char), fmtString, args);
   assert(nBuf < sizeof(szBuffer));//Output truncated as it was > sizeof(szBuffer)

   va_end(args);

   return std::string(szBuffer);
}

void DrawInfoText();

void wsTick( float deltaTime )
{
   if( IsKeyClicked( kcToggleHelpText ) )
      g_bHelpText = !g_bHelpText;

   if( IsKeyClicked( kcPause ) )
      g_bPause = !g_bPause;
   if( IsKeyClicked( kcOneFrameStep ) )
      g_bPause = true;

   if( IsKeyClicked( kcToggleAppMode ) )
   {
      if( g_appMode == AM_Simulation )
         wsSwitchToMode( AM_Preview );
      else if( g_appMode == AM_Preview )
         wsSwitchToMode( AM_Simulation );
   }

   if( IsKeyClicked( kcReloadShadersOnly ) || IsKeyClicked( kcReloadAll ) )
   {
      wsShader::ReloadAllShaders();
   }
   
   g_CursorDelta.x = 0;
   g_CursorDelta.y = 0;

   if( g_hasCursorCapture )
   {
      POINT pt;
      GetCursorPos( &pt );
      SetCursorPos( g_CursorCenter.x, g_CursorCenter.y );

      g_CursorDelta.x = pt.x - g_CursorCenter.x;
      g_CursorDelta.y = pt.y - g_CursorCenter.y;
   }

   //ws3DPreviewLightMgr::Instance().Tick( deltaTime );

   if( g_appMode == AM_Preview )
   {
      g_pPreviewCamera->Tick( deltaTime );
      g_pPreviewLightMgr->Tick( deltaTime );
   }

   if( g_bPause )
   {
      if( IsKeyClicked( kcOneFrameStep ) )
         deltaTime = 1 / 60.0f;
      else
         deltaTime = 0.0f;
   }

   wsSpringsMgr::Instance().Tick( deltaTime );

   if( g_appMode == AM_Simulation )
   {
      g_pSimulator->Tick( deltaTime );
   }

   if( g_appMode == AM_Preview )
   {
      g_pPreviewRenderer->Tick( g_pPreviewCamera, deltaTime );
   }

   DrawInfoText();

   for( int i = 0; i < kcLASTVALUE; i++ )
   {
      g_KeyUps[i]		= false;
      g_KeyDowns[i]	= false;
   }
}

extern void wsDrawCanvas3D( ws3DPreviewCamera * camera );

HRESULT wsRender( float deltaTime )
{
   HRESULT hr;

   if( g_appMode == AM_Simulation )
   {
      V( g_pSimulator->Render() );
   }

   if( g_appMode == AM_Preview )
   {
      V( g_pPreviewLightMgr->Render( g_pPreviewCamera ) );
      V( g_pPreviewRenderer->Render( g_pPreviewCamera, g_pPreviewLightMgr, deltaTime ) );

      wsDrawCanvas3D( g_pPreviewCamera );
   }

   return S_OK;
}

string GetWorkingDirectory()
{
   return g_initialWorkingDir;
}

string wsGetProjectDirectory()
{
   return g_projectDir;
}

string wsGetExeDirectory()
{
   /*
   #if _DEBUG
   return wstring(L"");
   #else
   */
   // Get the exe name, and exe path
   WCHAR strExePath[MAX_PATH] = {0};
   WCHAR strExeName[MAX_PATH] = {0};
   WCHAR* strLastSlash = NULL;
   GetModuleFileName( NULL, strExePath, MAX_PATH );
   strExePath[MAX_PATH-1]=0;
   strLastSlash = wcsrchr( strExePath, TEXT('\\') );
   if( strLastSlash )
   {
      StringCchCopy( strExeName, MAX_PATH, &strLastSlash[1] );

      // Chop the exe name from the exe path
      *strLastSlash = 0;

      // Chop the .exe from the exe name
      strLastSlash = wcsrchr( strExeName, TEXT('.') );
      if( strLastSlash )
         *strLastSlash = 0;
   }

   wstring ret = wstring(strExePath) + L"\\";
   return simple_narrow( ret );
   //#endif
}
//
bool wsFileExists( const char * file )
{
   FILE * fp = fopen( file, "rb" );
   if( fp != NULL )
   {
      fclose( fp );
      return true;
   }
   return false;
}
//
bool wsFileExists( const wchar_t * file )
{
   FILE * fp = _wfopen( file, L"rb" );
   if( fp != NULL )
   {
      fclose( fp );
      return true;
   }
   return false;
}
//
std::string simple_narrow( const std::wstring & s ) {
   std::string ws;
   ws.resize(s.size());
   for( size_t i = 0; i < s.size(); i++ ) ws[i] = (char)s[i];
   return ws;
}
//
std::wstring simple_widen( const std::string & s ) {
   std::wstring ws;
   ws.resize(s.size());
   for( size_t i = 0; i < s.size(); i++ ) ws[i] = s[i];
   return ws;
}
//
string wsFindResource( const std::string & file, bool showErrorMessage )
{
   //   if( wsFileExists( file.c_str()) )
   //      return file;

   string test = GetWorkingDirectory() + file;
   if( wsFileExists( test.c_str() )  )
      return test;

   test = wsGetExeDirectory() + file;
   if( wsFileExists( test.c_str() )  )
      return test;

   ////NO COMMIT
   //test = wstring(L"E:\\WorkData\\Biiig\\MiniTest\\Source\\") + file;
   //if( wsFileExists( test.c_str() )  )
   //   return test;

   if( showErrorMessage )
   {
      MessageBoxA(NULL, Format("Error trying to find '%s'!", file.c_str()).c_str(), "Error", MB_OK );
   }

   return "";
}

wstring wsFindResource( const wstring & file, bool showErrorMessage )
{
   return simple_widen( wsFindResource( simple_narrow( file ), showErrorMessage) );
}

void DrawInfoText()
{
   int lineHeight = 13;
   int lineYPos = 42;

   const int lineXPos = 6;

   if( g_appMode == AM_Simulation )    GetCanvas2D()->DrawText( lineXPos, lineYPos, L"Mode: Simulation (F5 to switch)" );
   if( g_appMode == AM_Preview )       GetCanvas2D()->DrawText( lineXPos, lineYPos, L"Mode: Preview (F5 to switch)" );
   if( g_bPause )                      GetCanvas2D()->DrawText( g_backBufferSrufaceDesc.Width/2 - 44, lineYPos, L"PAUSED" );
   lineYPos += (int)(lineHeight * 1.5);

   if( !g_bHelpText )
   {
      GetCanvas2D()->DrawText( lineXPos, lineYPos, L"F1 : show more info"); lineYPos += lineHeight;
   }
   else
   {
      //GetCanvas2D()->DrawText( lineXPos, lineYPos, L"Info:" ); lineYPos += lineHeight + 4;
      //GetCanvas2D()->DrawText( lineXPos, lineYPos, L"Nothing to see here yet, sorry :)" ); lineYPos += lineHeight;
      GetCanvas2D()->DrawText( lineXPos, lineYPos, L"F1 : hide info"); lineYPos += lineHeight;
      lineYPos += 6;

      if( g_appMode == AM_Simulation )
      {
         GetCanvas2D()->DrawText( lineXPos, lineYPos, L"Simulation mode keyboard commands" );   lineYPos += lineHeight;
         GetCanvas2D()->DrawText( lineXPos, lineYPos, L"  Right mouse btn: drag to move"); lineYPos += lineHeight;
         GetCanvas2D()->DrawText( lineXPos, lineYPos, L"  Left mouse btn: add water at point"); lineYPos += lineHeight;
         GetCanvas2D()->DrawText( lineXPos, lineYPos, L"  Mouse wheel: zoom in/out"); lineYPos += lineHeight;
         GetCanvas2D()->DrawText( lineXPos, lineYPos, L"  I: toggle calculate total water sum"); lineYPos += lineHeight;
         GetCanvas2D()->DrawText( lineXPos, lineYPos, L"  P: pause"); lineYPos += lineHeight;
         GetCanvas2D()->DrawText( lineXPos, lineYPos, L"  O: pause and do one step"); lineYPos += lineHeight;
         GetCanvas2D()->DrawText( lineXPos, lineYPos, L"  B: spring boost mode (apply 10x boost to all springs water output)"); lineYPos += lineHeight;
         GetCanvas2D()->DrawText( lineXPos, lineYPos, L"  F6: toggle between 'show water depth' and 'show water velocities' modes"); lineYPos += lineHeight;
         GetCanvas2D()->DrawText( lineXPos, lineYPos, L"  Ctrl+F11 : reload all config files"); lineYPos += lineHeight;
      }

      if( g_appMode == AM_Preview )
      {
         GetCanvas2D()->DrawText( lineXPos, lineYPos, L"Camera: yaw %.1f, pitch %.1f, pos %.1f, %.1f, %.1f", g_pPreviewCamera->GetYaw() / 3.1415 * 180, g_pPreviewCamera->GetPitch() / 3.1415 * 180, g_pPreviewCamera->GetPosition().x, g_pPreviewCamera->GetPosition().y, g_pPreviewCamera->GetPosition().z );
         lineYPos += lineHeight;
         GetCanvas2D()->DrawText( lineXPos, lineYPos, L"Visibility range: %.1f", g_pPreviewCamera->GetViewRange() );
         lineYPos += lineHeight;
         lineYPos += 6;

         GetCanvas2D()->DrawText( lineXPos, lineYPos, L"3DPreview mode keyboard commands" );   lineYPos += lineHeight;
         GetCanvas2D()->DrawText( lineXPos, lineYPos, L"  W,S,A,D,Q,Z: camera movement" ); lineYPos += lineHeight;
         GetCanvas2D()->DrawText( lineXPos, lineYPos, L"  P: pause"); lineYPos += lineHeight;
         GetCanvas2D()->DrawText( lineXPos, lineYPos, L"  O: pause and do one 60hz step"); lineYPos += lineHeight;
         GetCanvas2D()->DrawText( lineXPos, lineYPos, L"  C, V: change view range (and mesh detail level)"); lineYPos += lineHeight;
         GetCanvas2D()->DrawText( lineXPos, lineYPos, L"  Shift/Ctrl: boost/slow camera movement speed"); lineYPos += lineHeight;
         GetCanvas2D()->DrawText( lineXPos, lineYPos, L"  F1 : toggle this text menu"); lineYPos += lineHeight;
         GetCanvas2D()->DrawText( lineXPos, lineYPos, L"  F5 : toggle mode"); lineYPos += lineHeight;
         GetCanvas2D()->DrawText( lineXPos, lineYPos, L"  F8 : toggle simple render"); lineYPos += lineHeight;
         GetCanvas2D()->DrawText( lineXPos, lineYPos, L"  F9 : toggle wireframe"); lineYPos += lineHeight;
         GetCanvas2D()->DrawText( lineXPos, lineYPos, L"  Ctrl+F11 : reload all config files"); lineYPos += lineHeight;
         GetCanvas2D()->DrawText( lineXPos, lineYPos, L"  Right mouse btn: toggle mouse mode"); lineYPos += lineHeight;
         GetCanvas2D()->DrawText( lineXPos, lineYPos, L"  Left mouse btn: do simple splash"); lineYPos += lineHeight;
      }


      /*
      if( g_pTerrain->GetLastUpdateInfo().RenderQuadsCriticalPendingLoad != 0 )
      {
         //GetCanvas2D()->DrawText( lineXPos, lineYPos, "RenderQuadsCriticalPendingLoad: %d", g_pTerrain->GetLastUpdateInfo().RenderQuadsCriticalPendingLoad );
         GetCanvas2D()->DrawText( lineXPos, lineYPos, L"Streaming...." );
         //CompleteAllQueued(m_Terrain.GetAdVantage()->GetStreaming());
      }
      lineYPos += lineHeight;

      lineYPos += lineHeight;
      GetCanvas2D()->DrawText( lineXPos, lineYPos, L"Collision LOD level used, camera: %d, physics: %d", CAMERA_COLLISION_LOD_LEVEL_USED, PHYSICS_COLLISION_LOD_LEVEL_USED );
      lineYPos += lineHeight;

      //BulletPhysicsDemo * bphysics = (BulletPhysicsDemo *)g_pPhysicsDemo;
      //lineYPos += lineHeight;
      //GetCanvas2D()->DrawText( lineXPos, lineYPos, L"Bullet (www.bulletphysics.com) physics status:" ); lineYPos += lineHeight;
      //GetCanvas2D()->DrawText( lineXPos, lineYPos, L"  fixed phys fps: %d (phys steps this frame: %d)", (int)floorf(0.5f+1.0f/bphysics->GetFixedTimeStep()), bphysics->GetLastStepCount() ); lineYPos += lineHeight;
      //GetCanvas2D()->DrawText( lineXPos, lineYPos, L"  physics objects: demo bodies %d; terrain quads %d", bphysics->GetNumberOfPhysicsObject(), bphysics->GetNumberOfTerrainQuads() ); lineYPos += lineHeight;
      //GetCanvas2D()->DrawText( lineXPos, lineYPos, L"  avg CPU phys time per phys step: %.2f ms", bphysics->GetAverageStepTime() ); lineYPos += lineHeight;
      */

   }

}
