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

#pragma warning (disable : 4995)

#include "wsCommon.h"


enum IntersectType
{
   IT_Outside,
   IT_Intersect,
   IT_Inside
};

struct wsAABB
{
   D3DXVECTOR3       Min;
   D3DXVECTOR3       Max;
   D3DXVECTOR3       Center()   { return (Min + Max) * 0.5f; }
   D3DXVECTOR3       Size()     { return Max - Min; }


   wsAABB() {}
   wsAABB( const D3DXVECTOR3 & min, const D3DXVECTOR3 & max ) : Min(min), Max(max)   { }

   void GetCornerPoints( D3DXVECTOR3 corners [] )
   {
      corners[0].x = Min.x;
      corners[0].y = Min.y;
      corners[0].z = Min.z;

      corners[1].x = Min.x;
      corners[1].y = Max.y;
      corners[1].z = Min.z;

      corners[2].x = Max.x;
      corners[2].y = Min.y;
      corners[2].z = Min.z;

      corners[3].x = Max.x;
      corners[3].y = Max.y;
      corners[3].z = Min.z;

      corners[4].x = Min.x;
      corners[4].y = Min.y;
      corners[4].z = Max.z;

      corners[5].x = Min.x;
      corners[5].y = Max.y;
      corners[5].z = Max.z;

      corners[6].x = Max.x;
      corners[6].y = Min.y;
      corners[6].z = Max.z;

      corners[7].x = Max.x;
      corners[7].y = Max.y;
      corners[7].z = Max.z;
   }

   bool Intersects( const wsAABB & other )
   {
      return !( (other.Max.x < this->Min.x) || (other.Min.x > this->Max.x) 
         || (other.Max.y < this->Min.y) || (other.Min.y > this->Max.y) 
         || (other.Max.z < this->Min.z) || (other.Min.z > this->Max.z) );
   }

   IntersectType TestInBoundingPlanes( D3DXPLANE planes[] )
   {
      D3DXVECTOR3 corners[9];
      GetCornerPoints(corners);
      corners[8] = Center();

      D3DXVECTOR3 boxSize = Size();
      float size = D3DXVec3Length( &boxSize );

      // test box's bounding sphere against all planes - removes many false tests, adds one more check
      for(int p = 0; p < 6; p++) 
         if( D3DXPlaneDotCoord( &planes[p], &corners[8] ) < -size/2 )
            return IT_Outside;

      int totalIn = 0;
      size /= 6.0f; //reduce size to 1/4 (half of radius) for more precision!! // tweaked to 1/6, more sensible

      // test all 8 corners and 9th center point against the planes
      // if all points are behind 1 specific plane, we are out
      // if we are in with all points, then we are fully in
      for(int p = 0; p < 6; p++) 
      {

         int inCount = 9;
         int ptIn = 1;

         for(int i = 0; i < 9; ++i) 
         {

            // test this point against the planes
            float distance = D3DXPlaneDotCoord( &planes[p], &corners[i] );
            if (distance < -size) 
            {
               ptIn = 0;
               inCount--;
            }
         }

         // were all the points outside of plane p?
         if (inCount == 0) 
            return IT_Outside;

         // check if they were all on the right side of the plane
         totalIn += ptIn;
      }

      if( totalIn == 6 )
         return IT_Inside;

      return IT_Intersect;
   }

   float MinDistanceFromPointSq( const D3DXVECTOR3 & point )
   {
      float dist = 0.0f;

      if (point.x < Min.x)
      {
         float d=point.x-Min.x;
         dist+=d*d;
      }
      else
         if (point.x > Max.x)
         {
            float d=point.x-Max.x;
            dist+=d*d;
         }

         if (point.y < Min.y)
         {
            float d=point.y-Min.y;
            dist+=d*d;
         }
         else
            if (point.y > Max.y)
            {
               float d=point.y-Max.y;
               dist+=d*d;
            }

            if (point.z < Min.z)
            {
               float d=point.z-Min.z;
               dist+=d*d;
            }
            else
               if (point.z > Max.z)
               {
                  float d=point.z-Max.z;
                  dist+=d*d;
               }

               return dist;
   }

