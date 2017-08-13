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

#include "TiledBitmap/TiledBitmap.h"

struct ExporterSettings
{
   float    WaterIgnoreBelowDepth;
   float    WaterIgnoreBelowDepthSpec;
   float    WaterElevateIfNotIgnored;

   // These are simulation constants but are also used to calculate perturbance info
   float    DepthBasedFrictionMinDepth;
   float    DepthBasedFrictionMaxDepth;
   float    DepthBasedFrictionAmount;

};

void wsExportRenderData( AdVantage::TiledBitmap * terrainHeightmapFile, AdVantage::TiledBitmap * watermapFile, AdVantage::TiledBitmap * rendWaterHeightmapFile, AdVantage::TiledBitmap * rendWaterInfomapFile, const MapDimensions & mapDims, const ExporterSettings & settings );
