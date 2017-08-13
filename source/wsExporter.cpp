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

#include "wsExporter.h"

const float c_emptyWaterPixelID     = -FLT_MAX;

static void LoadTerrainHeightmap( AdVantage::TiledBitmap * terrainHeightmapFile, unsigned short * outBuffer )
{
   int rasterSizeX =  terrainHeightmapFile->Width();
   int rasterSizeY =  terrainHeightmapFile->Height();

   for( int y = 0; y < rasterSizeY; y++ )
      for( int x = 0; x < rasterSizeX; x++ )
      {
         unsigned short pixel;
         terrainHeightmapFile->GetPixel( x, y, &pixel );
         outBuffer[ y * rasterSizeX + x ] = pixel; //(float)pixel / 65535.0f;
      }
}

static void LoadAndInitializeWaterData( unsigned short * terrainHeightmap, AdVantage::TiledBitmap * watermapFile, float * waterHeightmap, unsigned int * waterInfomap, const MapDimensions & mapDims, const ExporterSettings & settings )
{
   int rasterSizeX =  watermapFile->Width();
   int rasterSizeY =  watermapFile->Height();

   //int minbx = INT_MAX;
   //int minby = INT_MAX;
   //int maxbx = INT_MIN;
   //int maxby = INT_MIN;

   //float maxSpeed = 0;
   for( int y = 0; y < rasterSizeY; y++ )
   {
      for( int x = 0; x < rasterSizeX; x++ )
      {
         int pixIndex = y * rasterSizeX + x;

         unsigned short terrainPixel = terrainHeightmap[ pixIndex ];

         __int64  watermapPixel;
         watermapFile->GetPixel( x, y, &watermapPixel );
         
         unsigned int val  = (unsigned int)(watermapPixel >> 32);
         float waterDepth = reinterpret_cast<float&>(val);
         float terrZ = mapDims.MinZ + terrainPixel * mapDims.SizeZ / 65535.0f;

         float vx = HalfFloatUnpack( (unsigned short)watermapPixel );
         float vy = HalfFloatUnpack( (unsigned short)(watermapPixel>>16) );
         float speed = sqrtf( vx*vx + vy*vy );
         //maxSpeed = ::max( speed, maxSpeed );

         float frictionK = ::clamp( (waterDepth - settings.DepthBasedFrictionMinDepth) / (settings.DepthBasedFrictionMaxDepth - settings.DepthBasedFrictionMinDepth), 0.0f, 1.0f );
         float perturbanceDepthMod = lerp( settings.DepthBasedFrictionAmount, 1.0f, frictionK );
         perturbanceDepthMod = powf( perturbanceDepthMod, 8.0f );

         waterDepth -= settings.WaterIgnoreBelowDepth;// * clamp( 1.0f - speed * 3.0f, 0.0f, 1.0f );

         float ignoreBelowDepthSpec = settings.WaterIgnoreBelowDepthSpec * saturate( 1 - (speed) * 40.0f );

         if( (waterDepth <= ignoreBelowDepthSpec) )
            waterHeightmap[pixIndex] = c_emptyWaterPixelID;
         else
            waterHeightmap[pixIndex] = terrZ + waterDepth;

         //minbx = ::min( minbx, (int)((vx+0.5f) * 255.0f + 0.5f) );
         //minby = ::min( minby, (int)((vy+0.5f) * 255.0f + 0.5f) );
         //maxbx = ::max( maxbx, (int)((vx+0.5f) * 255.0f + 0.5f) );
         //maxby = ::max( maxby, (int)((vy+0.5f) * 255.0f + 0.5f) );
         
         unsigned char bx = (unsigned char)clamp( (vx+0.5f) * 255.0f + 0.5f, 0.0f, 255.0f );
         unsigned char by = (unsigned char)clamp( (vy+0.5f) * 255.0f + 0.5f, 0.0f, 255.0f );

         // this should be additionally calculated by sampling neighbouring speeds and calculating difference!
         unsigned char bp = (unsigned char)clamp( (speed / 0.5f / perturbanceDepthMod) * 255.0f, 0.0f, 255.0f );

         waterInfomap[ pixIndex ] = (unsigned int)( (bp << 0) | (bx << 8) | (by << 16) );
      }
   }
}

