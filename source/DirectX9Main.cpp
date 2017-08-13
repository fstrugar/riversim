//--------------------------------------------------------------------------------------
// File: DirectX Example 2.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
#include "SDKmesh.h"
#include "resource.h"

#include "wsMain.h"
#include "DxEventNotifier.h"

#include "ws3DPreviewCamera.h"

#define VERTS_PER_EDGE 64

#pragma warning ( disable: 4996 )

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CDXUTDialogResourceManager    g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg               g_SettingsDlg;          // Device settings dialog
CDXUTTextHelper*              g_pTxtHelper = NULL;
CDXUTDialog                   g_HUD;                  // manages the 3D UI

extern string                 g_cmdLine;

extern wstring                g_sWindowTitle;

extern bool                   g_deviceCreated;
extern bool                   g_deviceReset;
extern D3DSURFACE_DESC        g_backBufferSrufaceDesc;

extern ws3DPreviewCamera *    g_pPreviewCamera;


struct SCENE_VERTEX
{
    D3DXVECTOR2 pos;
};

// Direct3D 9 resources
// Direct3D 9 resources
ID3DXFont*                          g_pFont = NULL;
ID3DXSprite*                        g_pTextSprite = NULL;
LPDIRECT3DVERTEXBUFFER9             g_pVB = NULL;
LPDIRECT3DINDEXBUFFER9              g_pIB = NULL;
LPDIRECT3DVERTEXSHADER9             g_pVertexShader = NULL;
LPD3DXCONSTANTTABLE                 g_pConstantTable = NULL;
LPDIRECT3DVERTEXDECLARATION9        g_pVertexDeclaration = NULL;

struct VS_CONSTANT_BUFFER
{
    D3DXMATRIX mWorldViewProj;      //mWorldViewProj will probably be global to all shaders in a project.
    //It's a good idea not to move it around between shaders.
    D3DXVECTOR4 vSomeVectorThatMayBeNeededByASpecificShader;
    float fSomeFloatThatMayBeNeededByASpecificShader;
    float fTime;                    //fTime may also be global to all shaders in a project.
    float fSomeFloatThatMayBeNeededByASpecificShader2;
    float fSomeFloatThatMayBeNeededByASpecificShader3;
};


//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN    1
#define IDC_TOGGLEREF           3
#define IDC_CHANGEDEVICE        4


//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
extern bool CALLBACK IsD3D9DeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat,
                                             bool bWindowed, void* pUserContext );
extern HRESULT CALLBACK OnD3D9CreateDevice( IDirect3DDevice9* pd3dDevice,
                                            const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
extern HRESULT CALLBACK OnD3D9ResetDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                           void* pUserContext );
extern void CALLBACK OnD3D9FrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime,
                                        void* pUserContext );
extern void CALLBACK OnD3D9LostDevice( void* pUserContext );
extern void CALLBACK OnD3D9DestroyDevice( void* pUserContext );

bool CALLBACK IsD3D10DeviceAcceptable( UINT Adapter, UINT Output, D3D10_DRIVER_TYPE DeviceType,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext );

bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext );
//void CALLBACK KeyboardProc( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );

void InitApp();
void RenderText();

