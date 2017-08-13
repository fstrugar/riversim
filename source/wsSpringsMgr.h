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

#include "wsCommon.h"

#pragma warning (disable : 4995)
#include <vector>

class wsSpringsMgr : public wsSimpleMgrSingleton<wsSpringsMgr>
{
public:

   struct Spring
   {
      float    x, y;
      float    radius;
      float    quantity;
      float    normalizedQuantity;     // quantity spread over radius pixels

      Spring()      {}

      Spring( float x, float y, float quantity, float radius );
   };

private:

   std::string                   m_springListPath;
   std::vector<Spring>           m_springs;

private:
   // Must do this when using wsSimpleMgrSingleton to prevent any inheritance!
   friend class wsSimpleMgrSingleton<wsSpringsMgr>;
   wsSpringsMgr(void);
   virtual ~wsSpringsMgr(void);
   //
public:
   void                          Initialize( const MapDimensions & mapDims );
   void                          Deinitialize( );
   //
   const std::vector<Spring> &   GetSprings( )                                   { return m_springs; }
   //
   void                          Tick( float deltaTime );
   //HRESULT                       Render( );
private:
   void                          InitializeFromIni( );
   //
};
