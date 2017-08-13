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

#ifndef __ACANVAS_H__
#define __ACANVAS_H__

// #include "..\..\Source\Auxiliary\Math\MathIncludes.h"
// using namespace AdVantage;


#ifdef DrawText
#undef DrawText
#endif

class ICanvas2D
{
protected:
   virtual ~ICanvas2D() {}
   //
public:
   virtual void         DrawText( int x, int y, const wchar_t * text, ... )                        = 0;
   virtual void         DrawLine( float x0, float y0, float x1, float y1, unsigned int penColor )  = 0;
   //
   virtual int          GetWidth( )                                                                = 0;
   virtual int          GetHeight( )                                                               = 0;
   //
   virtual void         CleanQueued( )                                                             = 0;
   //
};

class ICanvas3D
{
protected:
   virtual ~ICanvas3D() {}
   //
public:
   //
   virtual void         DrawBox( const D3DXVECTOR3 & v0, const D3DXVECTOR3 & v1, unsigned int penColor, unsigned int brushColor = 0x000000, const D3DXMATRIX * transform = NULL ) = 0;
   virtual void         DrawTriangle( const D3DXVECTOR3 & v0, const D3DXVECTOR3 & v1, const D3DXVECTOR3 & v2, unsigned int penColor, unsigned int brushColor = 0x000000, const D3DXMATRIX * transform = NULL ) = 0;
   //
};

ICanvas2D *    GetCanvas2D();

ICanvas3D *    GetCanvas3D();

#endif // __ACANVAS_H__

//////////////////////////////////////////////////////////////////////////
