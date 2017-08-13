#include "DXUT.h"

#include "wsCommon.h"
#include "DxEventNotifier.h"

#include "wsShader.h"

void wsSetVertexTexture( IDirect3DTexture9 * texture, const char * name, ID3DXConstantTable * constants )
{
   IDirect3DDevice9 * device = DxEventNotifier::GetD3DDevice();

   int samplerIndex = constants->GetSamplerIndex( name );
   if( samplerIndex != -1 )
   {
      device->SetTexture( D3DVERTEXTEXTURESAMPLER0 + samplerIndex, texture );
      device->SetSamplerState( D3DVERTEXTEXTURESAMPLER0 + samplerIndex, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
      device->SetSamplerState( D3DVERTEXTEXTURESAMPLER0 + samplerIndex, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
      device->SetSamplerState( D3DVERTEXTEXTURESAMPLER0 + samplerIndex, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
      device->SetSamplerState( D3DVERTEXTEXTURESAMPLER0 + samplerIndex, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
      //device->SetSamplerState( D3DVERTEXTEXTURESAMPLER0 + samplerIndex, D3DSAMP_MIPFILTER, mipFilter );
   }
}


void wsSetTexture( IDirect3DTexture9 * texture, const char * name, ID3DXConstantTable * constants, DWORD addrU, DWORD addrV, DWORD minFilter, DWORD magFilter, DWORD mipFilter )
{
   IDirect3DDevice9 * device = DxEventNotifier::GetD3DDevice();

   int samplerIndex = constants->GetSamplerIndex( name );
   if( samplerIndex != -1 )
   {
      device->SetTexture( samplerIndex, texture );
      device->SetSamplerState( samplerIndex, D3DSAMP_ADDRESSU, addrU );
      device->SetSamplerState( samplerIndex, D3DSAMP_ADDRESSV, addrV );
      device->SetSamplerState( samplerIndex, D3DSAMP_MINFILTER, minFilter );
      device->SetSamplerState( samplerIndex, D3DSAMP_MAGFILTER, magFilter );
      device->SetSamplerState( samplerIndex, D3DSAMP_MIPFILTER, mipFilter );
   }
}

void wsGetFrustumPlanes( D3DXPLANE * pPlanes, const D3DXMATRIX & mCameraViewProj )
{
   // Left clipping plane
   pPlanes[0].a = mCameraViewProj(0,3) + mCameraViewProj(0,0);
   pPlanes[0].b = mCameraViewProj(1,3) + mCameraViewProj(1,0);
   pPlanes[0].c = mCameraViewProj(2,3) + mCameraViewProj(2,0);
   pPlanes[0].d = mCameraViewProj(3,3) + mCameraViewProj(3,0);

   // Right clipping plane
   pPlanes[1].a = mCameraViewProj(0,3) - mCameraViewProj(0,0);
   pPlanes[1].b = mCameraViewProj(1,3) - mCameraViewProj(1,0);
   pPlanes[1].c = mCameraViewProj(2,3) - mCameraViewProj(2,0);
   pPlanes[1].d = mCameraViewProj(3,3) - mCameraViewProj(3,0);

   // Top clipping plane
   pPlanes[2].a = mCameraViewProj(0,3) - mCameraViewProj(0,1);
   pPlanes[2].b = mCameraViewProj(1,3) - mCameraViewProj(1,1);
   pPlanes[2].c = mCameraViewProj(2,3) - mCameraViewProj(2,1);
   pPlanes[2].d = mCameraViewProj(3,3) - mCameraViewProj(3,1);

   // Bottom clipping plane
   pPlanes[3].a = mCameraViewProj(0,3) + mCameraViewProj(0,1);
   pPlanes[3].b = mCameraViewProj(1,3) + mCameraViewProj(1,1);
   pPlanes[3].c = mCameraViewProj(2,3) + mCameraViewProj(2,1);
   pPlanes[3].d = mCameraViewProj(3,3) + mCameraViewProj(3,1);

   // Near clipping plane
   pPlanes[4].a = mCameraViewProj(0,2);
   pPlanes[4].b = mCameraViewProj(1,2);
   pPlanes[4].c = mCameraViewProj(2,2);
   pPlanes[4].d = mCameraViewProj(3,2);

   // Far clipping plane
   pPlanes[5].a = mCameraViewProj(0,3) - mCameraViewProj(0,2);
   pPlanes[5].b = mCameraViewProj(1,3) - mCameraViewProj(1,2);
   pPlanes[5].c = mCameraViewProj(2,3) - mCameraViewProj(2,2);
   pPlanes[5].d = mCameraViewProj(3,3) - mCameraViewProj(3,2);
   // Normalize the plane equations, if requested

   for (int i = 0; i < 6; i++) 
      D3DXPlaneNormalize( &pPlanes[i], &pPlanes[i] );
}
//
HRESULT wsRenderSimulation( int fromX, int fromY, int toX, int toY, int texWidth, int texHeight )
{
   HRESULT hr;

   IDirect3DDevice9* device = DxEventNotifier::GetD3DDevice();

   TransofmedTexturedVertex vertices[4];

   float uminx = fromX / (float)(texWidth-1);
   float uminy = fromY / (float)(texHeight-1); 
   float umaxx = (toX) / (float)(texWidth-1);
   float umaxy = (toY) / (float)(texHeight-1);

   vertices[0] = TransofmedTexturedVertex( -0.5f + fromX,      -0.5f + fromY,       0.5f, 1.0f, uminx, uminy );
   vertices[1] = TransofmedTexturedVertex( (toX) - 0.5f,       -0.5f + fromY,       0.5f, 1.0f, umaxx, uminy );
   vertices[2] = TransofmedTexturedVertex( -0.5f + fromX,      (toY) - 0.5f,        0.5f, 1.0f, uminx, umaxy );
   vertices[3] = TransofmedTexturedVertex( (toX) - 0.5f ,      (toY) - 0.5f,        0.5f, 1.0f, umaxx, umaxy );

   V_RETURN( device->SetRenderState( D3DRS_ZENABLE, FALSE ) );
   V_RETURN( device->SetFVF( TransofmedTexturedVertex::FVF ) );

   //device->SetRenderState( D3DRS_ZENABLE, FALSE );
   //device->SetRenderState( D3DRS_LIGHTING, FALSE );

   V_RETURN( device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, vertices, sizeof( TransofmedTexturedVertex ) ) );

   //device->SetRenderState( D3DRS_LIGHTING, FALSE );
   V_RETURN( device->SetRenderState( D3DRS_ZENABLE, TRUE ) );

   return S_OK;
}
//
HRESULT wsRenderSimulation( int width, int height, int cutEdge )
{
   HRESULT hr;

   IDirect3DDevice9* device = DxEventNotifier::GetD3DDevice();

   TransofmedTexturedVertex vertices[4];

   float fCutEdge = (float)cutEdge;

   float uminx = 0 + fCutEdge / (float)(width-1);
   float uminy = 0 + fCutEdge / (float)(height-1); 
   float umaxx = 1 - fCutEdge / (float)(width-1);
   float umaxy = 1 - fCutEdge / (float)(height-1); 

   vertices[0] = TransofmedTexturedVertex( -0.5f + fCutEdge,         -0.5f + fCutEdge,         0.5f, 1.0f, uminx, uminy );
   vertices[1] = TransofmedTexturedVertex( width - 0.5f - fCutEdge,  -0.5f + fCutEdge,         0.5f, 1.0f, umaxx, uminy );
   vertices[2] = TransofmedTexturedVertex( -0.5f + fCutEdge,         height - 0.5f - fCutEdge, 0.5f, 1.0f, uminx, umaxy );
   vertices[3] = TransofmedTexturedVertex( width - 0.5f - fCutEdge,  height - 0.5f - fCutEdge, 0.5f, 1.0f, umaxx, umaxy );

   for( int i = 0; i < 4; i++ )
   {
      vertices[i].x += 0;
      vertices[i].y += 0;
   }

   V_RETURN( device->SetRenderState( D3DRS_ZENABLE, FALSE ) );
   V_RETURN( device->SetFVF( TransofmedTexturedVertex::FVF ) );

   //device->SetRenderState( D3DRS_ZENABLE, FALSE );
   //device->SetRenderState( D3DRS_LIGHTING, FALSE );

   V_RETURN( device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, vertices, sizeof( TransofmedTexturedVertex ) ) );

   //device->SetRenderState( D3DRS_LIGHTING, FALSE );
   V_RETURN( device->SetRenderState( D3DRS_ZENABLE, TRUE ) );

   return S_OK;
}
//
HRESULT wsSetRenderTarget( int index, IDirect3DTexture9 * texture )
{
   IDirect3DSurface9 * surface = NULL;

   HRESULT hr;
   if( texture != NULL )
   {
      V_RETURN( texture->GetSurfaceLevel( 0, &surface ) );
   }

   V_RETURN( DxEventNotifier::GetD3DDevice()->SetRenderTarget( index, surface ) );

   SAFE_RELEASE( surface );

   return S_OK;
}
//
HRESULT wsStretchRect( IDirect3DTexture9* pSourceTexture, CONST RECT* pSourceRect, IDirect3DTexture9* pDestTexture, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter)
{
   HRESULT hr;
   IDirect3DSurface9 * sourceSurface, * destSurface;
   V( pSourceTexture->GetSurfaceLevel( 0, &sourceSurface ) );
   V( pDestTexture->GetSurfaceLevel( 0, &destSurface ) );

   hr = DxEventNotifier::GetD3DDevice()->StretchRect( sourceSurface, pSourceRect, destSurface, pDestRect, Filter );

   SAFE_RELEASE( sourceSurface );
   SAFE_RELEASE( destSurface );

   V( hr );

   return hr;
}
//
HRESULT wsGetRenderTargetData( IDirect3DTexture9 * srcTexture, IDirect3DTexture9 * destTexture )
{
   HRESULT hr;
   IDirect3DSurface9 * sourceSurface, * destSurface;
   V( srcTexture->GetSurfaceLevel( 0, &sourceSurface ) );
   V( destTexture->GetSurfaceLevel( 0, &destSurface ) );

   hr = DxEventNotifier::GetD3DDevice()->GetRenderTargetData( sourceSurface, destSurface );

   SAFE_RELEASE( sourceSurface );
   SAFE_RELEASE( destSurface );

   V( hr );

   return hr;
}
//
HRESULT wsFillColor( D3DXVECTOR4 color, IDirect3DTexture9 * texture )
{
   HRESULT hr;
   IDirect3DDevice9* device = DxEventNotifier::GetD3DDevice();

   static wsPixelShader psFillColor( "wsCommon.psh", "justFillColor" );

   V_RETURN( device->SetPixelShader(psFillColor) );
   V_RETURN( psFillColor.SetVector( "g_justFillColor_Color", color ) );

   D3DSURFACE_DESC desc;
   texture->GetLevelDesc( 0, &desc );

   V_RETURN( wsRenderSimulation( desc.Width, desc.Height, 0 ) );

   return S_OK;
}
//