/*
void UnpackVelocities( unsigned int waterInfomapPixel, float & vx, float & vy )
{
   vx = ((waterInfomapPixel >> 8) & 0xFF) / 255.0f - 0.5f - 0.0019608140f;
   vy = ((waterInfomapPixel >> 16) & 0xFF) / 255.0f - 0.5f - 0.0019608140f;
}

float GetSpeedDifference( unsigned int wipA, unsigned int wipB )
{
   float ax, ay, bx, by;
   UnpackVelocities( wipA, ax, ay );
   UnpackVelocities( wipB, bx, by );

   ax -= bx;
   ay -= by;

   return sqrtf( ax*ax + ay*ay );
}
*/
float GetSpeedDifference( unsigned __int64 wipA, unsigned __int64 wipB )
{
   float ax = HalfFloatUnpack( (unsigned short)wipA);
   float ay = HalfFloatUnpack( (unsigned short)(wipA>>16) );
   float bx = HalfFloatUnpack( (unsigned short)wipB);
   float by = HalfFloatUnpack( (unsigned short)(wipB>>16) );

   ax -= bx;
   ay -= by;

   return sqrtf( ax*ax + ay*ay );
}

void CalcPerturbanceFilter( float * waterHeightmap, unsigned int * waterInfomap, AdVantage::TiledBitmap * watermapFile, const int rasterSizeX, const int rasterSizeY, const MapDimensions & mapDims )
{
   float stepX = mapDims.SizeX / (float)rasterSizeX;
   float stepY = mapDims.SizeY / (float)rasterSizeY;
   float avgPixelSize = (stepX + stepY) * 0.5f;

   unsigned int * wiData0 = waterInfomap;
   unsigned int * wiData1 = waterInfomap + 1 * (rasterSizeX);
   unsigned int * wiData2 = waterInfomap + 2 * (rasterSizeX);

   float minAngle = FLT_MAX;
   float maxAngle = 0;

   for( int y = 1; y < rasterSizeY-1; y++ )
   {
      for( int x = 1; x < rasterSizeX-1; x++ )
      {
         /*
         float vx, vy;
         UnpackVelocities( wiData1[x+0], vx, vy );
         float speed = sqrtf( vx*vx + vy*vy );

         float pert = (speed / 0.5f) + 
                     ( GetSpeedDifference( wiData1[x+1], wiData1[x-1] ) + 
                     GetSpeedDifference( wiData0[x+0], wiData2[x-0] ) + 
                     GetSpeedDifference( wiData0[x-1], wiData1[x+1] ) + 
                     GetSpeedDifference( wiData1[x-1], wiData2[x+1] ) ) * 0.25f;
                     */

         /*
         __int64  c00, c01, c02;
         __int64  c10, c11, c12;
         __int64  c20, c21, c22;

         watermapFile->GetPixel( x-1, y-1, &c00 );
         watermapFile->GetPixel( x+0, y-1, &c01 );
         watermapFile->GetPixel( x+1, y-1, &c02 );
         watermapFile->GetPixel( x-1, y+0, &c10 );
         watermapFile->GetPixel( x+0, y+0, &c11 );
         watermapFile->GetPixel( x+1, y+0, &c12 );
         watermapFile->GetPixel( x-1, y+1, &c20 );
         watermapFile->GetPixel( x+0, y+1, &c21 );
         watermapFile->GetPixel( x+1, y+1, &c22 );

         float vx = HalfFloatUnpack( (unsigned short)c11 );
         float vy = HalfFloatUnpack( (unsigned short)(c11>>16) );
         float speed = sqrtf( vx*vx + vy*vy );

         float pert = (speed / 0.5f);

         pert += ( GetSpeedDifference( c00, c22 ) + 
                   GetSpeedDifference( c10, c12 ) + 
                   GetSpeedDifference( c20, c02 ) +
                   GetSpeedDifference( c01, c21 ) ) * 0.25f;
                   */

         __int64 watermapPixel;
         watermapFile->GetPixel( x, y, &watermapPixel );
         float vx = HalfFloatUnpack( (unsigned short)watermapPixel );
         float vy = HalfFloatUnpack( (unsigned short)(watermapPixel>>16) );
         float speed = sqrtf( vx*vx + vy*vy );

         float pert = (speed / 0.5f);

         //float wh00, wh01, wh02;
         //float wh10, wh11, wh12;
         //float wh20, wh21, wh22;
         //wh00 = waterHeightmap[ rasterSizeX * (y-1) + (x-1) ];
         //wh01 = waterHeightmap[ rasterSizeX * (y+0) + (x-1) ];
         //wh02 = waterHeightmap[ rasterSizeX * (y+1) + (x-1) ];
         //wh10 = waterHeightmap[ rasterSizeX * (y-1) + (x+0) ];
         //wh11 = waterHeightmap[ rasterSizeX * (y+0) + (x+0) ];
         //wh12 = waterHeightmap[ rasterSizeX * (y+1) + (x+0) ];
         //wh20 = waterHeightmap[ rasterSizeX * (y-1) + (x+1) ];
         //wh21 = waterHeightmap[ rasterSizeX * (y+0) + (x+1) ];
         //wh22 = waterHeightmap[ rasterSizeX * (y+1) + (x+1) ];
         //float d0 = 0, d1 = 0, d2 = 0, d3 = 0;
         //if( wh00 != c_emptyWaterPixelID && wh22 != c_emptyWaterPixelID )  d0 = fabsf( wh00 - wh22 );
         //if( wh10 != c_emptyWaterPixelID && wh12 != c_emptyWaterPixelID )  d0 = fabsf( wh10 - wh12 );
         //if( wh20 != c_emptyWaterPixelID && wh02 != c_emptyWaterPixelID )  d0 = fabsf( wh20 - wh02 );
         //if( wh01 != c_emptyWaterPixelID && wh21 != c_emptyWaterPixelID )  d0 = fabsf( wh01 - wh21 );

         float whC = waterHeightmap[ rasterSizeX * (y+0) + (x+0) ];
         float wh0 = waterHeightmap[ rasterSizeX * (y+0) + (x-1) ];
         float wh1 = waterHeightmap[ rasterSizeX * (y-1) + (x+0) ];
         float wh2 = waterHeightmap[ rasterSizeX * (y+0) + (x+1) ];
         float wh3 = waterHeightmap[ rasterSizeX * (y+1) + (x+0) ];

         float d0 = 0, d1 = 0, d2 = 0, d3 = 0;
         if( whC != c_emptyWaterPixelID && wh0 != c_emptyWaterPixelID )  d0 = fabsf( whC - wh0 );
         if( whC != c_emptyWaterPixelID && wh1 != c_emptyWaterPixelID )  d0 = fabsf( whC - wh1 );
         if( whC != c_emptyWaterPixelID && wh2 != c_emptyWaterPixelID )  d0 = fabsf( whC - wh2 );
         if( whC != c_emptyWaterPixelID && wh3 != c_emptyWaterPixelID )  d0 = fabsf( whC - wh3 );


         float maxDelta = ::max( ::max( d0, d1 ), ::max( fabsf( d2 ), fabsf( d3 ) ) );

         float angle = atanf( maxDelta / avgPixelSize );

         maxAngle = ::max( angle, maxAngle );
         minAngle = ::min( angle, minAngle );

         pert *= 0.6f + saturate( angle - 0.3f ) * 4.0f;

         unsigned char bp = (unsigned char)clamp( pert * 255.0f, 0.0f, 255.0f );

         wiData1[x+0] = (wiData1[x+0] & 0xFFFFFF00) | (bp << 0);

      }
      wiData0 += rasterSizeX;
      wiData1 += rasterSizeX;
      wiData2 += rasterSizeX;
   }

   int debug = 0;
   debug++;

}

