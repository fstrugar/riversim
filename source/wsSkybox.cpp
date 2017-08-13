#include "DXUT.h"

#include "wsCommon.h"

#include "wsSkybox.h"

#include "DxEventNotifier.h"

#include "ws3DPreviewCamera.h"
#include "ws3DPreviewLightMgr.h"

wsSkybox::wsSkybox()
{
   m_skyCubemap = NULL;

   m_vsSkybox.SetShaderInfo( "wsSkybox.sh", "vsSkybox" );
   m_psSkybox.SetShaderInfo( "wsSkybox.sh", "psSkybox" );
}
//
void wsSkybox::Render( ws3DPreviewCamera * camera, ws3DPreviewLightMgr * lightMgr )
{
   IDirect3DDevice9 * device = DxEventNotifier::GetD3DDevice();

   D3DXMATRIX view = *camera->GetViewMatrix();
   const D3DXMATRIX & proj = *camera->GetProjMatrix();
   //
   view._41 = 0.0; view._42 = 0.0; view._43 = 0.0;
   D3DXMATRIX viewProj = view * proj;

   m_vsSkybox.SetMatrix( "g_worldViewProjection", viewProj );
   m_psSkybox.SetTexture( "g_skyCubeMapTexture", m_skyCubemap, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR );

   //int cubemapSamplerIndex = m_constantTable->GetSamplerIndex( "g_skyCubeMapTexture" );
   //if( cubemapSamplerIndex != -1 )
   //{
   //   device->SetTexture()
   //}

   float dist = 1e3f;

   D3DXVECTOR3 boxMin = D3DXVECTOR3( -dist, -dist, -dist );
   D3DXVECTOR3 boxMax = D3DXVECTOR3( dist, dist, dist );

   D3DXVECTOR3 a0 = D3DXVECTOR3( boxMin.x, boxMin.y, boxMin.z );
   D3DXVECTOR3 a1 = D3DXVECTOR3( boxMax.x, boxMin.y, boxMin.z );
   D3DXVECTOR3 a2 = D3DXVECTOR3( boxMax.x, boxMax.y, boxMin.z );
   D3DXVECTOR3 a3 = D3DXVECTOR3( boxMin.x, boxMax.y, boxMin.z );
   D3DXVECTOR3 b0 = D3DXVECTOR3( boxMin.x, boxMin.y, boxMax.z );
   D3DXVECTOR3 b1 = D3DXVECTOR3( boxMax.x, boxMin.y, boxMax.z );
   D3DXVECTOR3 b2 = D3DXVECTOR3( boxMax.x, boxMax.y, boxMax.z );
   D3DXVECTOR3 b3 = D3DXVECTOR3( boxMin.x, boxMax.y, boxMax.z );

   PositionVertex vertices[36];
   int index = 0;

   vertices[index++] = PositionVertex( a0 ); vertices[index++] = PositionVertex( a1 ); vertices[index++] = PositionVertex( a2 );
   vertices[index++] = PositionVertex( a2 ); vertices[index++] = PositionVertex( a3 ); vertices[index++] = PositionVertex( a0 );
   vertices[index++] = PositionVertex( b0 ); vertices[index++] = PositionVertex( b1 ); vertices[index++] = PositionVertex( b2 );
   vertices[index++] = PositionVertex( b2 ); vertices[index++] = PositionVertex( b3 ); vertices[index++] = PositionVertex( b0 );

   vertices[index++] = PositionVertex( a0 ); vertices[index++] = PositionVertex( a1 ); vertices[index++] = PositionVertex( b1 );
   vertices[index++] = PositionVertex( b1 ); vertices[index++] = PositionVertex( b0 ); vertices[index++] = PositionVertex( a0 );
   vertices[index++] = PositionVertex( a3 ); vertices[index++] = PositionVertex( a2 ); vertices[index++] = PositionVertex( b2 );
   vertices[index++] = PositionVertex( b2 ); vertices[index++] = PositionVertex( b3 ); vertices[index++] = PositionVertex( a3 );

   vertices[index++] = PositionVertex( a1 ); vertices[index++] = PositionVertex( a2 ); vertices[index++] = PositionVertex( b2 );
   vertices[index++] = PositionVertex( b2 ); vertices[index++] = PositionVertex( b1 ); vertices[index++] = PositionVertex( a1 );
   vertices[index++] = PositionVertex( a0 ); vertices[index++] = PositionVertex( a3 ); vertices[index++] = PositionVertex( b3 );
   vertices[index++] = PositionVertex( b3 ); vertices[index++] = PositionVertex( b0 ); vertices[index++] = PositionVertex( a0 );

   device->SetVertexShader( m_vsSkybox );
   device->SetPixelShader( m_psSkybox );

   device->SetFVF( PositionVertex::FVF );
   device->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
   device->SetRenderState( D3DRS_ZENABLE, FALSE );
   device->DrawPrimitiveUP( D3DPT_TRIANGLELIST, index/3, vertices, sizeof( PositionVertex ) );
   device->SetRenderState( D3DRS_ZENABLE, TRUE );
   device->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
}
//
/*
private void RenderSkyBox( Device device, Effect effect )
{
   device.VertexFormat = VertexFormats.Position;
   effect.Technique = "SkyBox";

   CustomVertex.PositionOnly[] vertices = new CustomVertex.PositionOnly[ 6 * 6 ];

   AABox box = new AABox( new Vector3(-100, -100, -100), new Vector3(100, 100, 100) );

   Vector3 a0 = new Vector3( box.Min.X, box.Min.Y, box.Min.Z );
   Vector3 a1 = new Vector3( box.Max.X, box.Min.Y, box.Min.Z );
   Vector3 a2 = new Vector3( box.Max.X, box.Max.Y, box.Min.Z );
   Vector3 a3 = new Vector3( box.Min.X, box.Max.Y, box.Min.Z );
   Vector3 b0 = new Vector3( box.Min.X, box.Min.Y, box.Max.Z );
   Vector3 b1 = new Vector3( box.Max.X, box.Min.Y, box.Max.Z );
   Vector3 b2 = new Vector3( box.Max.X, box.Max.Y, box.Max.Z );
   Vector3 b3 = new Vector3( box.Min.X, box.Max.Y, box.Max.Z );

   int index = 0;
   vertices[index++] = new CustomVertex.PositionOnly( a0 ); vertices[index++] = new CustomVertex.PositionOnly( a1 ); vertices[index++] = new CustomVertex.PositionOnly( a2 );
   vertices[index++] = new CustomVertex.PositionOnly( a2 ); vertices[index++] = new CustomVertex.PositionOnly( a3 ); vertices[index++] = new CustomVertex.PositionOnly( a0 );
   vertices[index++] = new CustomVertex.PositionOnly( b0 ); vertices[index++] = new CustomVertex.PositionOnly( b1 ); vertices[index++] = new CustomVertex.PositionOnly( b2 );
   vertices[index++] = new CustomVertex.PositionOnly( b2 ); vertices[index++] = new CustomVertex.PositionOnly( b3 ); vertices[index++] = new CustomVertex.PositionOnly( b0 );

   vertices[index++] = new CustomVertex.PositionOnly( a0 ); vertices[index++] = new CustomVertex.PositionOnly( a1 ); vertices[index++] = new CustomVertex.PositionOnly( b1 );
   vertices[index++] = new CustomVertex.PositionOnly( b1 ); vertices[index++] = new CustomVertex.PositionOnly( b0 ); vertices[index++] = new CustomVertex.PositionOnly( a0 );
   vertices[index++] = new CustomVertex.PositionOnly( a3 ); vertices[index++] = new CustomVertex.PositionOnly( a2 ); vertices[index++] = new CustomVertex.PositionOnly( b2 );
   vertices[index++] = new CustomVertex.PositionOnly( b2 ); vertices[index++] = new CustomVertex.PositionOnly( b3 ); vertices[index++] = new CustomVertex.PositionOnly( a3 );

   vertices[index++] = new CustomVertex.PositionOnly( a1 ); vertices[index++] = new CustomVertex.PositionOnly( a2 ); vertices[index++] = new CustomVertex.PositionOnly( b2 );
   vertices[index++] = new CustomVertex.PositionOnly( b2 ); vertices[index++] = new CustomVertex.PositionOnly( b1 ); vertices[index++] = new CustomVertex.PositionOnly( a1 );
   vertices[index++] = new CustomVertex.PositionOnly( a0 ); vertices[index++] = new CustomVertex.PositionOnly( a3 ); vertices[index++] = new CustomVertex.PositionOnly( b3 );
   vertices[index++] = new CustomVertex.PositionOnly( b3 ); vertices[index++] = new CustomVertex.PositionOnly( b0 ); vertices[index++] = new CustomVertex.PositionOnly( a0 );

   int passes = effect.Begin( 0 );
   for( int pass = 0; pass < passes; pass++ )
   {
      effect.BeginPass( pass );

      device.DrawUserPrimitives(PrimitiveType.TriangleList, vertices.Length/3, vertices );
      //mesh.DrawSubset(0);

      effect.EndPass();
   }
   effect.End();
}*/
//
void wsSkybox::OnDestroyDevice()
{
   SAFE_RELEASE( m_skyCubemap );
}

HRESULT wsSkybox::OnCreateDevice()
{
   HRESULT hr;
   IDirect3DDevice9 * device = DxEventNotifier::GetD3DDevice();

   V( D3DXCreateCubeTextureFromFile( device, wsFindResource(L"Media\\skyCube.dds").c_str(), &m_skyCubemap ) );

   m_directionalLightDir = D3DXVECTOR3( -0.64959717f, 0.64959717f, -0.39502779f );
   m_directionalLightDir.z -= 0.8f;
   m_directionalLightDir.x += 0.15f;

   D3DXVec3Normalize( &m_directionalLightDir, &m_directionalLightDir );

   return S_OK;
}
