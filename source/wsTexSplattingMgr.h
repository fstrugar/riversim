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

#ifndef __ATEX_SPLATTING_MGR_H__
#define __ATEX_SPLATTING_MGR_H__

#include "wsCommon.h"
#include "DxEventNotifier.h"

class wsTexSplattingMgr : public wsSimpleMgrSingleton<wsTexSplattingMgr>, private DxEventReceiver
{

   struct SplatMaterialInfo
   {
      IDirect3DTexture9 *  Texture;
      IDirect3DTexture9 *  NormalMapTexture;

      float                AlphaAdd;
      float                AlphaMul;

      float                SpecularPow;
      float                SpecularMul;
      //
   public:
      SplatMaterialInfo();

      void              ReleaseResources();
   };
   //
   vector<SplatMaterialInfo>     m_splatMaterials;
   //
   IDirect3DTexture9*            m_detailUVModTexture;
   IDirect3DTexture9*            m_detailNormalMapTexture;
   //
   bool                          m_initialized;
   //
private:
   // Must do this when using wsSimpleMgrSingleton to prevent any inheritance!
   friend class wsSimpleMgrSingleton<wsTexSplattingMgr>;
   wsTexSplattingMgr(void);
   virtual ~wsTexSplattingMgr(void);

//
   //
public:
   //
   void                          Initialize( );
   void                          Deinitialize( );
   //
   void                          SetupShaderParams( ID3DXConstantTable * psConstants );
   //
private:
   //
   virtual HRESULT 				   OnCreateDevice( );
   virtual void                  OnDestroyDevice( );
   //
   IDirect3DTexture9 *           LoadTexture( const char * texName );
   //
   void                          InitializeFromMaterialsInfo( );
};

#endif // __ATEX_SPLATTING_MGR_H__
