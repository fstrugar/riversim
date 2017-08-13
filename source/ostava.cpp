#include "DXUT.h"
#include "wsCommon.h"

inline D3DXVECTOR3 AvgNormalFromQuad( float ha, float hb, float hc, float hd, float sizex, float sizey, float scalez )
{
   D3DXVECTOR3 n0, n1;

   n0.x = - (hb - ha) * scalez * sizey;
   n0.y = - sizex * (hc - ha) * scalez;
   n0.z = sizex * sizey;

   //D3DXVec3Normalize( &n0, &n0 );

   n1.x = -sizey * (hd-hc) * scalez;
   n1.y = ((hb-hc) * sizex - sizex * (hd-hc)) * scalez;
   n1.z = sizey * sizex;


   //D3DXVec3Normalize( &n1, &n1 );

   n0 += n1;
   //D3DXVec3Normalize( &n0, &n0 );

   return n0;
}

//D3DXVECTOR3 AvgNormalFromQuad( const D3DXVECTOR3 & a, const D3DXVECTOR3 & b, const D3DXVECTOR3 & c, const D3DXVECTOR3 & d )
//{
//   D3DXVECTOR3 v0, v1;
//   D3DXVECTOR3 n0, n1;
//
//   v0 = b - a;
//   v1 = c - a;
//   D3DXVec3Cross( &n0, &v0, &v1 );
//   D3DXVec3Normalize( &n0, &n0 );
//
//   v0 = b - c;
//   v1 = d - c;
//   D3DXVec3Cross( &n1, &v0, &v1 );
//   D3DXVec3Normalize( &n1, &n1 );
//
//   n0 += n1;
//   D3DXVec3Normalize( &n0, &n0 );
//
//   return n0;
//}

static void CreateNormalMap( int sizeX, int sizeY, MapDimensions & mapDims, float * heightmapData, int heightmapDataPitch, unsigned int * normalmapData, int normalmapDataPitch )
{
   float stepx = 1.0f / (sizeX-1) * mapDims.SizeX;
   float stepy = 1.0f / (sizeY-1) * mapDims.SizeY;

   const int smoothSteps = 0; // can be 0, 1, 2, ... more steps == slower algorithm
   for( int dist = 1; dist < 2+smoothSteps; dist++ )
   {
      for( int y = dist; y < sizeY-dist; y++ )
      {
         float * hmScanLine0 = &heightmapData[ (y-dist) * (heightmapDataPitch/sizeof(float)) ];
         float * hmScanLine1 = &heightmapData[ (y+0) * (heightmapDataPitch/sizeof(float)) ];
         float * hmScanLine2 = &heightmapData[ (y+dist) * (heightmapDataPitch/sizeof(float)) ];

         unsigned int * nmScanLine = &normalmapData[ y * (normalmapDataPitch/sizeof(unsigned int)) ];

         for( int x = dist; x < sizeX-dist; x++ )
         {
            float ha = hmScanLine0[x-dist];
            float hb = hmScanLine1[x-dist];
            float hc = hmScanLine2[x-dist];
            float hd = hmScanLine0[x+0];
            float he = hmScanLine1[x+0];
            float hf = hmScanLine2[x+0];
            float hg = hmScanLine0[x+dist];
            float hh = hmScanLine1[x+dist];
            float hi = hmScanLine2[x+dist];

            D3DXVECTOR3 norm( 0, 0, 0 );
            norm += AvgNormalFromQuad( ha, hb, hd, he, stepx, stepy, mapDims.SizeZ );
            norm += AvgNormalFromQuad( hb, hc, he, hf, stepx, stepy, mapDims.SizeZ );
            norm += AvgNormalFromQuad( hd, he, hg, hh, stepx, stepy, mapDims.SizeZ );
            norm += AvgNormalFromQuad( he, hf, hh, hi, stepx, stepy, mapDims.SizeZ );

            D3DXVec3Normalize( &norm, &norm );

            if( dist > 1 )
            {
               D3DXVECTOR3 oldNorm( HalfFloatUnpack( (unsigned short)(nmScanLine[x] & 0xFFFF) ), HalfFloatUnpack( (unsigned short)(nmScanLine[x] >> 16) ), 0 );
               oldNorm.z = sqrtf( 1 - oldNorm.x*oldNorm.x - oldNorm.y*oldNorm.y );

               norm += oldNorm * 1.0f; // use bigger const to add more weight to normals calculated from smaller quads
               D3DXVec3Normalize( &norm, &norm );
            }

            unsigned short a = HalfFloatPack( norm.x );
            unsigned short b = HalfFloatPack( norm.y );

            nmScanLine[x] = (b << 16) | a;
         }
      }
   }

   for( int y = 0; y < sizeY; y++ )
   {
      normalmapData[ y * (normalmapDataPitch/sizeof(unsigned int)) + sizeX-1 ] = normalmapData[ y * (normalmapDataPitch/sizeof(unsigned int)) + sizeX-2 ];
      normalmapData[ y * (normalmapDataPitch/sizeof(unsigned int)) + 0 ] = normalmapData[ y * (normalmapDataPitch/sizeof(unsigned int)) + 1 ];
   }

   for( int x = 0; x < sizeX; x++ )
   {
      normalmapData[ (sizeY-1) * (normalmapDataPitch/sizeof(unsigned int)) + x ] = normalmapData[ (sizeY-2) * (normalmapDataPitch/sizeof(unsigned int)) + x ];
      normalmapData[ (0) * (normalmapDataPitch/sizeof(unsigned int)) + x ] = normalmapData[ (1) * (normalmapDataPitch/sizeof(unsigned int)) + x ];
   }
}