#include "DXUT.h"

#include "wsCommon.h"
#include "wsShader.h"

#ifdef _DEBUG
#define SHADER_LOAD_FROM_EMBEDDED (0)
#else
#define SHADER_LOAD_FROM_EMBEDDED (1)
#endif

const char *               g_VertexShaderProfile   = "vs_3_0";
const char *               g_PixelShaderProfile    = "ps_3_0";

#if SHADER_LOAD_FROM_EMBEDDED

#include "shaders.cpp"

static int         shadersFindFile( const char * name )
{
   string nameOnly = name;
   string pathOnly;
   wsSplitFilePath<string>( name, pathOnly, nameOnly );

   for( int i = 0; i < c_numberOfFiles; i++ )
   {
      if( _strnicmp( nameOnly.c_str(), (const char *)&c_buffer[c_fileNameOffsets[i]],  nameOnly.size() ) == 0 )
         return i;
   }
   return -1;
}

static int     shadersGetDataSize( int index )
{
   int size =     (c_buffer[c_fileDataOffsets[index]+0])
               |  (c_buffer[c_fileDataOffsets[index]+1] << 8)
               |  (c_buffer[c_fileDataOffsets[index]+2] << 16)
               |  (c_buffer[c_fileDataOffsets[index]+3] << 24);
   return size;
}

static const unsigned char * shadersGetData( int index )
{
   return &c_buffer[c_fileDataOffsets[index]+4];
}

class DxIncludeHandler : public ID3DXInclude
{
private:
   STDMETHOD(Open)(THIS_ D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes)
   {
      int builtInIndex = shadersFindFile( pFileName );
      if( builtInIndex == -1 )
         return E_FAIL;

      *ppData  = shadersGetData( builtInIndex );
      *pBytes  = shadersGetDataSize( builtInIndex )-1;

      return S_OK;
   }
   STDMETHOD(Close)(THIS_ LPCVOID pData)
   {
      return S_OK;
   }
};

static DxIncludeHandler    s_dxIncludeHandler;
#endif


//
HRESULT wsCompileVertexShader( IDirect3DDevice9 * device, DWORD dwShaderFlags, const wchar_t * filePath, const char * entryProc, ID3DXConstantTable ** outConstants, IDirect3DVertexShader9 ** outShader )
{
   LPD3DXBUFFER pCode = NULL;
   LPD3DXBUFFER pErrorMsg = NULL;

   HRESULT hr;

#if SHADER_LOAD_FROM_EMBEDDED
   int builtInIndex = shadersFindFile( simple_narrow(filePath).c_str() );
   if( builtInIndex != -1 )
   {
      int dataSize = shadersGetDataSize( builtInIndex );
      hr = D3DXCompileShader( (LPCSTR)shadersGetData( builtInIndex ), dataSize-1, NULL, &s_dxIncludeHandler, entryProc, g_VertexShaderProfile, dwShaderFlags, &pCode, &pErrorMsg, outConstants );
      hr = device->CreateVertexShader( ( DWORD* )pCode->GetBufferPointer(), outShader );
      pCode->Release();
      return hr;
   }
#endif

retryCompileVS:

   hr = D3DXCompileShaderFromFile( filePath, NULL, NULL, entryProc, g_VertexShaderProfile, dwShaderFlags, &pCode, &pErrorMsg, outConstants );
   if( FAILED( hr ) )
   {
      wprintf( L"\n" );
      wprintf( (const wchar_t *)pErrorMsg->GetBufferPointer() );
      wprintf( L"\n" );
      pErrorMsg->Release();
      if( MessageBox(NULL, L"Error while compiling pixel shader, retry?", L"Shader error", MB_YESNO) == IDYES )
         goto retryCompileVS;
      return hr;
   }

retryCreateVS:

   hr = device->CreateVertexShader( ( DWORD* )pCode->GetBufferPointer(), outShader );
   pCode->Release();
   if( FAILED( hr ) )
   {
      if( MessageBox(NULL, L"Error while creating pixel shader, retry?", L"Shader error", MB_YESNO) == IDYES )
         goto retryCreateVS;
      return hr;
   }

   return S_OK;
}