extern HWND                       g_hwndMain;

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
   // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
   _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

   g_cmdLine = simple_narrow( wstring(lpCmdLine) );

   if( !wsInitialize() )
   {
      return DXUTGetExitCode();
   }

   // DXUT will create and use the best device (either D3D9 or D3D10) 
   // that is available on the system depending on which D3D callbacks are set below

   // Set DXUT callbacks
   DXUTSetCallbackMsgProc( MsgProc );
   //DXUTSetCallbackKeyboard( KeyboardProc );
   DXUTSetCallbackFrameMove( OnFrameMove );

   DXUTSetCallbackD3D9DeviceAcceptable( IsD3D9DeviceAcceptable );
   DXUTSetCallbackD3D9DeviceCreated( OnD3D9CreateDevice );
   DXUTSetCallbackD3D9DeviceReset( OnD3D9ResetDevice );
   DXUTSetCallbackD3D9FrameRender( OnD3D9FrameRender );
   DXUTSetCallbackD3D9DeviceLost( OnD3D9LostDevice );
   DXUTSetCallbackD3D9DeviceDestroyed( OnD3D9DestroyDevice );

   DXUTSetD3DVersionSupport( true, false );

   DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );

   InitApp();
   DXUTInit( true, true, NULL ); // Parse the command line, show msgboxes on error, no extra command line params
   DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
   DXUTCreateWindow( g_sWindowTitle.c_str() );

   //bool serializeDeviceSettings = false;

   //DXUTDeviceSettings deviceSettings;

   //bool hasSettings = false;

   //if( serializeDeviceSettings )
   //{
   //   FILE * f = fopen( "deviceSettings", "rb" );
   //   if( f != NULL )
   //   {
   //      size_t rs = fread( &deviceSettings, 1, sizeof(DXUTDeviceSettings), f );
   //      hasSettings = rs == sizeof(DXUTDeviceSettings);
   //      assert( hasSettings );
   //      fclose( f );

   //      deviceSettings.d3d9.pp.hDeviceWindow = DXUTGetHWNDFocus();
   //   }
   //}

   //if( hasSettings )
   //{
   //   if( SUCCEEDED( DXUTCreateDeviceFromSettings( &deviceSettings, true ) ) )
   //      hasSettings = true;
   //}

   //if( !hasSettings )
   //{
   if( FAILED( DXUTCreateDevice( true, 800, 600 ) ) )
   {
      return DXUTGetExitCode();
   }
   //}

   //if( serializeDeviceSettings )
   //{
   //   FILE * f = fopen( "deviceSettings", "wb" );
   //   if( f != NULL )
   //   {
   //      fwrite( &deviceSettings, 1, sizeof(DXUTDeviceSettings), f );
   //      fclose( f );
   //   }
   //}

   DXUTMainLoop(); // Enter into the DXUT render loop

   wsDeinitialize();

   DXUTShutdown();

   return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    g_SettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );

    g_HUD.SetCallback( OnGUIEvent ); int iY = 10;
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 35, iY, 125, 22 );
    g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 35, iY += 24, 125, 22, VK_F3 );
    g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 35, iY += 24, 125, 22, VK_F2 );
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D10 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    if( pDeviceSettings->ver == DXUT_D3D9_DEVICE )
    {
        IDirect3D9* pD3D = DXUTGetD3D9Object();
        D3DCAPS9 Caps;
        pD3D->GetDeviceCaps( pDeviceSettings->d3d9.AdapterOrdinal, pDeviceSettings->d3d9.DeviceType, &Caps );

        // If device doesn't support HW T&L or doesn't support 1.1 vertex shaders in HW 
        // then switch to SWVP.
        if( ( Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT ) == 0 ||
            Caps.VertexShaderVersion < D3DVS_VERSION( 2, 0 ) )
        {
            pDeviceSettings->d3d9.BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        }

        // Debugging vertex shaders requires either REF or software vertex processing 
        // and debugging pixel shaders requires REF.  
#ifdef DEBUG_VS
        if( pDeviceSettings->d3d9.DeviceType != D3DDEVTYPE_REF )
        {
            pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_HARDWARE_VERTEXPROCESSING;
            pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_PUREDEVICE;
            pDeviceSettings->d3d9.BehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        }
#endif
#ifdef DEBUG_PS
        pDeviceSettings->d3d9.DeviceType = D3DDEVTYPE_REF;
#endif
    }

    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( ( DXUT_D3D9_DEVICE == pDeviceSettings->ver && pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF ) ||
            ( DXUT_D3D10_DEVICE == pDeviceSettings->ver &&
              pDeviceSettings->d3d10.DriverType == D3D10_DRIVER_TYPE_REFERENCE ) )
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
    }

    return true;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
   fElapsedTime = clamp( fElapsedTime, 0.0f, 0.5f );

   wsTick( (float)fElapsedTime );

    assert( DXUTIsAppRenderingWithD3D9() );
}


//--------------------------------------------------------------------------------------
// Render the help and statistics text. This function uses the ID3DXFont interface for 
// efficient text rendering.
//--------------------------------------------------------------------------------------
void RenderText()
{
    // Output statistics
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 5, 5 );
    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );
