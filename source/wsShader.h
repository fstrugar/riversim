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

#pragma once

#include "DxEventNotifier.h"

#include <vector>

HRESULT        wsCompileVertexShader( IDirect3DDevice9 * device, DWORD dwShaderFlags, const wchar_t * filePath, const char * entryProc, ID3DXConstantTable ** outConstants, IDirect3DVertexShader9 ** outShader );
HRESULT        wsCompilePixelShader( IDirect3DDevice9 * device, DWORD dwShaderFlags, const wchar_t * filePath, const char * entryProc, ID3DXConstantTable ** outConstants, IDirect3DPixelShader9 ** outShader );


class wsShader : protected DxEventReceiver
{
protected:
   IUnknown *              m_shader;
   ID3DXConstantTable *    m_constantTable;

   string                  m_path;
   string                  m_entryPoint;

public:
   wsShader();
   wsShader( const char * path, const char * entryPoint );
   virtual ~wsShader();
   //
   void                       SetShaderInfo( const char * path, const char * entryPoint );
   //
   virtual void               Reload()                   { DestroyShader(); CreateShader(); }
   //
   ID3DXConstantTable *       GetConstantTable()         { return m_constantTable; }
   //
public:
   HRESULT                    SetBool( const char * name, bool val );
   //
   HRESULT                    SetFloat( const char * name, float val );
   HRESULT                    SetFloatArray( const char * name, float val0, float val1 );
   HRESULT                    SetFloatArray( const char * name, float val0, float val1, float val2 );
   HRESULT                    SetFloatArray( const char * name, float val0, float val1, float val2, float val3 );
   HRESULT                    SetFloatArray( const char * name, const float * valArray, int valArrayCount );
   //
   HRESULT                    SetVector( const char * name, const D3DXVECTOR4 & vec );
   HRESULT                    SetMatrix( const char * name, const D3DXMATRIX & matrix);

protected:
   static std::vector<wsShader *> & GetAllShadersList();

public:
   //
   static void                      ReloadAllShaders();
   //
protected:
   virtual void               OnDestroyDevice();
   virtual HRESULT            OnCreateDevice();
   //
   virtual void               CreateShader()             = 0;
   void                       DestroyShader();
   //
};

class wsPixelShader : public wsShader
{
public:
   wsPixelShader()            {}
   wsPixelShader( const char * path, const char * entryPoint ) : wsShader( path, entryPoint ) { wsPixelShader::CreateShader(); }
   virtual ~wsPixelShader()   {}

public:
   IDirect3DPixelShader9 *    GetShader();

   operator IDirect3DPixelShader9 * () { return GetShader(); }

public:
   HRESULT                    SetTexture( const char * name, IDirect3DBaseTexture9 * texture,
                                                                                          DWORD addrU = D3DTADDRESS_CLAMP, 
                                                                                          DWORD addrV = D3DTADDRESS_CLAMP,
                                                                                          DWORD minFilter = D3DTEXF_POINT,
                                                                                          DWORD magFilter = D3DTEXF_POINT,
                                                                                          DWORD mipFilter = D3DTEXF_POINT );

protected:
   virtual void               CreateShader();

};

class wsVertexShader : public wsShader
{
public:
   wsVertexShader()           {}
   wsVertexShader( const char * path, const char * entryPoint ) : wsShader( path, entryPoint ) { wsVertexShader::CreateShader(); }
   virtual ~wsVertexShader()  {}

public:
   IDirect3DVertexShader9 *   GetShader();

   operator IDirect3DVertexShader9 * () { return GetShader(); }

public:
   HRESULT                    SetTexture( const char * name, IDirect3DTexture9 * texture,
                                                                                          DWORD addrU = D3DTADDRESS_CLAMP, 
                                                                                          DWORD addrV = D3DTADDRESS_CLAMP,
                                                                                          DWORD minFilter = D3DTEXF_POINT,
                                                                                          DWORD magFilter = D3DTEXF_POINT,
                                                                                          DWORD mipFilter = D3DTEXF_POINT );


protected:
   virtual void               CreateShader();

};
