//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#include "wsCommon.sh"

float4      g_gridOffset;           // .z holds the z center of the AABB!
float4      g_gridScale;
float2      g_gridWorldMax;         // .xy max used to clamp triangles outside of world range

float4      g_terrainScale;
float4      g_terrainOffset;

float4      g_waterTexConsts;       // xy - offset, zw - scale

float4      g_morphConsts;

float4x4    g_viewProjection;

float4      g_cameraPos;

float4      g_diffuseLightDir;

float2      g_samplerWorldToTextureScale;
float2      g_samplerPixelSize;

sampler     g_surfaceTerrainHMVertexTexture;     // used to sample terrain when rendering water in order to determine water depth
sampler     g_surfaceWaterHMVertexTexture;       // used to sample water when rendering water in order to determine water depth

sampler     g_surfaceHMVertexTexture;
sampler     g_surfaceNMVertexTexture;
sampler     g_surfaceVMVertexTexture;

float4      g_fadeConsts;

float       g_CWSVertexHeightOffsetAverage;
float       g_CWSVertexHeightOffsetScale;
float       g_CWSVertexDropoffK;
float       g_CWSVertexDropoffDepth;
sampler     g_CWSCombinedHeightMapTexture;
float       g_CWSTimeLerpK;

//float       g_waterZOffset;

static float c_gridDim         = 32.0;
static float c_gridDimHalf     = c_gridDim * 0.5;
static float c_gridDimHalfRec  = 1.0 / c_gridDimHalf;

// returns baseVertexPos where: xy are true values, z is g_gridOffset.z which is z center of the AABB
float4 getBaseVertexPos( float4 inPos )
{
   float4 ret = inPos * g_gridScale + g_gridOffset;
   ret.xy = min( ret.xy, g_gridWorldMax );
   return ret;
}

// returns amount of morph required to blend from high to low detailed mesh
float calcMorphLerpK( float4 vertex )
{
   float distanceToCamera = distance( vertex, g_cameraPos );
   return 1.0f - clamp( g_morphConsts.z - distanceToCamera * g_morphConsts.w, 0.0, 1.0 );
}

// morphs vertex xy from from high to low detailed mesh position using given morphLerpK
float2 morphVertex( float4 inPos, float2 vertex, float morphLerpK )
{
   float2 fracPart = (frac( inPos.xy * float2(c_gridDimHalf, c_gridDimHalf) ) * float2(c_gridDimHalfRec, c_gridDimHalfRec) ) * g_gridScale.xy;
   return vertex.xy - fracPart * morphLerpK;
}

// calc big texture tex coords
float2 calcGlobalUV( float2 vertex )
{
   float2 globalUV = (vertex.xy - g_terrainOffset.xy) / g_terrainScale.xy;  // this can be combined into one inPos * a + b
   globalUV *= g_samplerWorldToTextureScale;
   globalUV += g_samplerPixelSize * 0.5;
   return globalUV;
}

float sampleCWSHeight( float2 uv )
{
   float2 samp = tex2Dlod( g_CWSCombinedHeightMapTexture, float4( uv.x, uv.y, 0.0, 0.0 ) );
   return (lerp( samp.y, samp.x, g_CWSTimeLerpK ) - g_CWSVertexHeightOffsetAverage) * c_combinedHeightmapPrecisionMod;
}