void CutEdgesFilter( float * waterHeightmap, float * waterHeightmapTemp, const int rasterSizeX, const int rasterSizeY, const int c_minNeighbours )
{
   int bufferSize = rasterSizeX * rasterSizeY * sizeof(float);

   // Copy to waterHeightmapTemp only because we are not processing edge pixels. 
   // Could be removed if having even number of steps.
   memcpy( waterHeightmapTemp, waterHeightmap, bufferSize );

   float * readBuffer   = waterHeightmap;
   float * writeBuffer  = waterHeightmapTemp;

   for( int ce = 0; ce < 3; ce++ )
   {
      float * whData0 = readBuffer;
      float * whData1 = readBuffer + 1 * (rasterSizeX);
      float * whData2 = readBuffer + 2 * (rasterSizeX);
      float * whDest = writeBuffer + 1 * (rasterSizeX);

      for( int y = 1; y < rasterSizeY-1; y++ )
      {
         for( int x = 1; x < rasterSizeX-1; x++ )
         {
            whDest[x] = whData1[x];
            if( whData1[x] == c_emptyWaterPixelID )
            {
               continue;
            }

            int emptyNeighbours = 0;
            emptyNeighbours += (whData0[x-1] == c_emptyWaterPixelID)?(1):(0);
            emptyNeighbours += (whData0[x+1] == c_emptyWaterPixelID)?(1):(0);
            emptyNeighbours += (whData1[x-1] == c_emptyWaterPixelID)?(1):(0);
            emptyNeighbours += (whData1[x+1] == c_emptyWaterPixelID)?(1):(0);
            emptyNeighbours += (whData2[x-1] == c_emptyWaterPixelID)?(1):(0);
            emptyNeighbours += (whData2[x+0] == c_emptyWaterPixelID)?(1):(0);
            emptyNeighbours += (whData2[x+1] == c_emptyWaterPixelID)?(1):(0);

            if( emptyNeighbours > c_minNeighbours )
               whDest[x] = c_emptyWaterPixelID;
         }
         whData0 += rasterSizeX;
         whData1 += rasterSizeX;
         whData2 += rasterSizeX;
         whDest  += rasterSizeX;
      }
      ::swap( writeBuffer, readBuffer );
   }

   // Could be removed if having even number of steps.
   if( writeBuffer != waterHeightmapTemp )
   {
      memcpy( waterHeightmap, waterHeightmapTemp, bufferSize );
   }
}

