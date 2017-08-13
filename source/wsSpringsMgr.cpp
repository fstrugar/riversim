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

#include "wsSpringsMgr.h"

#include "wsCommon.h"

#include "iniparser\src\IniParser.hpp"

#pragma warning (disable : 4996)

using namespace std;

static float calculatePixelInfluence( float diffX, float diffY, float radius )
{
   float distQ    = clamp( 1.0f - sqrtf( diffX*diffX + diffY*diffY ) / radius, 0.0f, 1.0f );
   
   float amount   = sqrtf( distQ );

   return amount;
}

static float calculateSumPixelInfluences( float radius )
{
   float totalInfluence = 0.0f;

   int maxRad = (int)ceilf( radius );
   for( int ix = -maxRad; ix < maxRad; ix++ )
      for( int iy = -maxRad; iy < maxRad; iy++ )
      {
         totalInfluence += calculatePixelInfluence( (float)ix, (float)iy, radius );
      }

   return totalInfluence;
}

wsSpringsMgr::Spring::Spring( float x, float y, float quantity, float radius )
 : x(x), y(y), quantity(quantity), radius(radius)
{
   normalizedQuantity = quantity / calculateSumPixelInfluences( radius );
}

wsSpringsMgr::wsSpringsMgr()
{

}

wsSpringsMgr::~wsSpringsMgr()
{

}

void wsSpringsMgr::Initialize( const MapDimensions & mapDims )
{
   InitializeFromIni();
}

void wsSpringsMgr::Deinitialize( )
{

}

static string removeUnusedChars( const string & input )
{
   char bufferOut[2048];
   assert( input.size() < sizeof(bufferOut)-1 );

   int currIndex = 0;

   for( size_t i = 0; i < input.size(); i++ )
   {
      if( input[i] != ' ' && input[i] != '\t' && input[i] != '\n' )
         bufferOut[currIndex++] = input[i];
   }
   bufferOut[currIndex] = 0;

   return string( bufferOut );
}

void wsSpringsMgr::InitializeFromIni( )
{
   ::erase(m_springs);

   IniParser   iniParser( wsGetIniPath() );
   
   m_springListPath = iniParser.getString( "main:springListPath", "" );

   if( !wsFileExists( m_springListPath.c_str() ) )
   {
      MessageBoxA( NULL, Format("Error trying to load spring list from '%s'!", m_springListPath.c_str()).c_str(), "Error", MB_OK );
   }
   else
   {
      FILE * file = fopen( m_springListPath.c_str(), "r" );

      if( file != NULL )
      {
         char lineBuffer[1024];
         int lineCounter = 0;
         while( fgets( lineBuffer, sizeof(lineBuffer), file ) != NULL )
         {
            lineCounter++;
            string line(lineBuffer);

            size_t findRes;
            
            findRes = line.find( "//" );
            if( findRes != string::npos ) 
               line = line.substr( 0, findRes );
            
            line = removeUnusedChars( line );

            if( line.size() == 0 )
               continue;

            findRes = line.find( ',', 0 );
            if( findRes == string::npos ) 
               goto wsSpringsMgr_lineFormatError;
            float coordX = (float)atof( line.substr( 0, findRes ).c_str() );
            line = line.substr( findRes+1 );
            
            findRes = line.find( ';', 0 );
            if( findRes == string::npos ) 
               goto wsSpringsMgr_lineFormatError;
            float coordY = (float)atof( line.substr( 0, findRes ).c_str() );
            line = line.substr( findRes+1 );
            
            findRes = line.find( ';', 0 );
            if( findRes == string::npos ) 
               goto wsSpringsMgr_lineFormatError;
            float waterQuantity = (float)atof( line.substr( 0, findRes ).c_str() );
            line = line.substr( findRes+1 );

            float springRadius = (float)atof( line.c_str() );

            // coordX, coordY, waterQuantity, springRadius should be checked here and appropriate error msg displayed.....

            m_springs.push_back( Spring( coordX, coordY, waterQuantity, springRadius) );
            continue;

            wsSpringsMgr_lineFormatError:
               MessageBoxA( NULL, Format("Bad format in spring list file on line %d.", lineCounter).c_str(), "Error", MB_OK );
         }

      //wsSpringsMgr_springListLoadError:
         fclose( file );
      }
   }
}

void wsSpringsMgr::Tick( float deltaTime )
{
   if( IsKeyClicked(kcReloadAll) )
   {
      InitializeFromIni();
   }
}

