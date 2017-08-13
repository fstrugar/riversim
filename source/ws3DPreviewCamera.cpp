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

//#include "Memory.h"

#include "ws3DPreviewCamera.h"

#include "wsCommon.h"

using namespace std;

#pragma warning( disable : 4996 )

static void SerializeCamera( D3DXVECTOR3 & pos, float & yaw, float & pitch, float & speed, float & viewRange, bool load )
{
   string exePath = wsGetProjectDirectory();

   exePath += "\\lastCameraPosRot.caminfo";

   if( load )
   {
      FILE * file = fopen( exePath.c_str(), "rb" );
      if( file == NULL )
         return;

      int ver;
      if( fread( &ver, sizeof(ver), 1, file ) != 1 )
         goto loadErrorRet;

      if( ver != 1 )
         goto loadErrorRet;

      if( fread( &pos, sizeof(pos), 1, file ) != 1 )
         goto loadErrorRet;

      if( fread( &yaw, sizeof(yaw), 1, file ) != 1 )
         goto loadErrorRet;

      if( fread( &pitch, sizeof(pitch), 1, file ) != 1 )
         goto loadErrorRet;

      if( fread( &speed, sizeof(speed), 1, file ) != 1 )
         goto loadErrorRet;

      if( fread( &viewRange, sizeof(viewRange), 1, file ) != 1 )
         goto loadErrorRet;

      loadErrorRet:
      fclose( file );
   }
   else
   {
      FILE * file = fopen( exePath.c_str(), "wb" );
      if( file == NULL )
         return;

      fseek( file, 0, SEEK_SET );

      int ver = 1;
      if( fwrite( &ver, sizeof(ver), 1, file ) != 1 )
         goto saveErrorRet;

      if( fwrite( &pos, sizeof(pos), 1, file ) != 1 )
         goto saveErrorRet;

      if( fwrite( &yaw, sizeof(yaw), 1, file ) != 1 )
         goto saveErrorRet;

      if( fwrite( &pitch, sizeof(pitch), 1, file ) != 1 )
         goto saveErrorRet;

      if( fwrite( &speed, sizeof(speed), 1, file ) != 1 )
         goto saveErrorRet;

      if( fwrite( &viewRange, sizeof(viewRange), 1, file ) != 1 )
         goto saveErrorRet;

      saveErrorRet:
      fclose( file );
   }

}