void SmoothEdgesFilter( float * waterHeightmap, float * waterHeightmapTemp, const int rasterSizeX, const int rasterSizeY, const int c_smoothenSteps = 1 )
{
   int bufferSize = rasterSizeX * rasterSizeY * sizeof(float);

   // Copy to waterHeightmapTemp only because we are not processing edge pixels. 
   // Could be removed if having even number of steps.
   memcpy( waterHeightmapTemp, waterHeightmap, bufferSize );

   float * readBuffer   = waterHeightmap;
   float * writeBuffer  = waterHeightmapTemp;

   for( int p = 0; p < c_smoothenSteps; p++ )
   {
      float * whData0 = readBuffer;
      float * whData1 = readBuffer + 1 * (rasterSizeX);
      float * whData2 = readBuffer + 2 * (rasterSizeX);
      float * whDest = writeBuffer + 1 * (rasterSizeX);

      for( int y = 1; y < rasterSizeY-1; y++ )
      {
         for( int x = 1; x < rasterSizeX-1; x++ )
         {
            whDest[x] = whData1[x];

            // If we're empty exit
            if( whData1[x] == c_emptyWaterPixelID )
               continue;

            const int numOfAdjecent = 8;
            float h[numOfAdjecent];
            h[0] = whData0[x-1];
            h[1] = whData0[x+0];
            h[2] = whData0[x+1];
            h[3] = whData1[x-1];
            //float h[4] = whData1[x+0];
            h[4] = whData1[x+1];
            h[5] = whData2[x-1];
            h[6] = whData2[x+0];
            h[7] = whData2[x+1];

            int i;
            // Find first non-empty
            for( i = 0; i < numOfAdjecent; i++ )
            {
               if( h[i] != c_emptyWaterPixelID )
                  break;
            }

            // All neighbours are empty, exit
            if( i == numOfAdjecent )
            {
               whDest[x] = whData1[x];
               continue;
            }

            float sum = h[i];
            float count = 1;
            i++;
            for( ; i < numOfAdjecent; i++ )
               if( h[i] != c_emptyWaterPixelID )
               {
                  sum += h[i];
                  count++;
               }

               // if we're not on the edge, don't smooth us!
               if( count > 5 )
                  continue;

               sum += whData1[x] * count;
               count *= 2;

               whDest[x] = sum / count;
         }
         whData0 += rasterSizeX;
         whData1 += rasterSizeX;
         whData2 += rasterSizeX;
         whDest  += rasterSizeX;
      }
      ::swap( writeBuffer, readBuffer );
   }
   //////////////////////////////////////////////////////////////////////////

   // Could be removed if having even number of steps.
   if( writeBuffer != waterHeightmapTemp )
   {
      memcpy( waterHeightmap, waterHeightmapTemp, bufferSize );
   }
}

