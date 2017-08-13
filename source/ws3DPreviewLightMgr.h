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

#ifndef __A_LIGHT_MGR_H__
#define __A_LIGHT_MGR_H__

#include "DxEventNotifier.h"
#include "ws3DPreviewCamera.h"
#include "wsSkybox.h"

class ws3DPreviewLightMgr : protected DxEventReceiver
{
   D3DXVECTOR3                   m_directionalLightDir;
   //
   D3DXVECTOR3                   m_directionalLightDirTarget;
   D3DXVECTOR3                   m_directionalLightDirTargetL1;
   //
   ID3DXMesh *                   m_sunMesh;
   //
   bool                          m_useSkybox;
   wsSkybox                      m_skyBox;
   //
public:
   ws3DPreviewLightMgr(void);
   ~ws3DPreviewLightMgr(void);
   //
   D3DXVECTOR3                   GetDirectionalLightDir()               { return m_directionalLightDir; }
   //
   bool                          IsUsingSkybox()                        { return m_useSkybox; }
   IDirect3DCubeTexture9 *       GetSkyboxTexture()                     { return m_skyBox.GetCubeMap(); }
   //
   void                          Tick( float deltaTime );
   HRESULT                       Render( ws3DPreviewCamera * pCamera );
   //
protected:
   virtual HRESULT 				   OnCreateDevice( );
   virtual void                  OnDestroyDevice( );
   //
};

#endif // __A_LIGHT_MGR_H__