HRESULT wsCompilePixelShader( IDirect3DDevice9 * device, DWORD dwShaderFlags, const wchar_t * filePath, const char * entryProc, ID3DXConstantTable ** outConstants, IDirect3DPixelShader9 ** outShader )
{
   LPD3DXBUFFER pCode = NULL;
   LPD3DXBUFFER pErrorMsg = NULL;

   HRESULT hr;

#if SHADER_LOAD_FROM_EMBEDDED
   int builtInIndex = shadersFindFile( simple_narrow(filePath).c_str() );
   if( builtInIndex != -1 )
   {
      int dataSize = shadersGetDataSize( builtInIndex );
      hr = D3DXCompileShader( (LPCSTR)shadersGetData( builtInIndex ), dataSize, NULL, &s_dxIncludeHandler, entryProc, g_PixelShaderProfile, dwShaderFlags, &pCode, &pErrorMsg, outConstants );
      hr = device->CreatePixelShader( ( DWORD* )pCode->GetBufferPointer(), outShader );
      pCode->Release();
      return hr;
   }
#endif

retryCompilePS:

   hr = D3DXCompileShaderFromFile( filePath, NULL, NULL, entryProc, g_PixelShaderProfile, dwShaderFlags, &pCode, &pErrorMsg, outConstants );
   if( FAILED( hr ) )
   {
      wprintf( L"\n" );
      wprintf( (const wchar_t *)pErrorMsg->GetBufferPointer() );
      wprintf( L"\n" );
      pErrorMsg->Release();
      if( MessageBox(NULL, L"Error while compiling pixel shader, retry?", L"Shader error", MB_YESNO) == IDYES )
         goto retryCompilePS;
      return hr;
   }
/*
   {
      FILE * file = fopen( "T:\\testshader.txt", "wb" );
      fwrite( pCode->GetBufferPointer(), 1, pCode->GetBufferSize(), file );
      fclose( file );
  }
  */

retryCreatePS:

   hr = device->CreatePixelShader( ( DWORD* )pCode->GetBufferPointer(), outShader );
   pCode->Release();
   if( FAILED( hr ) )
   {
      if( MessageBox(NULL, L"Error while creating pixel shader, retry?", L"Shader error", MB_YESNO) == IDYES )
         goto retryCreatePS;
      return hr; //DXTRACE_ERR( TEXT( "CreatePixelShader" ), hr );
   }

   return S_OK;
}
//
wsShader::wsShader()
{
   m_shader          = NULL;
   m_constantTable   = NULL;
   m_path            = "";
   m_entryPoint      = "";

   GetAllShadersList().push_back(this);
}
//
wsShader::wsShader( const char * path, const char * entryPoint )
{
   m_shader          = NULL;
   m_constantTable   = NULL;
   m_path            = path;
   m_entryPoint      = entryPoint;

   GetAllShadersList().push_back(this);
}
//
wsShader::~wsShader()
{
   DestroyShader();

   for( size_t i = 0; i < GetAllShadersList().size(); i++ )
   {
      if( GetAllShadersList()[i] == this )
      {
         GetAllShadersList().erase( GetAllShadersList().begin() + i );
         break;
      }
   }
}
//
std::vector<wsShader *> & wsShader::GetAllShadersList()
{
   static std::vector<wsShader *> list;
   return list;
}
//
void wsShader::ReloadAllShaders()
{
   for( size_t i = 0; i < GetAllShadersList().size(); i++ )
      GetAllShadersList()[i]->Reload();
}
//
void wsShader::SetShaderInfo( const char * path, const char * entryPoint )
{
   DestroyShader();
   m_path         = path;
   m_entryPoint   = entryPoint;
   CreateShader();
}
//
void wsShader::OnDestroyDevice()
{
   DestroyShader();
}
//
void wsShader::DestroyShader()
{
   SAFE_RELEASE( m_shader );
   SAFE_RELEASE( m_constantTable );
}
//
HRESULT wsShader::OnCreateDevice()
{
   CreateShader();

   return S_OK;
}
//
IDirect3DPixelShader9 * wsPixelShader::GetShader()
{
   if(m_shader == NULL) 
      return NULL; 

   HRESULT hr;
   IDirect3DPixelShader9 * ret;
   V( m_shader->QueryInterface( IID_IDirect3DPixelShader9, (void**)&ret ) );
   ret->Release();

   return ret;
}
//
IDirect3DVertexShader9 * wsVertexShader::GetShader()
{
   if(m_shader == NULL) 
      return NULL; 

   HRESULT hr;
   IDirect3DVertexShader9 * ret;
   V( m_shader->QueryInterface( IID_IDirect3DVertexShader9, (void**)&ret ) );
   ret->Release();

   return ret;
}
//
void wsPixelShader::CreateShader()
{
   HRESULT hr;
   IDirect3DDevice9 * device = GetD3DDevice();
   if( device == NULL || m_path.size() == 0 )
      return;

   DWORD dwShaderFlags = 0;
#ifdef DEBUG_PS
   dwShaderFlags |= D3DXSHADER_FORCE_PS_SOFTWARE_NOOPT;
#endif

#if _DEBUG
   dwShaderFlags |= D3DXSHADER_DEBUG;
#endif

#if SHADER_LOAD_FROM_EMBEDDED
   wstring path = simple_widen( m_path );
#else
   wstring path = simple_widen( wsFindResource(m_path) );
#endif

   IDirect3DPixelShader9 * shader;
   V( wsCompilePixelShader( device, dwShaderFlags, path.c_str(), m_entryPoint.c_str(), &m_constantTable, &shader ) );

   V( shader->QueryInterface( IID_IUnknown, (void**)&m_shader ) );
   shader->Release();
}
//
void wsVertexShader::CreateShader()
{
   HRESULT hr;
   IDirect3DDevice9 * device = GetD3DDevice();
   if( device == NULL || m_path.size() == 0 )
      return;

   DWORD dwShaderFlags = 0;
#ifdef DEBUG_VS
   dwShaderFlags |= D3DXSHADER_FORCE_VS_SOFTWARE_NOOPT;
#endif

#if _DEBUG
   dwShaderFlags |= D3DXSHADER_DEBUG;
#endif

#if SHADER_LOAD_FROM_EMBEDDED
   wstring path = simple_widen( m_path );
#else
   wstring path = simple_widen( wsFindResource(m_path) );
#endif


   IDirect3DVertexShader9 * shader;
   V( wsCompileVertexShader( device, dwShaderFlags, path.c_str(), m_entryPoint.c_str(), &m_constantTable, &shader ) );

   V( shader->QueryInterface( IID_IUnknown, (void**)&m_shader ) );
   shader->Release();
}
//
//
HRESULT wsShader::SetBool( const char * name, bool val )
{
   return m_constantTable->SetBool( GetD3DDevice(), name, val );
}
//
HRESULT wsShader::SetFloat( const char * name, float val )
{
   return m_constantTable->SetFloat( GetD3DDevice(), name, val );
}
//
HRESULT wsShader::SetFloatArray( const char * name, float val0, float val1 )
{
   float v[2] = { val0, val1 };
   return m_constantTable->SetFloatArray( GetD3DDevice(), name, v, 2 );
}
//
HRESULT wsShader::SetFloatArray( const char * name, float val0, float val1, float val2 )
{
   float v[3] = { val0, val1, val2 };
   return m_constantTable->SetFloatArray( GetD3DDevice(), name, v, 3 );
}
//
HRESULT wsShader::SetFloatArray( const char * name, float val0, float val1, float val2, float val3 )
{
   float v[4] = { val0, val1, val2, val3 };
   return m_constantTable->SetFloatArray( GetD3DDevice(), name, v, 4 );
}
//
HRESULT wsShader::SetFloatArray( const char * name, const float * valArray, int valArrayCount )
{
   return m_constantTable->SetFloatArray( GetD3DDevice(), name, valArray, valArrayCount );
}
//
HRESULT wsShader::SetVector( const char * name, const D3DXVECTOR4 & vec )
{
   return m_constantTable->SetVector( GetD3DDevice(), name, &vec );
}
//
HRESULT wsShader::SetMatrix( const char * name, const D3DXMATRIX & matrix)
{
   return m_constantTable->SetMatrix( GetD3DDevice(), name, &matrix );
}
//
HRESULT wsPixelShader::SetTexture( const char * name, IDirect3DBaseTexture9 * texture, DWORD addrU, DWORD addrV, DWORD minFilter, DWORD magFilter, DWORD mipFilter )
{
   IDirect3DDevice9 * device = GetD3DDevice();

   int samplerIndex = m_constantTable->GetSamplerIndex( name );
   if( samplerIndex != -1 )
   {
      device->SetTexture( samplerIndex, texture );
      device->SetSamplerState( samplerIndex, D3DSAMP_ADDRESSU, addrU );
      device->SetSamplerState( samplerIndex, D3DSAMP_ADDRESSV, addrV );
      device->SetSamplerState( samplerIndex, D3DSAMP_MINFILTER, minFilter );
      device->SetSamplerState( samplerIndex, D3DSAMP_MAGFILTER, magFilter );
      device->SetSamplerState( samplerIndex, D3DSAMP_MIPFILTER, mipFilter );
      return S_OK;
   }
   return E_FAIL;
}
//
HRESULT wsVertexShader::SetTexture( const char * name, IDirect3DTexture9 * texture, DWORD addrU, DWORD addrV, DWORD minFilter, DWORD magFilter, DWORD mipFilter )
{
   IDirect3DDevice9 * device = GetD3DDevice();

   int samplerIndex = m_constantTable->GetSamplerIndex( name );
   if( samplerIndex != -1 )
   {
      device->SetTexture( D3DVERTEXTEXTURESAMPLER0 + samplerIndex, texture );
      device->SetSamplerState( D3DVERTEXTEXTURESAMPLER0 + samplerIndex, D3DSAMP_ADDRESSU, addrU );
      device->SetSamplerState( D3DVERTEXTEXTURESAMPLER0 + samplerIndex, D3DSAMP_ADDRESSV, addrV );
      device->SetSamplerState( D3DVERTEXTEXTURESAMPLER0 + samplerIndex, D3DSAMP_MINFILTER, minFilter );
      device->SetSamplerState( D3DVERTEXTEXTURESAMPLER0 + samplerIndex, D3DSAMP_MAGFILTER, magFilter );
      device->SetSamplerState( D3DVERTEXTEXTURESAMPLER0 + samplerIndex, D3DSAMP_MIPFILTER, mipFilter );
      return S_OK;
   }
   return E_FAIL;
}
//