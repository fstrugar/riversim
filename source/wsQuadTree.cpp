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

#include "wsQuadTree.h"

#include "wsCanvas.h"

#include "ws3DPreviewCamera.h"

#include "wsAABB.h"


wsQuadTree::wsQuadTree()
{
   m_allNodesBuffer = NULL;
   m_baseLevel = NULL;
}

wsQuadTree::~wsQuadTree()
{
   Clean();
}

void wsQuadTree::Create( MapDimensions dims, int rasterSizeX, int rasterSizeY, float * heightField, int heightFieldPitch, int nodeMinSize )
{
   Clean();

   m_mapDims      = dims;
   m_rasterSizeX  = rasterSizeX;
   m_rasterSizeY  = rasterSizeY;
   m_nodeMinSize  = nodeMinSize/2;

   //////////////////////////////////////////////////////////////////////////
   // Determine how many depth levels our tree will have, and how many nodes it will use.
   //
   int levelCount = 1;
   const int stopOnBaseCount = 3;
   m_baseNodeSize = nodeMinSize;
   int bottomNodesSizeX = (rasterSizeX-1) / nodeMinSize + 1;
   int bottomNodesSizeY = (rasterSizeY-1) / nodeMinSize + 1;
   //
   int maxNodeCount = bottomNodesSizeX * bottomNodesSizeY;
   //
   while( ((bottomNodesSizeX > stopOnBaseCount && bottomNodesSizeY > stopOnBaseCount) || (levelCount < 2)) && (levelCount < c_maxRenderLayers) )
   {
      bottomNodesSizeX = ::max( (bottomNodesSizeX+1) / 2, 1 );
      bottomNodesSizeY = ::max( (bottomNodesSizeY+1) / 2, 1 );

      maxNodeCount += (bottomNodesSizeX+1) * (bottomNodesSizeY+1);
      levelCount++;
      m_baseNodeSize *= 2;
   }
   m_numberOfLevels = levelCount;
   //////////////////////////////////////////////////////////////////////////

   //////////////////////////////////////////////////////////////////////////
   // Initialize tree memory and stuff, create tree nodes, and extract min/max Zs (heights)
   //
   m_allNodesBuffer = new Node[maxNodeCount];
   int nodeCounter = 0;
   //
   m_baseDimX = bottomNodesSizeX;
   m_baseDimY = bottomNodesSizeY;
   m_baseLevel = new Node**[m_baseDimY];
   for( int y = 0; y < m_baseDimY; y++ )
   {
      m_baseLevel[y] = new Node*[m_baseDimX];
      for( int x = 0; x < m_baseDimX; x++ )
      {
         m_baseLevel[y][x] = &m_allNodesBuffer[nodeCounter];
         nodeCounter++;

         m_baseLevel[y][x]->Create( x * m_baseNodeSize, y * m_baseNodeSize, m_baseNodeSize, nodeMinSize, 0, dims, rasterSizeX, rasterSizeY, heightField, heightFieldPitch, m_allNodesBuffer, nodeCounter );
      }
   }
   m_allNodesCount = nodeCounter;
   //assert( nodeCounter == maxNodeCount );
   //////////////////////////////////////////////////////////////////////////
}