ws3DPreviewCamera::ws3DPreviewCamera()
{
   m_fFOV               = 3.1415f / 3.0f;
   m_fAspect            = 1.0f;
   m_fNear              = 0.1f;
   m_fFar               = 10.0f;
   //
   m_fYaw               = 0.88f;
   m_fPitch             = 0.5f;
   m_vPos               = D3DXVECTOR3( -37747.660f, -39706.039f, 37016.523f );
   //
   D3DXMatrixIdentity( &m_mView );
   D3DXMatrixIdentity( &m_mProj );
   //
   m_fSpeed             = 3000.0f;
   m_fViewRange         = 120000.0f;
   //m_fViewRange         = 100000.0f;
   //
   m_fSpeedKeyMod       = 0.0f;
   //
   {
      D3DXMATRIX mCameraRot;
      D3DXMATRIX mRotationY; D3DXMatrixRotationY( &mRotationY, m_fPitch );
      D3DXMATRIX mRotationZ; D3DXMatrixRotationZ( &mRotationZ, m_fYaw );
      //
      mCameraRot = mRotationY * mRotationZ;
      m_vDir = D3DXVECTOR3( mCameraRot.m[0] );
      //
      wsGetProjectDefaultCameraParams( m_vPos, m_vDir, m_fViewRange );
   }
   //
#ifdef _DEBUG
   SerializeCamera( m_vPos, m_fYaw, m_fPitch, m_fSpeed, m_fViewRange, true );
#endif
}
//
ws3DPreviewCamera::~ws3DPreviewCamera()
{
#ifdef _DEBUG
   SerializeCamera( m_vPos, m_fYaw, m_fPitch, m_fSpeed, m_fViewRange, false );
#endif
}
//
void ws3DPreviewCamera::GetFrustumPlanes( D3DXPLANE pPlanes[6] ) const
{
   D3DXMATRIX mCameraViewProj = m_mView * m_mProj;

   wsGetFrustumPlanes(pPlanes, mCameraViewProj);
}
//
/*
void ws3DPreviewCamera::UpdateCollision( float deltaTime )
{
   IAdVantageCollision * pCollision = NULL;
   if( !Succeeded( m_pAdVantage->GetCollision( &pCollision, CAMERA_COLLISION_LOD_LEVEL_USED ) ) )
   {
      assert( false );
      return;
   }

   pCollision->BidForCollisionData( (const D3DXVECTOR3 *)&m_vPos, 500 );

   aD3DXVECTOR3 hitPos;
   if( Succeeded( pCollision->IntersectRayVertical(m_vPos.x, m_vPos.y, *((D3DXVECTOR3 *)&hitPos)) ) )
   {
      float minZ = hitPos.z + 2.0f;
      if( m_vPos.z < minZ )
         m_vPos.z = minZ;
   }
}
*/
//
void ws3DPreviewCamera::GetZOffsettedProjMatrix( D3DXMATRIX & mat, float zModMul, float zModAdd )
{
   D3DXMatrixPerspectiveFovLH( &mat, m_fFOV, m_fAspect, m_fNear * zModMul + zModAdd, m_fFar * zModMul + zModAdd );
}
//
void ws3DPreviewCamera::Tick( float deltaTime )
{
   ///////////////////////////////////////////////////////////////////////////
   // Update camera rotation
   //if( IsKeyClicked( kcCameraSpeedDown ) )   m_fSpeed *= 0.95f;
   //if( IsKeyClicked( kcCameraSpeedUp ) )     m_fSpeed *= 1.05f;
   m_fSpeed = clamp( m_fSpeed, 10.0f, 100000.0f );
   if( IsKeyClicked( kcViewRangeDown ) )     m_fViewRange *= 0.95f;
   if( IsKeyClicked( kcViewRangeUp ) )       m_fViewRange *= 1.05f;
   m_fViewRange = clamp( m_fViewRange, 1000.0f, 1000000.0f );
   //
   float speedBoost = IsKey( kcShiftKey )?(12.0f):(1.0f);
   speedBoost *= IsKey( kcControlKey )?(0.1f):(1.0f);
   //
   ///////////////////////////////////////////////////////////////////////////
   // Update camera rotation
   POINT cd = GetCursorDelta();
   m_fYaw		+= cd.x * 0.01f;
   m_fPitch	+= cd.y * 0.005f;
   m_fYaw		= WrapAngle( m_fYaw );
   m_fPitch	= clamp( m_fPitch, -(float)PI/2 + 1e-2f, +(float)PI/2 - 1e-2f );
   //
   ///////////////////////////////////////////////////////////////////////////
   // Update camera matrices
   D3DXMATRIX mCameraRot;
   D3DXMATRIX mRotationY; D3DXMatrixRotationY( &mRotationY, m_fPitch );
   D3DXMATRIX mRotationZ; D3DXMatrixRotationZ( &mRotationZ, m_fYaw );
   //
   mCameraRot = mRotationY * mRotationZ;
   //
   ///////////////////////////////////////////////////////////////////////////
   // Update camera movement
   bool hasInput = IsKey(kcForward) || IsKey(kcBackward) || IsKey(kcRight) || IsKey(kcLeft) || IsKey(kcUp) || IsKey(kcDown);
   m_fSpeedKeyMod = (hasInput)?(::min(m_fSpeedKeyMod + deltaTime, 2.0f)):(0.0f);
   float moveSpeed = m_fSpeed * deltaTime * (0.1f + 0.9f * (m_fSpeedKeyMod / 2.0f) * (m_fSpeedKeyMod / 2.0f)) * speedBoost;

   D3DXVECTOR3    forward( mCameraRot.m[0] );
   D3DXVECTOR3    left( mCameraRot.m[1] );
   D3DXVECTOR3    up( mCameraRot.m[2] );

   if( IsKey( kcForward ) )   m_vPos += forward * moveSpeed;
   if( IsKey( kcBackward ) )  m_vPos -= forward * moveSpeed;
   if( IsKey( kcRight) )      m_vPos += left * moveSpeed;
   if( IsKey( kcLeft) )       m_vPos -= left * moveSpeed;
   if( IsKey( kcUp) )         m_vPos += up * moveSpeed;
   if( IsKey( kcDown ) )      m_vPos -= up * moveSpeed;
   //

   //UpdateCollision( deltaTime );
   //
   m_vDir = D3DXVECTOR3( mCameraRot.m[0] );
   D3DXVECTOR3 vLookAtPt = m_vPos + m_vDir;
   D3DXVECTOR3 vUpVec( 0.0f, 0.0f, 1.0f );
   //
   //now do these two...
   D3DXMatrixLookAtLH( &m_mView, &m_vPos, &vLookAtPt, &vUpVec );

   m_fNear = 3.0f; //m_fViewRange / 20000.0f;
   m_fFar  = m_fViewRange * 1.05f;
   D3DXMatrixPerspectiveFovLH( &m_mProj, m_fFOV, m_fAspect, m_fNear, m_fFar );

   //D3DXMATRIX view, proj;
   //D3DXMatrixLookAtLH( &view, (D3DXVECTOR3*)&m_vPos, (D3DXVECTOR3*)&vLookAtPt, (D3DXVECTOR3*)&vUpVec );
   //D3DXMatrixPerspectiveFovLH( &proj, m_fFOV, m_fAspect, m_fNear, m_fFar );

}
//