PreviewVertexOutput main( float4 inPos : POSITION, uniform bool isWater, uniform bool applyCWSOffset )
{
   PreviewVertexOutput output;
   
   float4 vertex     = getBaseVertexPos( inPos );
   
   float2 preGlobalUV = calcGlobalUV( vertex.xy );
   vertex.z = tex2Dlod( g_surfaceHMVertexTexture, float4( preGlobalUV.x, preGlobalUV.y, 0, 0 ) ).x * g_terrainScale.z + g_terrainOffset.z;
   
   float morphLerpK  = calcMorphLerpK( vertex );
   vertex.xy         = morphVertex( inPos, vertex.xy, morphLerpK );
 
   float2 globalUV   = calcGlobalUV( vertex.xy );

   ////////////////////////////////////////////////////////////////////////////   
   // should probably use mipmaps for performance reasons but I'll leave that for later....
   float4 vertexSamplerUV = float4( globalUV.x, globalUV.y, 0, 0 );

   ////////////////////////////////////////////////////////////////////////////   
   // sample height and calculate it
   vertex.z = tex2Dlod( g_surfaceHMVertexTexture, vertexSamplerUV ).x * g_terrainScale.z + g_terrainOffset.z;
   vertex.w = 1.0;   // this could also be set simply by having g_gridOffset.w = 1 and g_gridScale.w = 0....
   
   float2 waterTexUV = float2(0, 0);
   float waterDepth  = 0.0;
   float approxWaterEyeDepth = 0.0;
   if( isWater )
   {
      waterTexUV = (vertex.xy - g_waterTexConsts.xy) / g_waterTexConsts.zw;
      
      // this will need to be stored in water vertex too...
      float terrainZ = tex2Dlod( g_surfaceTerrainHMVertexTexture, vertexSamplerUV ).x * g_terrainScale.z + g_terrainOffset.z;
      waterDepth           = vertex.z - terrainZ;
      if( applyCWSOffset )
      {
         //
         float waveOffset = sampleCWSHeight(waterTexUV) * g_CWSVertexHeightOffsetScale;
         //
         float dropoffK = lerp( g_CWSVertexDropoffK, 1.0, saturate( waterDepth / g_CWSVertexDropoffDepth ) );
         waveOffset *= dropoffK;
         
         // this code will (hopefully) prevent offset waves pushing water below terrain
         // (I say 'hopefully' because it relays on some magic numbers, I should change this....)
         if( waveOffset < 0 )
         {
            float allowedOffset = max( 0.000001, max( 0, waterDepth ) - g_waterMinVisibleLimitDepth );

            // smooth limit
            float div = -waveOffset / allowedOffset;
            waveOffset = -allowedOffset * (0.7 * min(1.0, div) + 0.3 * (1.0 - 1.0 / max(1.0, div)));

            // just clamp
            // waveOffset = max( waveOffset, -allowedOffset ); 
         }

         vertex.z += waveOffset;
         waterDepth           = vertex.z - terrainZ;
      }
   }
   else
   {
      float waterZ = tex2Dlod( g_surfaceWaterHMVertexTexture, vertexSamplerUV ).x * g_terrainScale.z + g_terrainOffset.z;
      
      waterDepth           = waterZ - vertex.z;
   }
   
   float3 worldPos = vertex.xyz;
   
   ////////////////////////////////////////////////////////////////////////////   
   // sample water velocity if needed
   float waterFadeAway = 0;
   if( isWater ) 
   {
      // Calculate distance to camera in order to calculate fade-to-lower-LOD const, but reduce z-influence 
      // a bit as it adds to layer visibility but doesn't hurt morph
      //float3 tweakedVertex = float3( vertex.x, vertex.y, vertex.z * 0.5 + g_cameraPos.z * 0.5 );
      float distanceToCamera = distance( worldPos, g_cameraPos.xyz );
      
      waterFadeAway = clamp(distanceToCamera * g_fadeConsts.y + g_fadeConsts.x, -0.001, 1.0);
      waterFadeAway *= clamp((distanceToCamera - g_fadeConsts.z)*1000000.0, -1.0, 1.0);
   }
   
   ////////////////////////////////////////////////////////////////////////////   
   // sample normal (warning, modifying vertexSamplerUV! )
   vertexSamplerUV.xy += g_samplerPixelSize * 0.5f; // minor hack: use sampler linear filter to blur normals!
   float3 normal  = tex2Dlod( g_surfaceNMVertexTexture, vertexSamplerUV );
   normal.z = sqrt( 1 - normal.x * normal.x - normal.y * normal.y );
   
   ////////////////////////////////////////////////////////////////////////////   
   // move directional light and eye direction into normal space
 	float3 eyeDir     = normalize( (float3)g_cameraPos - (float3)vertex );
   float3 lightDir   = g_diffuseLightDir;
   //
   float3 tangent         = normalize( cross( float3( 0, 1, 0 ), normal ) );
   float3 bitangent       = normalize( cross( normal, tangent ) ); // maybe needs normalize 
   //
   float3x3 worldToTangentSpace;
   worldToTangentSpace[0] = tangent;
   worldToTangentSpace[1] = bitangent;
   worldToTangentSpace[2] = normal;
   //
   if( !isWater )
   {
      // if not water output in tangent space, otherwise we need world space and will do the rest in pixel shader
      eyeDir            = mul( worldToTangentSpace, eyeDir );
      lightDir          = mul( worldToTangentSpace, lightDir );   
   }
   else
   {
      // we need this for submerged surface refraction
      // this is pure approximation, adjusted with pow to reduce it a bit
      approxWaterEyeDepth  = max(0, waterDepth / pow( dot( eyeDir, normal ), 0.5 ) );
   }
   
   vertex = mul( vertex, g_viewProjection );
   //vertex *= 1 / vertex.w;
   
   output.worldPos         = float4( worldPos, distance( worldPos, g_cameraPos.xyz ) );
   output.position         = vertex;
   output.globalUV_cwmUV   = float4( globalUV.x, globalUV.y, waterTexUV.x, waterTexUV.y );
   output.eyeDir           = eyeDir;
   output.lightDir         = lightDir;
   //output.waterTexUV       = waterTexUV;
   output.waterConsts      = float3( waterFadeAway, waterDepth, approxWaterEyeDepth );  // .x = waterFade, .y = waterDepth, .z = approxWaterEyeDepth
   
   if( isWater )
   {
      output.distFromCamera = distance( worldPos, g_cameraPos.xyz );
      
      //worldToTangentSpace  = transpose( worldToTangentSpace );
      output.worldToTang0  = worldToTangentSpace[0];
      output.worldToTang1  = worldToTangentSpace[1];
      output.worldToTang2  = worldToTangentSpace[2];
   }
   else
   {
      output.distFromCamera = 0.0;
      output.worldToTang0  = float3( 0, 0, 0 );
      output.worldToTang1  = float3( 0, 0, 0 );
      output.worldToTang2  = float3( 0, 0, 0 );
   }
   
   //Output.SubTextureUV = float2( vertex.x, vertex.y ) - g_NearWaveSimWorldOffset) / g_NearWaveSimWorldSize;

   return output;    
}