   float MaxDistanceFromPointSq( const D3DXVECTOR3 & point )
   {
      float dist = 0.0f;
      float k;

      k = ::max( fabsf( point.x - Min.x ), fabsf( point.x - Max.x ) );
      dist += k * k;

      k = ::max( fabsf( point.y - Min.y ), fabsf( point.y - Max.y ) );
      dist += k * k;

      k = ::max( fabsf( point.z - Min.z ), fabsf( point.z - Max.z ) );
      dist += k * k;

      return dist;
   }

   bool	IntersectSphereSq( const D3DXVECTOR3 & center, float radiusSq )
   {
      return MinDistanceFromPointSq( center ) <= radiusSq;
   }

   bool IsInsideSphereSq( const D3DXVECTOR3 & center, float radiusSq )
   {
      return MaxDistanceFromPointSq( center ) <= radiusSq;
   }

   bool IntersectRay( const D3DXVECTOR3 & rayOrigin, const D3DXVECTOR3 & rayDirection, float & distance )
   {
      //Microsoft.DirectX.Direct3D.Box

      // Intersect ray R(t) = p + t*d against AABB a. When intersecting,
      // return intersection distance tmin and point q of intersection
      float tmin = -FLT_MAX;        // set to -FLT_MAX to get first hit on line
      float tmax = FLT_MAX;		   // set to max distance ray can travel (for segment)

      float _rayOrigin []     = { rayOrigin.x, rayOrigin.y, rayOrigin.z };
      float _rayDirection []  = { rayDirection.x, rayDirection.y, rayDirection.z };
      float _min []           = { Min.x, Min.y, Min.z };
      float _max []           = { Max.x, Max.y, Max.z };

      const float EPSILON = 1e-5f;

      // For all three slabs
      for (int i = 0; i < 3; i++) 
      {
         if ( fabsf(_rayDirection[i]) < EPSILON) 
         {
            // Ray is parallel to slab. No hit if origin not within slab
            if (_rayOrigin[i] < _min[i] || _rayOrigin[i] > _max[i]) 
            {
               //assert( !k );
               //hits1_false++;
               return false;
            }
         } 
         else 
         {
            // Compute intersection t value of ray with near and far plane of slab
            float ood = 1.0f / _rayDirection[i];
            float t1 = (_min[i] - _rayOrigin[i]) * ood;
            float t2 = (_max[i] - _rayOrigin[i]) * ood;
            // Make t1 be intersection with near plane, t2 with far plane
            if (t1 > t2) ::swap( t1, t2 );
            // Compute the intersection of slab intersections intervals
            if (t1 > tmin) tmin = t1;
            if (t2 < tmax) tmax = t2;
            // Exit with no collision as soon as slab intersection becomes empty
            if (tmin > tmax)
            {
               //assert( !k );
               //hits1_false++;
               return false;
            }

         }
      }
      // Ray intersects all 3 slabs. Return point (q) and intersection t value (tmin) 
      //D3DXVECTOR3 v = rayOrigin + rayDirection * tmin;
      //assert(k);
      //hits1_true++;

      distance = tmin;

      return true;

      //	return D3DXBoxBoundProbe(&Min, &Max, &rayOrigin, &rayDirection) == TRUE;
   }

   static wsAABB   Enclosing( const wsAABB & a, const wsAABB & b )
   {
      D3DXVECTOR3 bmin, bmax;

      bmin.x = ::min( a.Min.x, b.Min.x );
      bmin.y = ::min( a.Min.y, b.Min.y );
      bmin.z = ::min( a.Min.z, b.Min.z );

      bmax.x = ::max( a.Max.x, b.Max.x );
      bmax.y = ::max( a.Max.y, b.Max.y );
      bmax.z = ::max( a.Max.z, b.Max.z );

      return wsAABB( bmin, bmax );
   }

   /*
   public static wsAABB? Enclosing( wsAABB? na, wsAABB? nb )
   {
   if( na == null ) return nb;
   if( nb == null ) return na;

   wsAABB a = na.GetValueOrDefault();
   wsAABB b = nb.GetValueOrDefault();

   D3DXVECTOR3 min, max;

   min.x = ::min( a.Min.x, b.Min.x );
   min.y = ::min( a.Min.y, b.Min.y );
   min.z = ::min( a.Min.z, b.Min.z );

   max.x = ::max( a.Max.x, b.Max.x );
   max.y = ::max( a.Max.y, b.Max.y );
   max.z = ::max( a.Max.z, b.Max.z );

   return new wsAABB( min, max );
   }
   */
};
