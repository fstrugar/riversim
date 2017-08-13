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

class ws3DPreviewCamera;

class wsQuadTree
{
public:

   static const int     c_maxRenderLayers    = 16;

   struct Node;

   struct NodeSelection
   {
      Node *      pNode;
      bool        TL;
      bool        TR;
      bool        BL;
      bool        BR;
      float       minDistToCamera;

      NodeSelection()      {}
      NodeSelection( Node * pNode, bool tl, bool tr, bool bl, bool br ) : pNode( pNode ), TL(tl), TR(tr), BL(bl), BR(br)   { }
   };

   struct LODSelectionInfo
   {
      float       MorphEnd[c_maxRenderLayers];
      float       MorphStart[c_maxRenderLayers];

      void        GetMorphConsts( const NodeSelection * selectedNode, float consts[4] );
   };

   struct Node
   {
      friend class wsQuadTree;

      unsigned short    X;
      unsigned short    Y;
      unsigned short    Size;

      unsigned short    Level;

      // To save memory we can calculate this at runtime, but no point in doing that right now...
      float             WorldMinX;
      float             WorldMinY;
      float             WorldMaxX;
      float             WorldMaxY;
      float             WorldMinZ;
      float             WorldMaxZ;

      float             TLZ;
      float             TRZ;
      float             BLZ;
      float             BRZ;

      // To save memory we could use ushort indices in m_allNodesBuffer buffer, but no point in doing that right now...
      Node *            SubTL;
      Node *            SubTR;
      Node *            SubBL;
      Node *            SubBR;

      void              DebugDrawAABB( unsigned int penColor );
      void              FillSubNodes( Node * nodes[4], int & count );

   private:
      void              Create( int x, int y, int size, int stopSize, int level, const MapDimensions & dims, int rasterSizeX, int rasterSizeY, float * heightField, int heightFieldPitch, Node * allNodesBuffer, int & allNodesBufferLastIndex );

      bool              LODSelect( NodeSelection * selection, int maxSelectionCount, int & selectionCount, D3DXVECTOR3 cameraPosition, float lodRanges[], D3DXPLANE frustumPlanes[], int stopAtLevel );
      void              GetAreaMinMaxHeight( int fromX, int fromY, int toX, int toY, float & minZ, float & maxZ );

      bool              IntersectRay( const D3DXVECTOR3 & rayOrigin, const D3DXVECTOR3 & rayDirection, float maxDistance, D3DXVECTOR3 & hitPoint );
   };

                                   
private:                           

   Node *            m_allNodesBuffer;
   int               m_allNodesCount;

   MapDimensions     m_mapDims;

   int               m_nodeMinSize;

   int               m_numberOfLevels;

   Node ***          m_baseLevel;
   int               m_baseNodeSize;
   int               m_baseDimX;
   int               m_baseDimY;

   int               m_rasterSizeX;
   int               m_rasterSizeY;



public:
   wsQuadTree();
   virtual ~wsQuadTree();

   void           Create( MapDimensions dims, int rasterSizeX, int rasterSizeY, float * heightField, int heightFieldPitch, int nodeMinSize );
   void           Clean( );

   void           DebugDrawAllNodes( );

   int            LODSelect( NodeSelection * selection, int maxSelectionCount, LODSelectionInfo & selectionInfo, D3DXPLANE planes[6], D3DXVECTOR3 cameraPos, float visibilityDistance );

   bool           IntersectRay( const D3DXVECTOR3 & rayOrigin, const D3DXVECTOR3 & rayDirection, float maxDistance, D3DXVECTOR3 & hitPoint );

   void           GetAreaMinMaxHeight( float fromX, float fromY, float sizeX, float sizeY, float & minZ, float & maxZ );

};
