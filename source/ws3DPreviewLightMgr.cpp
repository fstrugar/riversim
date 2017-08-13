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

#pragma warning ( disable : 4995 4996 )

#include "DXUT.h"

#include "ws3DPreviewLightMgr.h"
#include "wsCommon.h"

//
ws3DPreviewLightMgr::ws3DPreviewLightMgr()
{
   m_directionalLightDir   = D3DXVECTOR3( c_diffuseLightDir[0], c_diffuseLightDir[1], c_diffuseLightDir[2] );

   m_directionalLightDirTarget   = m_directionalLightDir;
   m_directionalLightDirTargetL1 = m_directionalLightDirTarget;

   m_sunMesh = NULL;

   m_useSkybox = true;
}
//
ws3DPreviewLightMgr::~ws3DPreviewLightMgr()
{
   ws3DPreviewLightMgr::OnDestroyDevice();
}
//
void ws3DPreviewLightMgr::Tick( float deltaTime )
{
   static float zmmzmz = 10.0f;
   float lf = timeIndependentLerpF( deltaTime, zmmzmz );

   bool change = false;
   if( IsKeyClicked( kcSetLight1 ) )   { m_directionalLightDirTarget = D3DXVECTOR3( -1,  1,  0 ); change = true; }
   if( IsKeyClicked( kcSetLight2 ) )   { m_directionalLightDirTarget = D3DXVECTOR3(  0,  1,  0 ); change = true; }
   if( IsKeyClicked( kcSetLight3 ) )   { m_directionalLightDirTarget = D3DXVECTOR3(  1,  1,  0 ); change = true; }
   if( IsKeyClicked( kcSetLight4 ) )   { m_directionalLightDirTarget = D3DXVECTOR3( -1,  0,  0 ); change = true; }
   if( IsKeyClicked( kcSetLight5 ) )   { m_directionalLightDirTarget = D3DXVECTOR3(  0,  0, -1 ); change = true; }
   if( IsKeyClicked( kcSetLight6 ) )   { m_directionalLightDirTarget = D3DXVECTOR3(  1,  0,  0 ); change = true; }
   if( IsKeyClicked( kcSetLight7 ) )   { m_directionalLightDirTarget = D3DXVECTOR3( -1, -1,  0 ); change = true; }
   if( IsKeyClicked( kcSetLight8 ) )   { m_directionalLightDirTarget = D3DXVECTOR3(  0, -1,  0 ); change = true; }
   if( IsKeyClicked( kcSetLight9 ) )   { m_directionalLightDirTarget = D3DXVECTOR3(  1, -1,  0 ); change = true; }
   
   if( IsKeyClicked( kcSetLight0 ) )   { m_useSkybox = true; }
   
   if( change )
   {
      D3DXVec3Normalize( &m_directionalLightDirTarget, &m_directionalLightDirTarget );
      m_directionalLightDirTarget.z = -0.5f;
      D3DXVec3Normalize( &m_directionalLightDirTarget, &m_directionalLightDirTarget );
      m_useSkybox = false;
   }

   if( m_useSkybox )
   {
      m_directionalLightDirTarget = m_skyBox.GetDirectionalLightDir();
   }

   D3DXVec3Lerp( &m_directionalLightDirTargetL1, &m_directionalLightDirTargetL1, &m_directionalLightDirTarget, lf );
   D3DXVec3Lerp( &m_directionalLightDir, &m_directionalLightDir, &m_directionalLightDirTargetL1, lf );
}
//
/*
static D3DXMATRIX ToD3DXMATRIX( const AdVantage::aMatrix44 & m )
{
   D3DXMATRIX ret;
   memcpy( (FLOAT*)ret, (const float*)m, sizeof(m) );
   return ret;
}
*/
//
HRESULT ws3DPreviewLightMgr::Render( ws3DPreviewCamera * pCamera )
{
   if( m_useSkybox )
   {
      m_skyBox.Render( pCamera, this );
   }

   IDirect3DDevice9 * pDevice = DxEventNotifier::GetD3DDevice();

   D3DXMATRIX view = *pCamera->GetViewMatrix();
   D3DXMATRIX proj = *pCamera->GetProjMatrix();

   //D3DXMATRIX proj;
   //D3DXMatrixPerspectiveFovLH( &proj, pCamera->GetFOV(), pCamera->GetAspect(), 10.0f, 2e8 );

   view._41 = 0.0; view._42 = 0.0; view._43 = 0.0;
   //D3DXMATRIX viewProj = view * proj;

   D3DXMATRIXA16 matWorld;
   D3DXMatrixTranslation( &matWorld, m_directionalLightDir.x * -1e3f, m_directionalLightDir.y * -1e3f, m_directionalLightDir.z * -1e3f );
   pDevice->SetTransform( D3DTS_WORLD, &matWorld );

   pDevice->SetTransform( D3DTS_VIEW, &view );
   pDevice->SetTransform( D3DTS_PROJECTION, &proj );

   D3DMATERIAL9 sunMat;
   sunMat.Ambient.r = 1.0f; sunMat.Ambient.g = 1.0f; sunMat.Ambient.b = 1.0f; sunMat.Ambient.a = 1.0f;
   sunMat.Diffuse.r = 1.0f; sunMat.Diffuse.g = 1.0f; sunMat.Diffuse.b = 1.0f; sunMat.Diffuse.a = 1.0f;
   sunMat.Emissive.r = 1.0f; sunMat.Emissive.g = 1.0f; sunMat.Emissive.b = 1.0f; sunMat.Emissive.a = 1.0f;
   sunMat.Specular.r = 1.0f; sunMat.Specular.g = 1.0f; sunMat.Specular.b = 1.0f; sunMat.Specular.a = 1.0f;
   sunMat.Power = 1.0f;

   pDevice->SetFVF( m_sunMesh->GetFVF() );

   pDevice->SetMaterial( &sunMat );

   pDevice->SetTexture( 0, NULL );

   pDevice->SetVertexShader( NULL );
   pDevice->SetPixelShader( NULL );
   pDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
   pDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );

   m_sunMesh->DrawSubset( 0 );

   return S_OK;
}
//
HRESULT ws3DPreviewLightMgr::OnCreateDevice( )
{
   HRESULT hr;

   IDirect3DDevice9 * pDevice = DxEventNotifier::GetD3DDevice();

   hr = D3DXCreateSphere( pDevice, 3e1f, 9, 9, &m_sunMesh, NULL );

   V( m_skyBox.OnCreateDevice() );

   return hr;
}
//
void ws3DPreviewLightMgr::OnDestroyDevice( )
{
   if( m_sunMesh != NULL )
   {
      m_sunMesh->Release();
      m_sunMesh = NULL;
   }
   m_skyBox.OnDestroyDevice();
}
//