/*
static bool HasAnyData( float * waterHeightmap, const int rasterSizeX, const int rasterSizeY )
{
   bool hasAnyData = false;

   //////////////////////////////////////////////////////////////////////////
   // HasAnyData pass
   {
      float * whData = waterHeightmap;
      for( int y = 1; y < rasterSizeY-1; y++ )
      {
         for( int x = 1; x < rasterSizeX-1; x++ )
         {
            if( whData[x] != c_emptyWaterPixelID )
            {
               hasAnyData = true;
               break;
            }
         }
         whData += rasterSizeX;
      }
      //
   }
   //////////////////////////////////////////////////////////////////////////

   if( !hasAnyData )
   {
      // No water?
      assert( false );
   }

   return hasAnyData;
}
*/

static void ExpandEdgesFilter( float * waterHeightmap, float * waterHeightmapTemp, const int rasterSizeX, const int rasterSizeY, unsigned short * terrainHeightmap, const MapDimensions & mapDims, const ExporterSettings & settings )
{
   int stepsBeforeDropoffApplied = 3;
   int maxTotalSteps = 64;

   // This keepBelowOffset should make sure that any expansion of water heights in areas where they should be
   // below ground keeps it below ground. Increase this to remove z-fighting!
   float keepBelowOffset = settings.WaterElevateIfNotIgnored*3;

   int bufferSize = rasterSizeX * rasterSizeY * sizeof(float);

   float stepX = mapDims.SizeX / (float)rasterSizeX;
   float stepY = mapDims.SizeY / (float)rasterSizeY;
   float avgPixelSize = (stepX + stepY) * 0.5f;

   float waterDropoffRateIfBelow = avgPixelSize; // with avgPixelSize dropoff angle is 45º

   // Copy to waterHeightmapTemp only because we are not processing edge pixels. 
   // Could be removed if having even number of steps.
   memcpy( waterHeightmapTemp, waterHeightmap, bufferSize );

   float * readBuffer   = waterHeightmap;
   float * writeBuffer  = waterHeightmapTemp;

#pragma warning( push )
#pragma warning( disable : 4127 )   // yes I know it's a constant goddamit!
   while( true )
#pragma warning ( pop )
   {
      bool finished = true;

      float * whData0 = readBuffer;
      float * whData1 = readBuffer + 1 * (rasterSizeX);
      float * whData2 = readBuffer + 2 * (rasterSizeX);
      float * whDest = writeBuffer + 1 * (rasterSizeX);

      for( int y = 1; y < rasterSizeY-1; y++ )
      {
         for( int x = 1; x < rasterSizeX-1; x++ )
         {
            if( whData1[x] != c_emptyWaterPixelID )
            {
               whDest[x] = whData1[x];
               continue;
            }

            const int numOfAdjecent = 8;
            float h[numOfAdjecent];
            h[0] = whData0[x-1];
            h[1] = whData0[x+0];
            h[2] = whData0[x+1];
            h[3] = whData1[x-1];
            //float h[4] = whData1[x+0];
            h[4] = whData1[x+1];
            h[5] = whData2[x-1];
            h[6] = whData2[x+0];
            h[7] = whData2[x+1];
            finished = false;

            int i;
            // Find first non 
            for( i = 0; i < numOfAdjecent; i++ )
            {
               if( h[i] != c_emptyWaterPixelID )
                  break;
            }

            // All are empty
            if( i == numOfAdjecent )
            {
               whDest[x] = whData1[x];
               continue;
            }

            float sum = h[i];
            float count = 1;
            i++;
            for( ; i < numOfAdjecent; i++ )
               if( h[i] != c_emptyWaterPixelID )
               {
                  sum += h[i];
                  count++;
               }

               int pixIndex = y * rasterSizeX + x;
               unsigned short terrainPixel = terrainHeightmap[ pixIndex ];
               float terrZ = mapDims.MinZ + terrainPixel * mapDims.SizeZ / 65535.0f;

               // Make sure that any expansion doesn't put them above terrain!
               // (uses arbitrary value of (terrain-m_waterIgnoreBelowDepth*2) as a limit... )
               float newH = ::min( sum / count, terrZ - keepBelowOffset );

               if( stepsBeforeDropoffApplied == 0 )
                  newH = newH - waterDropoffRateIfBelow;

               whDest[x] = newH;
         }
         whData0 += rasterSizeX;
         whData1 += rasterSizeX;
         whData2 += rasterSizeX;
         whDest  += rasterSizeX;
      }

      ::swap( writeBuffer, readBuffer );

      if( stepsBeforeDropoffApplied > 0 )
         stepsBeforeDropoffApplied--;

      maxTotalSteps--;

      if( finished || (maxTotalSteps <= 0) )
         break;
   }

   // Could be removed if having even number of steps.
   if( writeBuffer != waterHeightmapTemp )
   {
      memcpy( waterHeightmap, waterHeightmapTemp, bufferSize );
   }
}