void wsQuadTree::Node::Create( int x, int y, int size, int stopSize, int level, const MapDimensions & dims, int rasterSizeX, int rasterSizeY, float * heightField, int heightFieldPitch, Node * allNodesBuffer, int & allNodesBufferLastIndex )
{
   this->X     = (unsigned short)x;       assert( x >= 0 && x < 65535 );
   this->Y     = (unsigned short)y;       assert( y >= 0 && y < 65535 );
   this->Level = (unsigned short)level;   assert( level >= 0 && level < 16 );
   this->Size  = (unsigned short)size;    assert( size >= 0 && level < 32768 );

   this->WorldMinX   = dims.MinX + (x / (float)(rasterSizeX-1)) * dims.SizeX;
   this->WorldMinY   = dims.MinY + (y / (float)(rasterSizeY-1)) * dims.SizeY;
   this->WorldMaxX   = dims.MinX + ((x+size) / (float)(rasterSizeX-1)) * dims.SizeX;
   this->WorldMaxY   = dims.MinY + ((y+size) / (float)(rasterSizeY-1)) * dims.SizeY;

   this->SubTL = NULL;
   this->SubTR = NULL;
   this->SubBL = NULL;
   this->SubBR = NULL;

   // Are we done yet?
   if( size == stopSize )
   {
      // Find min/max heights at this patch of terrain
      float minZ = FLT_MAX;
      float maxZ = -FLT_MAX;
      assert( heightFieldPitch % sizeof(float) == 0 ); // heightFieldPitch is in bytes - must be divisible by sizeof(float)!
      for( int ry = 0; ry < size; ry++ )
      {
         if( (ry + y) >= rasterSizeY )
            break;
         float * scanLine = &heightField[ x + (ry+y) * (heightFieldPitch/sizeof(float)) ];
         int sizeX = ::min( rasterSizeX - x, size );
         for( int rx = 0; rx < sizeX; rx++ )
         {
            float h = scanLine[rx];
            minZ = ::min( minZ, h );
            maxZ = ::max( maxZ, h );
         }
      }
      {
         int limitX = ::min( rasterSizeX - 1, x + size );
         int limitY = ::min( rasterSizeY - 1, y + size );
         TLZ = dims.MinZ + heightField[ (x+0)      + (y+0)     * (heightFieldPitch/sizeof(float)) ] * dims.SizeZ;
         TRZ = dims.MinZ + heightField[ (limitX)   + (y+0)     * (heightFieldPitch/sizeof(float)) ] * dims.SizeZ;
         BLZ = dims.MinZ + heightField[ (x+0)      + (limitY)  * (heightFieldPitch/sizeof(float)) ] * dims.SizeZ;
         BRZ = dims.MinZ + heightField[ (limitX)   + (limitY)  * (heightFieldPitch/sizeof(float)) ] * dims.SizeZ;
      }

      // Convert to world space...
      this->WorldMinZ = dims.MinZ + minZ * dims.SizeZ;
      this->WorldMaxZ = dims.MinZ + maxZ * dims.SizeZ;
   }
   else
   {
      int subSize = size / 2;

      this->SubTL = &allNodesBuffer[allNodesBufferLastIndex++];
      this->SubTL->Create( x, y, subSize, stopSize, level+1, dims, rasterSizeX, rasterSizeY, heightField, heightFieldPitch, allNodesBuffer, allNodesBufferLastIndex );
      this->WorldMinZ = this->SubTL->WorldMinZ;
      this->WorldMaxZ = this->SubTL->WorldMaxZ;

      if( (x + subSize) < rasterSizeX )
      {
         this->SubTR = &allNodesBuffer[allNodesBufferLastIndex++];
         this->SubTR->Create( x + subSize, y, subSize, stopSize, level+1, dims, rasterSizeX, rasterSizeY, heightField, heightFieldPitch, allNodesBuffer, allNodesBufferLastIndex );
         this->WorldMinZ = ::min( this->WorldMinZ, this->SubTL->WorldMinZ );
         this->WorldMaxZ = ::max( this->WorldMaxZ, this->SubTR->WorldMaxZ );
      }

      if( (y + subSize) < rasterSizeY )
      {
         this->SubBL = &allNodesBuffer[allNodesBufferLastIndex++];
         this->SubBL->Create( x, y + subSize, subSize, stopSize, level+1, dims, rasterSizeX, rasterSizeY, heightField, heightFieldPitch, allNodesBuffer, allNodesBufferLastIndex );
         this->WorldMinZ = ::min( this->WorldMinZ, this->SubBL->WorldMinZ );
         this->WorldMaxZ = ::max( this->WorldMaxZ, this->SubBL->WorldMaxZ );
      }

      if( ((x + subSize) < rasterSizeX) && ((y + subSize) < rasterSizeY) )
      {
         this->SubBR = &allNodesBuffer[allNodesBufferLastIndex++];
         this->SubBR->Create( x + subSize, y + subSize, subSize, stopSize, level+1, dims, rasterSizeX, rasterSizeY, heightField, heightFieldPitch, allNodesBuffer, allNodesBufferLastIndex );
         this->WorldMinZ = ::min( this->WorldMinZ, this->SubBR->WorldMinZ );
         this->WorldMaxZ = ::max( this->WorldMaxZ, this->SubBR->WorldMaxZ );
      }
   }
}

