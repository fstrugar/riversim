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

#include "DXUT.h"

#pragma warning (disable : 4995)
#include <string>
#include <vector>

#ifndef PI
#define PI 3.141592653
#endif

template<class T>
inline T clamp(T const & v, T const & b, T const & c)
{
   if( v < b ) return b;
   if( v > c ) return c;
   return v;
}

// short for clamp( a, 0, 1 )
template<class T>
inline T saturate( T const & a )
{
   return ::clamp( a, (T)0.0, (T)1.0 );
}

template<class T>
inline T lerp(T const & a, T const & b, T const & f)
{
   return a + (b-a)*f;
}

template<class T>
inline void swap(T & a, T & b)
{
   T temp = b;
   b = a;
   a = temp;
}

template<class T>
inline T sqr(T & a)
{
   return a * a;
}

inline float randf( )       { return rand() / (float)RAND_MAX; }

// Get time independent lerp function. Play with different lerpRates - the bigger the rate, the faster the lerp!
inline float timeIndependentLerpF(float deltaTime, float lerpRate)
{
   return 1.0f - expf(-fabsf(deltaTime*lerpRate));
}

enum ApplicationMode
{
   AM_InvalidValue   = -1,
   AM_Simulation     = 0,
   AM_Preview,
};

//#define PROFILE_LIB_MEMORY
enum KeyCommands
{
   kcForward	= 0,
   kcBackward,
   kcLeft,
   kcRight,
   kcUp,
   kcDown,
   kcShiftKey,
   kcControlKey,
   //kcCameraSpeedUp,
   //kcCameraSpeedDown,
   kcViewRangeUp,
   kcViewRangeDown,
   kcToggleHelpText,
   kcToggleWireframe,
   kcToggleSimpleRender,
   //kcAddSphereObject,
   //kcAddBoxObject,
   //kcAddManyObjects,
   //kcCleanAllObjects,

   kcToggleCalculateTotalWaterSum,

   kcDoSimpleSplash,

   kcReloadAll,
   kcReloadShadersOnly,

   kcSpringBoostMode,

   kcSaveState,
   kcLoadState,

   kcSetLight0,
   kcSetLight1,
   kcSetLight2,
   kcSetLight3,
   kcSetLight4,
   kcSetLight5,
   kcSetLight6,
   kcSetLight7,
   kcSetLight8,
   kcSetLight9,

   kcPause,
   kcOneFrameStep,

   kcToggleAppMode,

   kcToggleShowVelocitiesOrDepth,

   kcLASTVALUE
};

struct TransofmedTexturedVertex
{
   float    x, y, z, rhw;
   float    tu, tv;

   TransofmedTexturedVertex() {}
   TransofmedTexturedVertex( float x, float y, float z, float rhw, float tu, float tv )   : x(x), y(y), z(z), rhw(rhw), tu(tu), tv(tv)    { }

   static const DWORD     FVF = D3DFVF_XYZRHW | D3DFVF_TEX1;
};

struct TransformedVertex
{
   float    x, y, z, rhw;

   TransformedVertex() {}
   TransformedVertex( float x, float y, float z, float rhw )   : x(x), y(y), z(z), rhw(rhw)   { }

   static const DWORD     FVF = D3DFVF_XYZRHW;
};

struct TransformedColoredVertex
{
   float    x, y, z, rhw;

   DWORD    diffuse;

   TransformedColoredVertex() {}
   TransformedColoredVertex( float x, float y, float z, float rhw, DWORD diffuse )   : x(x), y(y), z(z), rhw(rhw), diffuse( diffuse )   { }

   static const DWORD     FVF = D3DFVF_XYZRHW | D3DFVF_DIFFUSE;
};

struct PositionVertex
{
   float    x, y, z;

   PositionVertex() {}
   PositionVertex( float x, float y, float z )  : x(x), y(y), z(z)         { }
   PositionVertex( const D3DXVECTOR3 & v )      : x(v.x), y(v.y), z(v.z)   { }

   static const DWORD     FVF = D3DFVF_XYZ;
};

struct MapDimensions
{
   float                      MinX;
   float                      MinY;
   float                      MinZ;
   float                      SizeX;
   float                      SizeY;
   float                      SizeZ;

   float                      MaxX() { return MinX + SizeX; }
   float                      MaxY() { return MinY + SizeY; }
   float                      MaxZ() { return MinZ + SizeZ; }
};

// This class is flawed in many ways but as long as we don't use it from multiple compilation units
// and it's specialized classes are not derived from (USE PRIVATE CONSTRUCTORS!!), we should be fine.
template<class T>
class wsSimpleMgrSingleton
{
protected:
   wsSimpleMgrSingleton()                 {}
   virtual ~wsSimpleMgrSingleton()        {}
   //
public:
   //
   static T & Instance(void)
   {
      static T me;
      return me;
   }
};

template<class T> static void erase( std::vector<T> & v )
{
   if( v.empty() ) return;
   v.erase(v.begin(), v.end());
}

inline unsigned short HalfFloatPack(float val)
{
   unsigned int num5 = *((unsigned int*) &val);
   unsigned int num3 = (unsigned int) ((num5 & 0x80000000) >> 0x10); // -2147483648
   unsigned int num = num5 & 0x7fffffff;
   if (num > 0x47ffefff)
   {
      return (unsigned short) (num3 | 0x7fff);
   }
   if (num < 0x38800000)
   {
      unsigned int num6 = (num & 0x7fffff) | 0x800000;
      int num4 = 0x71 - ((int) (num >> 0x17));
      num = (num4 > 0x1f) ? 0 : (num6 >> num4);
      return (unsigned short) (num3 | (((num + 0xfff) + ((num >> 13) & 1)) >> 13));
   }
   return (unsigned short) (num3 | ((((num + -939524096) + 0xfff) + ((num >> 13) & 1)) >> 13));
}