static void ElevateFilter( float * waterHeightmap, const int rasterSizeX, const int rasterSizeY, const ExporterSettings & settings )
{
   float * whData = waterHeightmap;
   for( int y = 0; y < rasterSizeY; y++ )
   {
      for( int x = 0; x < rasterSizeX; x++ )
      {
         whData[x] += settings.WaterElevateIfNotIgnored;
      }
      whData  += rasterSizeX;
   }
}

static void FinalizeFilter( float * waterHeightmap, const int rasterSizeX, const int rasterSizeY, const MapDimensions & mapDims, const ExporterSettings & settings )
{
   float waterLowestPossibleHeight = mapDims.MinZ - mapDims.SizeZ * 0.01f;

   float * whData = waterHeightmap;
   for( int y = 0; y < rasterSizeY; y++ )
   {
      for( int x = 0; x < rasterSizeX; x++ )
      {
         // Some pixels might still have c_emptyWaterPixelID value - set them to
         // the lowest value (waterLowestPossibleHeight)
         if( whData[x] == c_emptyWaterPixelID )
            whData[x] = waterLowestPossibleHeight;
         whData[x] = ::max( whData[x], waterLowestPossibleHeight );

         whData[x] = (whData[x] - mapDims.MinZ) / mapDims.SizeZ;
         assert( whData[x] > -100 && whData[x] < 100 );
      }
      whData  += rasterSizeX;
   }
}

static void SaveRenderWaterData( float * waterHeightmap, unsigned int * waterInfomap, const int rasterSizeX, const int rasterSizeY, AdVantage::TiledBitmap * rendWaterHeightmapFile, AdVantage::TiledBitmap * rendWaterInfomapFile )
{
   assert( rendWaterHeightmapFile->Width() == rasterSizeX && rendWaterHeightmapFile->Height() == rasterSizeY && rendWaterInfomapFile->Width() == rasterSizeX && rendWaterInfomapFile->Height() == rasterSizeY );
   if( !(rendWaterHeightmapFile->Width() == rasterSizeX && rendWaterHeightmapFile->Height() == rasterSizeY && rendWaterInfomapFile->Width() == rasterSizeX && rendWaterInfomapFile->Height() == rasterSizeY) )
      return;

   for( int y = 0; y < rasterSizeY; y++ )
   {
      for( int x = 0; x < rasterSizeX; x++ )
      {
         int pixIndex = y * rasterSizeX + x;

         unsigned short whmp = (unsigned short) clamp( waterHeightmap[pixIndex] * 65535.0f + 0.5f, 0.0f, 65535.0f );
         
         rendWaterHeightmapFile->SetPixel( x, y, &whmp );
         rendWaterInfomapFile->SetPixel( x, y, &waterInfomap[pixIndex] );
      }
   }
}