/*
    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );

    // Draw help
    if( g_bShowHelp )
    {
        UINT nBackBufferHeight = 0;
        if( DXUTIsAppRenderingWithD3D9() )
            nBackBufferHeight = DXUTGetD3D9BackBufferSurfaceDesc()->Height;
        else
            nBackBufferHeight = DXUTGetDXGIBackBufferSurfaceDesc()->Height;

        g_pTxtHelper->SetInsertionPos( 10, nBackBufferHeight - 15 * 6 );
        g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );
        g_pTxtHelper->DrawTextLine( L"Controls (F1 to hide):" );

        g_pTxtHelper->SetInsertionPos( 40, nBackBufferHeight - 15 * 5 );
        g_pTxtHelper->DrawTextLine( L"Rotate model: Left mouse button\n"
                                    L"Rotate camera: Right mouse button\n"
                                    L"Zoom camera: Mouse wheel scroll\n"
                                    L"Hide help: F1" );
    }
    else
    {
        g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
        g_pTxtHelper->DrawTextLine( L"Press F1 for help, F5 to change between simulator<->3d_preview modes" );
    }
    */

    g_pTxtHelper->End();
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext )
{

    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    wsMsgProc( hWnd, uMsg, wParam, lParam, pbNoFurtherProcessing );
    if( *pbNoFurtherProcessing )
       return 0;

    // Pass all remaining windows messages to camera so it can respond to user input
    // g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}

//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    switch( nControlID )
    {
        case IDC_TOGGLEFULLSCREEN:
            DXUTToggleFullScreen(); break;
        case IDC_TOGGLEREF:
            DXUTToggleREF(); break;
        case IDC_CHANGEDEVICE:
            g_SettingsDlg.SetActive( !g_SettingsDlg.IsActive() ); break;
    }
}


//--------------------------------------------------------------------------------------
// Reject any D3D10 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D10DeviceAcceptable( UINT Adapter, UINT Output, D3D10_DRIVER_TYPE DeviceType,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    return false;
}

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
extern bool CALLBACK IsD3D9DeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat,
                                            bool bWindowed, void* pUserContext );
extern HRESULT CALLBACK OnD3D9CreateDevice( IDirect3DDevice9* pd3dDevice,
                                           const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
extern HRESULT CALLBACK OnD3D9ResetDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                          void* pUserContext );
extern void CALLBACK OnD3D9FrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime,
                                       void* pUserContext );
extern void CALLBACK OnD3D9LostDevice( void* pUserContext );
extern void CALLBACK OnD3D9DestroyDevice( void* pUserContext );

extern void RenderText();

extern void wsDrawCanvas2D();
extern void wsDrawCanvas3D( ws3DPreviewCamera * camera );


//--------------------------------------------------------------------------------------
// Rejects any D3D9 devices that aren't acceptable to the app by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D9DeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat,
                                     D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
   //if( pCaps->DeviceType != D3DDEVTYPE_HAL )
   //   return false;

   // Skip backbuffer formats that don't support alpha blending
   IDirect3D9* pD3D = DXUTGetD3D9Object();
   if( FAILED( pD3D->CheckDeviceFormat( pCaps->AdapterOrdinal, pCaps->DeviceType,
      AdapterFormat, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING,
      D3DRTYPE_TEXTURE, BackBufferFormat ) ) )
   {
      return false;
   }

   //if( FAILED( pD3D->CheckDeviceFormat( pCaps->AdapterOrdinal, pCaps->DeviceType,
   //   D3DFMT_R32F, D3DUSAGE_RENDERTARGET | D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING,
   //   D3DRTYPE_SURFACE, BackBufferFormat ) ) )
   //{
   //   return false;
   //}

   return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D9 resources that will live through a device reset (D3DPOOL_MANAGED)
