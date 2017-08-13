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

#ifndef __ACAMERA_DX9_H__
#define __ACAMERA_DX9_H__

//#include "Common.h"

#pragma warning (disable : 4995)


class ws3DPreviewCamera
{
protected:
   //
   float                m_fFOV;
   float                m_fAspect;
   float                m_fNear;
   float                m_fFar;
   //
   float                m_fYaw;
   float                m_fPitch;
   D3DXVECTOR3          m_vPos;
   D3DXVECTOR3          m_vDir;
   //
   D3DXMATRIX           m_mView;
   D3DXMATRIX           m_mProj;
   //
   float                m_fSpeed;
   float                m_fViewRange;
   //
   float                m_fSpeedKeyMod;
   //
public:
   ws3DPreviewCamera();
   virtual ~ws3DPreviewCamera();
   //
public:
   //
   float                GetViewRange( )                        { return m_fViewRange; }
   float                GetSpeed( )                            { return m_fSpeed; }
   //
   void                 SetAspect( float aspect )              { m_fAspect = aspect; }
   //
   void                 GetFrustumPlanes( D3DXPLANE pPlanes[6] ) const;
   //
   void                 Tick( float deltaTime );
   //
   float                GetFOV() const                         { return m_fFOV; }
   float                GetAspect() const                      { return m_fAspect; }
   //
   void                 SetFOV( float fov )                    { m_fFOV = fov; }
   //
   float                GetYaw() const                         { return m_fYaw; }
   float                GetPitch() const                       { return m_fPitch; }
   const D3DXVECTOR3 &  GetPosition() const                    { return m_vPos; }
   const D3DXVECTOR3 &  GetDirection() const                   { return m_vDir; }
   //
   const D3DXMATRIX *   GetViewMatrix() const                  { return &m_mView; }
   const D3DXMATRIX *   GetProjMatrix() const                  { return &m_mProj; }
   //
   float                GetViewRange() const                   { return m_fViewRange; }
   //
   void                 GetZOffsettedProjMatrix( D3DXMATRIX & mat, float zModMul = 1.0f, float zModAdd = 0.0f );
   //
   //private:
   //   void                 UpdateCollision( float deltaTime );
   //
};

#endif // __ACAMERA_DX9_H__