void wsQuadTree::Node::DebugDrawAABB( unsigned int penColor )
{
   GetCanvas3D()->DrawBox( D3DXVECTOR3( this->WorldMinX, this->WorldMinY, this->WorldMinZ ), D3DXVECTOR3( this->WorldMaxX, this->WorldMaxY, this->WorldMaxZ ), penColor );
}

bool wsQuadTree::Node::LODSelect( NodeSelection * selection, int maxSelectionCount, int & selectionCount, D3DXVECTOR3 cameraPosition, float lodRanges[], D3DXPLANE frustumPlanes[], int stopAtLevel )
{
   wsAABB boundingBox( D3DXVECTOR3( WorldMinX, WorldMinY, WorldMinZ ), D3DXVECTOR3( WorldMaxX, WorldMaxY, WorldMaxZ ) );

   //float boxSize = boundingBox.Size.Length();

   //   float cameraBoxCenterDistance = D3DXVec3Length(&(boundingBox.GetCenter() - camera.Position));

   IntersectType it = boundingBox.TestInBoundingPlanes( frustumPlanes );
   if( it == IT_Outside )
      return false;

   float distanceLimit = lodRanges[this->Level];

   if( !boundingBox.IntersectSphereSq(cameraPosition, distanceLimit*distanceLimit) )
      return false;

   bool bSubTLSelected = false;
   bool bSubTRSelected = false;
   bool bSubBLSelected = false;
   bool bSubBRSelected = false;

   if( this->Level != stopAtLevel )
   {
      float nextDistanceLimit = lodRanges[this->Level+1];
      if( boundingBox.IntersectSphereSq( cameraPosition, nextDistanceLimit*nextDistanceLimit ) )
      {
         // trebalo bi sad markirati selektovane subquadove i koristiti adequate index buffer or whatever
         if( SubTL != NULL ) bSubTLSelected = this->SubTL->LODSelect( selection, maxSelectionCount, selectionCount, cameraPosition, lodRanges, frustumPlanes, stopAtLevel );
         if( SubTR != NULL ) bSubTRSelected = this->SubTR->LODSelect( selection, maxSelectionCount, selectionCount, cameraPosition, lodRanges, frustumPlanes, stopAtLevel );
         if( SubBL != NULL ) bSubBLSelected = this->SubBL->LODSelect( selection, maxSelectionCount, selectionCount, cameraPosition, lodRanges, frustumPlanes, stopAtLevel );
         if( SubBR != NULL ) bSubBRSelected = this->SubBR->LODSelect( selection, maxSelectionCount, selectionCount, cameraPosition, lodRanges, frustumPlanes, stopAtLevel );

         //if( (bSubTLSelected && bSubTRSelected && bSubTLSelected ... ) || boundingBox.IsInsideSphereSq( cameraPosition, nextDistanceLimit ) )
         //   return false;
      }

      //if( !subTL.IsValid() ) bSubTLSelected = true;
      //if( !subTR.IsValid() ) bSubTRSelected = true;
      //if( !subBL.IsValid() ) bSubBLSelected = true;
      //if( !subBR.IsValid() ) bSubBRSelected = true;
   }

   if( !(bSubTLSelected && bSubTRSelected && bSubBLSelected && bSubBRSelected) && (selectionCount < maxSelectionCount) )
      selection[selectionCount++] = NodeSelection( this, !bSubTLSelected, !bSubTRSelected, !bSubBLSelected, !bSubBRSelected );

   return true;
}

void wsQuadTree::Clean( )
{
   if( m_allNodesBuffer != NULL )
      delete[] m_allNodesBuffer;
   m_allNodesBuffer = NULL;

   if( m_baseLevel != NULL )
   {
      for( int y = 0; y < m_baseDimY; y++ )
         delete[] m_baseLevel[y];

	  delete[] m_baseLevel;
      m_baseLevel = NULL;
   }
}