// and aren't tied to the back buffer size
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D9CreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                    void* pUserContext )
{
   HRESULT hr;

   g_deviceCreated = true;

   g_hwndMain = DXUTGetHWND();

   V_RETURN( DxEventNotifier::PostCreateDevice(pd3dDevice) );

   V_RETURN( g_DialogResourceManager.OnD3D9CreateDevice( pd3dDevice ) );
   V_RETURN( g_SettingsDlg.OnD3D9CreateDevice( pd3dDevice ) );
   V_RETURN( D3DXCreateFont( pd3dDevice, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
      OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
      L"Arial", &g_pFont ) );

   return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D9 resources that won't live through a device reset (D3DPOOL_DEFAULT) 
// or that are tied to the back buffer size 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D9ResetDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                   void* pUserContext )
{
   HRESULT hr;

   g_deviceReset = true;
   g_backBufferSrufaceDesc = *pBackBufferSurfaceDesc;

   g_hwndMain = DXUTGetHWND();

   g_pPreviewCamera->SetAspect( pBackBufferSurfaceDesc->Width / (float) pBackBufferSurfaceDesc->Height );

   V_RETURN( DxEventNotifier::PostResetDevice(pBackBufferSurfaceDesc) );

   V_RETURN( g_DialogResourceManager.OnD3D9ResetDevice() );
   V_RETURN( g_SettingsDlg.OnD3D9ResetDevice() );

   if( g_pFont )
      V_RETURN( g_pFont->OnResetDevice() );

   // Create a sprite to help batch calls when drawing many lines of text
   V_RETURN( D3DXCreateSprite( pd3dDevice, &g_pTextSprite ) );
   g_pTxtHelper = new CDXUTTextHelper( g_pFont, g_pTextSprite, NULL, NULL, 15 );

   pd3dDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
   pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );

   g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
   g_HUD.SetSize( 170, 170 );

   return S_OK;
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D9 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9FrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext )
{
   fElapsedTime = clamp( fElapsedTime, 0.0f, 0.5f );

   // If the settings dialog is being shown, then render it instead of rendering the app's scene
   if( g_SettingsDlg.IsActive() )
   {
      g_SettingsDlg.OnRender( fElapsedTime );
      return;
   }

   HRESULT hr;

   // Clear the render target and the zbuffer 
   V( pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB( 0, 64, 128, 192 ), 1.0f, 0 ) ); //D3DCOLOR_ARGB( 0, 45, 50, 170 )

   // Render the scene
   if( SUCCEEDED( pd3dDevice->BeginScene() ) )
   {
      /*
      pd3dDevice->SetVertexDeclaration( g_pVertexDeclaration );
      pd3dDevice->SetVertexShader( g_pVertexShader );
      pd3dDevice->SetStreamSource( 0, g_pVB, 0, sizeof( D3DXVECTOR2 ) );
      pd3dDevice->SetIndices( g_pIB );
      V( pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, g_dwNumVertices, 0, g_dwNumIndices / 3 ) );
      */

      V( wsRender( fElapsedTime ) );

      RenderText();
      wsDrawCanvas2D();

      V( g_HUD.OnRender( fElapsedTime ) );
      V( pd3dDevice->EndScene() );
   }
}


//--------------------------------------------------------------------------------------
// Release D3D9 resources created in the OnD3D9ResetDevice callback 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9LostDevice( void* pUserContext )
{
   g_deviceReset = false;

   DxEventNotifier::PostLostDevice();

   g_DialogResourceManager.OnD3D9LostDevice();
   g_SettingsDlg.OnD3D9LostDevice();
   if( g_pFont )
      g_pFont->OnLostDevice();

   SAFE_RELEASE( g_pIB );
   SAFE_RELEASE( g_pVB );
   SAFE_RELEASE( g_pTextSprite );
   SAFE_DELETE( g_pTxtHelper );
}


//--------------------------------------------------------------------------------------
// Release D3D9 resources created in the OnD3D9CreateDevice callback 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9DestroyDevice( void* pUserContext )
{
   g_deviceCreated = false;

   DxEventNotifier::PostDestroyDevice();

   g_DialogResourceManager.OnD3D9DestroyDevice();
   g_SettingsDlg.OnD3D9DestroyDevice();
   SAFE_RELEASE( g_pFont );
   SAFE_RELEASE( g_pVertexShader );
   SAFE_RELEASE( g_pConstantTable );
   SAFE_RELEASE( g_pVertexDeclaration );
}