inline float HalfFloatUnpack(unsigned short val)
{
   unsigned int num3;
   if ((val & -33792) == 0)
   {
      if ((val & 0x3ff) != 0)
      {
         unsigned int num2 = 0xfffffff2;
         unsigned int num = (unsigned int) (val & 0x3ff);
         while ((num & 0x400) == 0)
         {
            num2--;
            num = num << 1;
         }
         num &= 0xfffffbff;
         num3 = ((unsigned int) (((val & 0x8000) << 0x10) | ((num2 + 0x7f) << 0x17))) | (num << 13);
      }
      else
      {
         num3 = (unsigned int) ((val & 0x8000) << 0x10);
      }
   }
   else
   {
      num3 = (unsigned int) ((((val & 0x8000) << 0x10) | (((((val >> 10) & 0x1f) - 15) + 0x7f) << 0x17)) | ((val & 0x3ff) << 13));
   }
   return *(((float*) &num3));
}

#ifdef DEBUG
#define DBGBREAK __asm{ int 3 }
#else
#define DBGBREAK
#endif

bool     IsKey( enum KeyCommands );
bool     IsKeyClicked( enum KeyCommands );
bool     IsKeyReleased( enum KeyCommands );

POINT    GetCursorDelta();
float	   WrapAngle( float angle );

const float    c_diffuseLightDir[4]    = { -0.632f, -0.632f, -0.447f, 0.0f }; // use w == 0 for directional light
//const float    c_diffuseLightDir[4]    = { 0.0f, -1.0f, 0.0f, 0.0f }; // use w == 0 for directional light
const float    c_fogColor[4]           = { 0.0f, 0.5f, 0.5f, 1.0f };
const float    c_lightColorDiffuse[4]  = { 0.7f, 0.7f, 0.7f, 1.0f };
const float    c_lightColorAmbient[4]  = { 0.3f, 0.3f, 0.3f, 1.0f };
//const float    c_lightColorDiffuse[4]  = { 1.0f, 1.0f, 1.0f, 1.0f };
//const float    c_lightColorAmbient[4]  = { 0.0f, 0.0f, 0.0f, 1.0f };

bool           FileExists( const char * file );
bool           FileExists( const wchar_t * file );

std::wstring   wsFindResource( const std::wstring & file, bool showErrorMessage = true );
std::string    wsFindResource( const std::string & file, bool showErrorMessage = true );

void           wsSetTexture( IDirect3DTexture9 * texture, const char * name, ID3DXConstantTable * constants, 
                            DWORD addrU = D3DTADDRESS_CLAMP, 
                            DWORD addrV = D3DTADDRESS_CLAMP,
                            DWORD minFilter = D3DTEXF_POINT,
                            DWORD magFilter = D3DTEXF_POINT,
                            DWORD mipFilter = D3DTEXF_POINT );
void           wsSetVertexTexture( IDirect3DTexture9 * texture, const char * name, ID3DXConstantTable * constants );


std::wstring   Format(const wchar_t * fmtString, ...);
std::string    Format(const char * fmtString, ...);

std::wstring   simple_widen( const std::string & s );
std::string    simple_narrow( const std::wstring & s );

const char *   wsGetIniPath();

void           wsGetFrustumPlanes( D3DXPLANE * pPlanes, const D3DXMATRIX & mCameraViewProj );

HRESULT        wsRenderSimulation( int fromX, int fromY, int toX, int toY, int texWidth, int texHeight );
HRESULT        wsRenderSimulation( int width, int height, int cutEdge = 0 );
HRESULT        wsSetRenderTarget( int index, IDirect3DTexture9 * texture );
HRESULT        wsGetRenderTargetData( IDirect3DTexture9 * srcTexture, IDirect3DTexture9 * destTexture );

HRESULT        wsFillColor( D3DXVECTOR4 color, IDirect3DTexture9 * texture );

HRESULT        wsStretchRect( IDirect3DTexture9* pSourceTexture, CONST RECT* pSourceRect, IDirect3DTexture9* pDestTexture, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter);

std::string    wsGetExeDirectory();

std::string    wsGetProjectDirectory();

void           wsGetProjectDefaultCameraParams( D3DXVECTOR3 & pos, const D3DXVECTOR3 & dir, float & viewRange );

bool           wsFileExists( const char * file );
bool           wsFileExists( const wchar_t * file );

template<class StringType>
void           wsSplitFilePath( const StringType & inFullPath, StringType & outDirectory, StringType & outFileName )
{
   size_t np = StringType::npos;
   size_t np1 = inFullPath.find_last_of('\\');
   size_t np2 = inFullPath.find_last_of('/');
   if( np1 != StringType::npos && np2 != StringType::npos )
      np = (np1>np2)?(np1):(np2);
   else if ( np1 != StringType::npos )  np = np1;
   else if ( np2 != StringType::npos )  np = np2;

   if( np != StringType::npos )
   {
      outDirectory   = inFullPath.substr( 0, np+1 );
      outFileName    = inFullPath.substr( np+1 );
   }
   else
   {
      outFileName = inFullPath;
      outDirectory.erase();
   }

}