void wsQuadTree::DebugDrawAllNodes( )
{
   for( int i = 0; i < m_allNodesCount; i++ )
      if( m_allNodesBuffer[i].Level != 0 ) m_allNodesBuffer[i].DebugDrawAABB( 0xFF00FF00 );
   for( int i = 0; i < m_allNodesCount; i++ )
      if( m_allNodesBuffer[i].Level == 0 ) m_allNodesBuffer[i].DebugDrawAABB( 0xFFFF0000 );
}

int compare_closerFirst( const void *arg1, const void *arg2 )
{
   const wsQuadTree::NodeSelection * a = (const wsQuadTree::NodeSelection *)arg1;
   const wsQuadTree::NodeSelection * b = (const wsQuadTree::NodeSelection *)arg2;

   return a->minDistToCamera > b->minDistToCamera;
}

int wsQuadTree::LODSelect( NodeSelection * selection, int maxSelectionCount, LODSelectionInfo & selectionInfo, D3DXPLANE planes[6], D3DXVECTOR3 cameraPos, float visibilityDistance )
{
   //const D3DXVECTOR3 & cameraPos = camera->GetPosition();

   float LODNear           = visibilityDistance * 0.003f;
   float LODFar            = visibilityDistance;
   float detailBalance     = 3.0f;

   int   layerCount        = m_numberOfLevels-1;

   //// correct LOD far if required
   //int   diff = maxLayers - (root.RenderLevel+1);
   //for( int i = 0; i < diff; i++ ) LODFar /= detailBalance;

   float total                = 0;
   float currentDetailBalance = 1.0f;

   float visibilityRanges[32]; assert( layerCount <= 32 ); // = new float[layerCount];

   for (int i=0; i<layerCount; i++) 
   {
      total += currentDetailBalance;
      currentDetailBalance *= detailBalance;
   }

   float sect = (LODFar - LODNear) / total;

   float prevPos = LODNear;
   currentDetailBalance = 1.0f;
   for( int i = 0; i < layerCount; i++ ) 
   {
      visibilityRanges[layerCount - i - 1] = prevPos + sect * currentDetailBalance;
      prevPos = visibilityRanges[layerCount - i - 1];
      currentDetailBalance *= detailBalance;
   }

   prevPos = LODNear;//camera.LODNear;
   for (int i=0; i<layerCount; i++)
   {
      int index = layerCount - i - 1;
      selectionInfo.MorphEnd[index] = visibilityRanges[index];
      selectionInfo.MorphStart[index] = (prevPos + selectionInfo.MorphEnd[index] * 3) / 4;

      //TWEAK:
      //!!Can be temporarily switched for stronger morph appearance!!
      //selectionInfo.MorphStart[index] = (prevPos + selectionInfo.MorphEnd[index] * 10) / 11;

      prevPos = selectionInfo.MorphStart[index];
   }

   //float oldFOV = camera->GetFOV();
   //camera->SetFOV( oldFOV / 2 );
   //camera->Tick(0.0f);

   //D3DXPLANE planes[6];
   //camera->GetFrustumPlanes( planes );

   //camera->SetFOV( oldFOV );
   //camera->Tick(0.0f);

   int selectionCount = 0;

   for( int y = 0; y < m_baseDimY; y++ )
      for( int x = 0; x < m_baseDimX; x++ )
      {
         m_baseLevel[y][x]->LODSelect( selection, maxSelectionCount, selectionCount, cameraPos, visibilityRanges, planes, layerCount-1 );
      }

   for( int i = 0; i < selectionCount; i++ )
   {
      const wsQuadTree::Node & node = *selection[i].pNode;
      wsAABB naabb( D3DXVECTOR3(node.WorldMinX, node.WorldMinY, node.WorldMinZ), D3DXVECTOR3(node.WorldMaxX, node.WorldMaxY, node.WorldMaxZ) );

      selection[i].minDistToCamera = sqrtf( naabb.MinDistanceFromPointSq( cameraPos ) );
   }

   qsort( selection, selectionCount, sizeof(selection[0]), compare_closerFirst );

   //TerrainQuadSelection[] sorted = new TerrainQuadSelection[list.Count];
   //int sortedCount = 0;

   //Vector3 cameraPos = camera.GetPosition();
   //while( list.Count > 0 )
   //{
   //   int   index    = -1;
   //   float minDist  = float.MaxValue;
   //   for( int i = 0; i < list.Count; i++ )
   //   {
   //      float dist = (cameraPos - list[i].Quad.BoundingBox.Center).LengthSq();
   //      if( dist < minDist )
   //      {
   //         minDist = dist;
   //         index = i;
   //      }
   //   }

   //   if( index == -1 ) throw new Exception( "Error while sorting." );

   //   sorted[sortedCount++] = list[index];
   //   list.RemoveAt(index);
   //}
   //if( sortedCount != sorted.Length ) throw new Exception( "Error while sorting." );

   //return sorted;

   return selectionCount;
}