void wsExportRenderData( AdVantage::TiledBitmap * terrainHeightmapFile, AdVantage::TiledBitmap * watermapFile, AdVantage::TiledBitmap * rendWaterHeightmapFile, AdVantage::TiledBitmap * rendWaterInfomapFile, const MapDimensions & mapDims, const ExporterSettings & settings )
{
   int rasterSizeX =  terrainHeightmapFile->Width();
   int rasterSizeY =  terrainHeightmapFile->Height();

   assert( watermapFile->Width() == rasterSizeX && watermapFile->Height() == rasterSizeY );
   if( ! (watermapFile->Width() == rasterSizeX && watermapFile->Height() == rasterSizeY) )
      return;
   

   unsigned short * terrainHeightmap   =  new unsigned short[rasterSizeX * rasterSizeY];
   float * waterHeightmap              =  new float[rasterSizeX * rasterSizeY];
   float * waterHeightmapTemp          =  new float[rasterSizeX * rasterSizeY];  // temp buffer, used by various filters
   unsigned int * waterInfomap         =  new unsigned int[rasterSizeX * rasterSizeY];

   //  - Loading passes -

   // Load heightmap from terrainHeightmapFile to the terrainHeightmap buffer (16bit unsigned short)
   LoadTerrainHeightmap( terrainHeightmapFile, terrainHeightmap );

   // Load watermap, convert it into world space heightmap and mark areas below settings.WaterIgnoreBelowDepth with c_emptyWaterPixelID
   // Also load velocities into waterInfomap
   LoadAndInitializeWaterData( terrainHeightmap, watermapFile, waterHeightmap, waterInfomap, mapDims, settings );
   
   //  - CutEdges filter -
   // Will remove every water pixel if it has less than cutEdges_minNeighbours non-empty neighbors.
   // This removes small pools and incorrect river edges caused by imprecision in velocity 
   // water propagation algorithm.
   CutEdgesFilter( waterHeightmap, waterHeightmapTemp, rasterSizeX, rasterSizeY, 4 );

   //  - SmoothEdges filter -
   // Will smoothen edges. Not perfect yet, should only be working (and using) pixels on the edge, which
   // should be precalculated.
   SmoothEdgesFilter( waterHeightmap, waterHeightmapTemp, rasterSizeX, rasterSizeY, 1 );

   //if( HasAnyData(waterHeightmap, rasterSizeX, rasterSizeY) )
   {
      //  - Expand filter -
      // Will expand non-empty pixels using average of their non-empty neighbors for
      // expand_stepsBeforeDropoffApplied steps, and then just apply 'dropoff' filter to
      // remaining water pixels which expands non-empty pixels to empty ones while reducing 
      // heights by m_waterDropoffRateIfBelow value for each step.
      ExpandEdgesFilter( waterHeightmap, waterHeightmapTemp, rasterSizeX, rasterSizeY, terrainHeightmap, mapDims, settings );

      //  - Elevate filter -
      // Will elevate all water pixels by m_waterElevateIfNotIgnored to counter already
      // applied m_waterIgnoreBelowDepth reduction.
      ElevateFilter( waterHeightmap, rasterSizeX, rasterSizeY, settings );

      // Modifies waterInfomap perturbance info
      CalcPerturbanceFilter( waterHeightmap, waterInfomap, watermapFile, rasterSizeX, rasterSizeY, mapDims );

      //  - Finalize filter -
      // Transform heights into same space as the heightmap, and sort out remaining c_emptyWaterPixelID pixels if any,
      // etc.
      FinalizeFilter( waterHeightmap, rasterSizeX, rasterSizeY, mapDims, settings );
   }

   SaveRenderWaterData( waterHeightmap, waterInfomap, rasterSizeX, rasterSizeY, rendWaterHeightmapFile, rendWaterInfomapFile );

   delete[] terrainHeightmap;
   delete[] waterHeightmap;
   delete[] waterHeightmapTemp;
   delete[] waterInfomap;


}