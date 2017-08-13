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

#pragma warning ( disable : 4995 4996 )

#include "wsTexSplattingMgr.h"

#include "iniparser\src\IniParser.hpp"

#pragma warning ( disable : 4995 4996 )

static const char *     c_SplattingPath   = "Media\\TexSplatting\\";

wsTexSplattingMgr::wsTexSplattingMgr()
{
   m_detailUVModTexture = NULL;
   m_detailNormalMapTexture = NULL;
   m_initialized = false;
}
//
wsTexSplattingMgr::~wsTexSplattingMgr(void)
{

}
//
wsTexSplattingMgr::SplatMaterialInfo::SplatMaterialInfo()
{
   Texture           = NULL;
   NormalMapTexture  = NULL;
}
//
void  wsTexSplattingMgr::SplatMaterialInfo::ReleaseResources()
{
   SAFE_RELEASE( Texture );
   SAFE_RELEASE( NormalMapTexture );
}
//
IDirect3DTexture9 * wsTexSplattingMgr::LoadTexture( const char * texName )
{
   if( texName == NULL )
   {
      MessageBox(NULL, L"Bad or missing texture name", L"Error", MB_OK );
      return NULL;
   }

   IDirect3DTexture9 * ret = NULL;

   string texPath = wsFindResource( string(c_SplattingPath) + string(texName), false );

   if( texPath.size() != 0 )
   {
      HRESULT hr;
      V( D3DXCreateTextureFromFileA( GetD3DDevice(), texPath.c_str(), &ret ) );

      return ret;
   }
   return NULL;

}
//
void wsTexSplattingMgr::Initialize( )
{
}
//
void wsTexSplattingMgr::Deinitialize( )
{
}

//
void wsTexSplattingMgr::InitializeFromMaterialsInfo(  )
{
   string materialInfoFileName = "splatting.ini";
   string filePath = wsFindResource( string(c_SplattingPath) + materialInfoFileName, false );

   IniParser   iniParser( filePath.c_str() );

   int matCount = iniParser.getInt( "Main:NumLayers", -1 );

   assert( matCount > 3 || matCount <= 4 );
   if( matCount <= 3 || matCount > 4 )
      goto initfrommat_error;

   m_detailUVModTexture       = LoadTexture( iniParser.getString( "DetailMap:UVModifierTexture", NULL ) );
   m_detailNormalMapTexture   = LoadTexture( iniParser.getString( "DetailMap:NormalMapTexture", NULL ) );

   m_splatMaterials.resize( matCount );

   for( int i = 0; i < matCount; i++ )
   {
      SplatMaterialInfo & matInfo = m_splatMaterials[i];

      char _matName[1024];
      sprintf( _matName, "Layer%d:", i );
      string matName(_matName);

      matInfo.Texture            = LoadTexture( iniParser.getString( (matName + string("Texture")).c_str(), NULL ) );
      matInfo.NormalMapTexture   = LoadTexture( iniParser.getString( (matName + string("NormalMapTexture")).c_str(), NULL ) );
      matInfo.AlphaAdd           = iniParser.getFloat( (matName + string("AlphaAdd")).c_str(), 0.0f );
      matInfo.AlphaMul           = iniParser.getFloat( (matName + string("AlphaMul")).c_str(), 1.0f );
      matInfo.SpecularPow        = iniParser.getFloat( (matName + string("SpecularPow")).c_str(), 64.0f );
      matInfo.SpecularMul        = iniParser.getFloat( (matName + string("SpecularMul")).c_str(), 0.2f );
   }

   return;

initfrommat_error:
   MessageBox( NULL, L"Error reading materials ini file", L"Error", MB_OK );
}
//
HRESULT wsTexSplattingMgr::OnCreateDevice()
{
   assert( m_splatMaterials.size() == 0 );
   assert( m_detailUVModTexture == NULL );
   assert( m_detailNormalMapTexture == NULL );

   InitializeFromMaterialsInfo();

   return S_OK; 
}
//
void wsTexSplattingMgr::OnDestroyDevice()
{
   for( int i = 0; i < (int)m_splatMaterials.size(); i++ )
   {
      m_splatMaterials[i].ReleaseResources();
   }
   ::erase( m_splatMaterials );

   SAFE_RELEASE( m_detailUVModTexture );
   SAFE_RELEASE( m_detailNormalMapTexture );
}
//
static void SetTexture( IDirect3DTexture9 * texture, const char * name, ID3DXConstantTable * psConstants, DWORD addrUV = D3DTADDRESS_WRAP, DWORD minFilter = D3DTEXF_LINEAR, DWORD magFilter = D3DTEXF_LINEAR, DWORD mipFilter = D3DTEXF_LINEAR )
{
   IDirect3DDevice9 * pDevice = DxEventNotifier::GetD3DDevice();

   int samplerIndex = psConstants->GetSamplerIndex( name );
   if( samplerIndex != -1 )
   {
      pDevice->SetTexture( samplerIndex, texture );
      pDevice->SetSamplerState( samplerIndex, D3DSAMP_ADDRESSU, addrUV );
      pDevice->SetSamplerState( samplerIndex, D3DSAMP_ADDRESSV, addrUV );
      pDevice->SetSamplerState( samplerIndex, D3DSAMP_MINFILTER, minFilter );
      pDevice->SetSamplerState( samplerIndex, D3DSAMP_MAGFILTER, magFilter );
      pDevice->SetSamplerState( samplerIndex, D3DSAMP_MIPFILTER, mipFilter );
   }
}
//
void wsTexSplattingMgr::SetupShaderParams( ID3DXConstantTable * psConstants )
{
   int from    = 0;
   int limit   = (int)m_splatMaterials.size();

   float vaa[4];   memset( vaa, 0, 4 * 4 );
   float vam[4];   memset( vam, 0, 4 * 4 );
   float vsp[5];   memset( vsp, 0, 5 * 4 );
   float vsm[5];   memset( vsm, 0, 5 * 4 );
   int vi = 0;

   for( int i = from; i < limit; i++ )
   {
      char texName[512];
      sprintf(texName, "g_splatTileTextures%d", i-from);
      SetTexture( m_splatMaterials[i].Texture, texName, psConstants );
      sprintf(texName, "g_splatMatTileTextures%d", i-from);
      SetTexture( m_splatMaterials[i].NormalMapTexture, texName, psConstants );

      vsp[i-from] = m_splatMaterials[i].SpecularPow;
      vsm[i-from] = m_splatMaterials[i].SpecularMul;

      if( i != 0 && vi < 4 )
      {
         vaa[vi] = m_splatMaterials[i].AlphaAdd;
         vam[vi] = m_splatMaterials[i].AlphaMul;
         vi++;
         assert( vi <= 4 );
      }
   }

   SetTexture( m_detailUVModTexture, "g_detailUVModTexture", psConstants );
   SetTexture( m_detailNormalMapTexture, "g_detailNormalMapTexture", psConstants );

   IDirect3DDevice9 * pDevice = DxEventNotifier::GetD3DDevice();
   psConstants->SetFloatArray( pDevice, "g_splatAlphaModAdd", vaa, 4 );
   psConstants->SetFloatArray( pDevice, "g_splatAlphaModMul", vam, 4 );

   psConstants->SetFloatArray( pDevice, "g_splatSpecularPow", vsp, 4 );
   psConstants->SetFloatArray( pDevice, "g_splatSpecularMul", vsm, 4 );
}
//