void wsQuadTree::LODSelectionInfo::GetMorphConsts( const NodeSelection * selectedNode, float consts[4] )
{
   int layerLevel = selectedNode->pNode->Level;
   float mStart = MorphStart[layerLevel];
   float mEnd = MorphEnd[layerLevel];

   consts[0] = mStart;
   consts[1] = 1.0f / (mEnd-mStart);
   //
   consts[2] = mEnd / (mEnd-mStart);
   consts[3] = 1.0f / (mEnd-mStart);
}

void wsQuadTree::Node::GetAreaMinMaxHeight( int fromX, int fromY, int toX, int toY, float & minZ, float & maxZ )
{
   if( ((toX < this->X) || (toY < this->Y)) || ((fromX > (this->X+this->Size) ) || (fromY > (this->Y+this->Size)) ))
   {
      // Completely outside
      return;
   }

   bool hasNoLeafs = this->SubTL == NULL;

   if( hasNoLeafs || (((fromX <= this->X) && (fromY <= this->Y)) && ((toX >= (this->X+this->Size) ) && (toY >= (this->Y+this->Size)))) )
   {
      // Completely inside
      minZ = ::min( minZ, this->WorldMinZ );
      maxZ = ::max( maxZ, this->WorldMaxZ );
      //DebugDrawAABB( 0x80FF0000 );
      return;
   }

   // Partially inside, partially outside
   if( this->SubTL != NULL )
      this->SubTL->GetAreaMinMaxHeight( fromX, fromY, toX, toY, minZ, maxZ );
   if( this->SubTR != NULL )
      this->SubTR->GetAreaMinMaxHeight( fromX, fromY, toX, toY, minZ, maxZ );
   if( this->SubBL != NULL )
      this->SubBL->GetAreaMinMaxHeight( fromX, fromY, toX, toY, minZ, maxZ );
   if( this->SubBR != NULL )
      this->SubBR->GetAreaMinMaxHeight( fromX, fromY, toX, toY, minZ, maxZ );
}