PreviewVertexDepthOnlyOutput outputWaterDepth( float4 inPos : POSITION )
{
   PreviewVertexDepthOnlyOutput output;
   
   float4 vertex     = getBaseVertexPos( inPos );
   float morphLerpK  = calcMorphLerpK( vertex );
   vertex.xy         = morphVertex( inPos, vertex.xy, morphLerpK );
 
   float2 globalUV   = calcGlobalUV( vertex.xy );

   ////////////////////////////////////////////////////////////////////////////   
   // should probably use mipmaps for performance reasons but I'll leave that for later....
   float4 vertexSamplerUV = float4( globalUV.x, globalUV.y, 0, 0 );

   ////////////////////////////////////////////////////////////////////////////   
   // sample height and calculate it
   vertex.z = tex2Dlod( g_surfaceHMVertexTexture, vertexSamplerUV ).x * g_terrainScale.z + g_terrainOffset.z;
   vertex.w = 1.0;   // this could also be set simply by having g_gridOffset.w = 1 and g_gridScale.w = 0....
   float3 worldPos = vertex.xyz;

   float waterZ = tex2Dlod( g_surfaceWaterHMVertexTexture, vertexSamplerUV ).x * g_terrainScale.z + g_terrainOffset.z;
   
   float waterDepth = waterZ - vertex.z;
   
   vertex = mul( vertex, g_viewProjection );
   //vertex *= 1 / vertex.w;
   
   output.position         = vertex;
   output.waterDepth       = waterDepth;
   output.distFromCamera   = distance( worldPos, g_cameraPos );

   return output;    
}

PreviewVertexOutput main_terrain( float4 inPos : POSITION )
{
   return main( inPos, false, false );
}

PreviewVertexOutput main_water( float4 inPos : POSITION )
{
   return main( inPos, true, false );
}

PreviewVertexOutput main_water_cwsoffset( float4 inPos : POSITION )
{
   return main( inPos, true, true );
}

DrawWaterInfoMapVertexOutput velocityMap( float4 inPos : POSITION )
{
   float4 vertex     = getBaseVertexPos( inPos );
   float morphLerpK  = calcMorphLerpK( vertex );
   vertex.xy         = morphVertex( inPos, vertex.xy, morphLerpK );
 
   float2 globalUV   = calcGlobalUV( vertex.xy );
   float4 vertexSamplerUV = float4( globalUV.x, globalUV.y, 0, 0 );

   vertex.z = tex2Dlod( g_surfaceHMVertexTexture, vertexSamplerUV ).x * g_terrainScale.z + g_terrainOffset.z;
   vertex.w = 1.0;   // this could also be set simply by having g_gridOffset.w = 1 and g_gridScale.w = 0....
  
   DrawWaterInfoMapVertexOutput output;
   output.position         = mul( vertex, g_viewProjection );
   output.waterInfo        = tex2Dlod( g_surfaceVMVertexTexture, vertexSamplerUV );
   
   return output;
}