void wsQuadTree::GetAreaMinMaxHeight( float fromX, float fromY, float sizeX, float sizeY, float & minZ, float & maxZ )
{
   float bfx = (fromX - m_mapDims.MinX) / m_mapDims.SizeX;
   float bfy = (fromY - m_mapDims.MinY) / m_mapDims.SizeY;
   float btx = (fromX + sizeX - m_mapDims.MinX) / m_mapDims.SizeX;
   float bty = (fromY + sizeY - m_mapDims.MinY) / m_mapDims.SizeY;

   int rasterFromX   = clamp( (int)(bfx * m_rasterSizeX), 0, m_rasterSizeX-1 );
   int rasterFromY   = clamp( (int)(bfy * m_rasterSizeY), 0, m_rasterSizeY-1 );
   int rasterToX     = clamp( (int)(btx * m_rasterSizeX), 0, m_rasterSizeX-1 );
   int rasterToY     = clamp( (int)(bty * m_rasterSizeY), 0, m_rasterSizeY-1 );

   int baseFromX   = rasterFromX / m_baseNodeSize;
   int baseFromY   = rasterFromY / m_baseNodeSize;
   int baseToX     = rasterToX / m_baseNodeSize;
   int baseToY     = rasterToY / m_baseNodeSize;

   assert( baseFromX < m_baseDimX );
   assert( baseFromY < m_baseDimY );
   assert( baseToX < m_baseDimX );
   assert( baseToY < m_baseDimY );

   minZ = FLT_MAX;
   maxZ = -FLT_MAX;

   for( int y = baseFromY; y <= baseToY; y++ )
      for( int x = baseFromX; x <= baseToX; x++ )
      {
         m_baseLevel[y][x]->GetAreaMinMaxHeight( rasterFromX, rasterFromY, rasterToX, rasterToY, minZ, maxZ );
      }

   //GetCanvas3D()->DrawBox( D3DXVECTOR3(fromX, fromY, minZ), D3DXVECTOR3(fromX + sizeX, fromY + sizeY, maxZ), 0xFFFFFF00, 0x10FFFF00 );
}
//
void wsQuadTree::Node::FillSubNodes( Node * nodes[4], int & count )
{
   count = 0;
   if( this->SubTL != NULL )
      nodes[count++] = this->SubTL;
   if( this->SubTR != NULL )
      nodes[count++] = this->SubTR;
   if( this->SubBL != NULL )
      nodes[count++] = this->SubBL;
   if( this->SubBR != NULL )
      nodes[count++] = this->SubBR;

}
//
/* Ray-Triangle Intersection Test Routines          */
/* Different optimizations of my and Ben Trumbore's */
/* code from journals of graphics tools (JGT)       */
/* http://www.acm.org/jgt/                          */
/* by Tomas Moller, May 2000                        */
//
#define _IT_CROSS(dest,v1,v2) \
   dest[0]=v1[1]*v2[2]-v1[2]*v2[1]; \
   dest[1]=v1[2]*v2[0]-v1[0]*v2[2]; \
   dest[2]=v1[0]*v2[1]-v1[1]*v2[0];
#define _IT_DOT(v1,v2) (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2])
#define _IT_SUB(dest,v1,v2) \
   dest[0]=v1[0]-v2[0]; \
   dest[1]=v1[1]-v2[1]; \
   dest[2]=v1[2]-v2[2]; 
bool IntersectTri(const D3DXVECTOR3 & _orig, const D3DXVECTOR3 & _dir, const D3DXVECTOR3 & _vert0, const D3DXVECTOR3 & _vert1, 
                          const D3DXVECTOR3 & _vert2, float &u, float &v, float &dist)
{
   const float c_epsilon = 1e-6f;

   float * orig = (float *)&_orig;
   float * dir = (float *)&_dir;
   float * vert0 = (float *)&_vert0;
   float * vert1 = (float *)&_vert1;
   float * vert2 = (float *)&_vert2;

   float edge1[3], edge2[3], tvec[3], pvec[3], qvec[3];
   float det,inv_det;

   // find vectors for two edges sharing vert0
   _IT_SUB(edge1, vert1, vert0);
   _IT_SUB(edge2, vert2, vert0);

   // begin calculating determinant - also used to calculate U parameter
   _IT_CROSS(pvec, dir, edge2);

   /* if determinant is near zero, ray lies in plane of triangle */
   det = _IT_DOT(edge1, pvec);

   /* calculate distance from vert0 to ray origin */
   _IT_SUB(tvec, orig, vert0);
   inv_det = 1.0f / det;

   if (det > c_epsilon)
   {
      /* calculate U parameter and test bounds */
      u = _IT_DOT(tvec, pvec);
      if (u < 0.0 || u > det)
         return false;

      /* prepare to test V parameter */
      _IT_CROSS(qvec, tvec, edge1);

      /* calculate V parameter and test bounds */
      v = _IT_DOT(dir, qvec);
      if (v < 0.0 || u + v > det)
         return false;

   }
   else if(det < -c_epsilon)
   {
      /* calculate U parameter and test bounds */
      u = _IT_DOT(tvec, pvec);
      if (u > 0.0 || u < det)
         return false;

      /* prepare to test V parameter */
      _IT_CROSS(qvec, tvec, edge1);

      /* calculate V parameter and test bounds */
      v = _IT_DOT(dir, qvec) ;
      if (v > 0.0 || u + v < det)
         return false;
   }
   else return false;  /* ray is parallel to the plane of the triangle */

   /* calculate t, ray intersects triangle */
   dist = _IT_DOT(edge2, qvec) * inv_det;
   u *= inv_det;
   v *= inv_det;

   return true;
}
//
bool wsQuadTree::Node::IntersectRay( const D3DXVECTOR3 & rayOrigin, const D3DXVECTOR3 & rayDirection, float maxDistance, D3DXVECTOR3 & hitPoint )
{
   wsAABB boundingBox( D3DXVECTOR3( WorldMinX, WorldMinY, WorldMinZ ), D3DXVECTOR3( WorldMaxX, WorldMaxY, WorldMaxZ ) );

   float hitDistance = FLT_MAX;
   if( !boundingBox.IntersectRay(rayOrigin, rayDirection, hitDistance ) )
      return false;

   if( hitDistance > maxDistance )
      return false;

   //DebugDrawAABB( 0xFF0000FF );

   Node * subNodes[4];
   int subNodeCount;
   FillSubNodes( subNodes, subNodeCount );

   if( subNodeCount == 0 )
   {
      D3DXVECTOR3 tl( WorldMinX, WorldMinY, TLZ );
      D3DXVECTOR3 tr( WorldMaxX, WorldMinY, TRZ );
      D3DXVECTOR3 bl( WorldMinX, WorldMaxY, BLZ );
      D3DXVECTOR3 br( WorldMaxX, WorldMaxY, BRZ );
      //GetCanvas3D()->DrawTriangle( tl, tr, bl, 0xFF000000, 0x8000FF00 );
      //GetCanvas3D()->DrawTriangle( tr, bl, br, 0xFF000000, 0x8000FF00 );

      float u0, v0, dist0;
      float u1, v1, dist1;
      bool t0 = IntersectTri( rayOrigin, rayDirection, tl, tr, bl, u0, v0, dist0 );
      bool t1 = IntersectTri( rayOrigin, rayDirection, tr, bl, br, u1, v1, dist1 );

      // No hits
      if( !t0 && !t1 )
         return false;

      // Only 0 hits, or 0 is closer
      if( (t0 && !t1) || ((t0 && t1) && (dist0 < dist1)) )
      {
         hitPoint = rayOrigin + rayDirection * dist0;
         return true;
      }

      hitPoint = rayOrigin + rayDirection * dist1;
      return true;
   }

   float       closestHitDist = FLT_MAX;
   D3DXVECTOR3 closestHit;

   for( int i = 0; i < subNodeCount; i++ )
   {
      D3DXVECTOR3 hit;
      if( subNodes[i]->IntersectRay( rayOrigin, rayDirection, maxDistance, hit ) )
      {
         D3DXVECTOR3 diff = hit - rayOrigin;
         float dist = D3DXVec3Length(&diff);
         assert( dist <= maxDistance );
         if( dist < closestHitDist )
         {
            closestHitDist = dist;
            closestHit = hit;
         }
      }
   }

   if( closestHitDist != FLT_MAX )
   {
      hitPoint = closestHit;      

      //D3DXVECTOR3 vz( 0.5f, 0.5f, 200.0f );
      //GetCanvas3D()->DrawBox( hitPoint, hitPoint + vz, 0xFF000000, 0x8000FF00 );

      return true;
   }

   return false;
}
//
bool wsQuadTree::IntersectRay( const D3DXVECTOR3 & rayOrigin, const D3DXVECTOR3 & rayDirection, float maxDistance, D3DXVECTOR3 & hitPoint )
{

   float       closestHitDist = FLT_MAX;
   D3DXVECTOR3 closestHit;

   for( int y = 0; y < m_baseDimY; y++ )
      for( int x = 0; x < m_baseDimX; x++ )
      {
         D3DXVECTOR3 hit;
         if( m_baseLevel[y][x]->IntersectRay( rayOrigin, rayDirection, maxDistance, hit ) )
         {
            D3DXVECTOR3 diff = hit - rayOrigin;
            float dist = D3DXVec3Length(&diff);
            assert( dist <= maxDistance );
            if( dist < closestHitDist )
            {
               closestHitDist = dist;
               closestHit = hit;
            }
         }
      }

   if( closestHitDist != FLT_MAX )
   {
      hitPoint = closestHit;      
      return true;
   }

   return false;
}
//