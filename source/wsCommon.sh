//////////////////////////////////////////////////////////////////////aatags<aa78hjy>//
// AdVantage Terrain SDK, Copyright (C) 2004 - 2008 Filip Strugar.
// 
// Distributed as a FREE PUBLIC part of the SDK source code.
// For conditions of use, please read the SDK license file.
//////////////////////////////////////////////////////////////////////aatage<aa78hjy>//

// modifies the stored height value in the combined heightmap so that there's enough
// number range to store upper layers
static float c_combinedHeightmapPrecisionMod = 64;    // !!WARNING!! if changing these, change them in the code too!
static float c_combinedHeightmapReduceInflMod = 0.7;  // !!WARNING!! if changing these, change them in the code too!

sampler        g_heightmapTexture;
sampler        g_depthmapTexture;
sampler        g_velocitymapTexture;

float          g_waterIgnoreBelowDepth;

float          g_waterMinVisibleLimitDepth;

struct FixedVertexOutput
{
   float4   position          : POSITION;   // vertex position 
   float2   tex0              : TEXCOORD0;
};

struct DrawWaterInfoMapVertexOutput
{
   float4   position       : POSITION;
   float3   waterInfo      : TEXCOORD0;      // .xy = velocities, .z = perturbance
};

struct PreviewVertexOutput
{
   float4   position       : POSITION;
   float4   globalUV_cwmUV       : TEXCOORD0;      // .xy = globalUV, .zw = waterTexUV
   //float2   gridUV         : TEXCOORD1;
   //float2   waterTexUV     : TEXCOORD1;
   float3   eyeDir         : TEXCOORD1;
   float3   lightDir       : TEXCOORD2;
   float3   waterConsts    : TEXCOORD3;      // .x = waterFade, .y = waterDepth, .z = approxWaterEyeDepth
   float    distFromCamera : TEXCOORD4;
   float3   worldToTang0   : TEXCOORD5;
   float3   worldToTang1   : TEXCOORD6;
   float3   worldToTang2   : TEXCOORD7;
   float4   worldPos       : TEXCOORD8;
};

struct PreviewVertexDepthOnlyOutput
{
   float4   position       : POSITION;
   float    waterDepth     : TEXCOORD0;
   float    distFromCamera : TEXCOORD1;
};

//-----------------------------------------------------------------------------
// Vertex shader output structure
//-----------------------------------------------------------------------------
struct VS_OUTPUT
{
   float4   position       : POSITION;   // vertex position 
   //float3   light          : COLOR0;
   
   float4   normal3_fog1      : COLOR0;
   
   float4   tex2_parentTex2         : TEXCOORD0;
   float4   tileTex2_detailTex2     : TEXCOORD1;
   
   float4   eyeDir_morphLerpK       : TEXCOORD2;
   float3   lightDirection          : TEXCOORD3;
   
   /*
   float2   TexCoords;
   float2   ParentTexCoords;
   float2   TileTexCoords;
   float2   DetailTexCoords;
   float    MorphLerpK;
   float    FogK;

   // In vertex normal space
   float3   LightDirection;
   float3   EyeDirection;
   */
};

#ifdef PIXEL_SHADER

// x - multiplies morph distance for blend, y - adds to result; set to (1, 0) for morph, (0, 1) for always morphed, (0, 0) for never morphed
bool                    g_doTexMorph;

float4                  g_lightColorDiffuse;
float4                  g_lightColorAmbient;
float4                  g_fogColor;

//texture                 g_SimpleOverlayBase;
//texture                 g_SimpleOverlayMorph;


float3 UncompressDXT5_NM(float4 normPacked)
{
   float3 norm = float3( normPacked.w * 2.0 - 1.0, normPacked.x * 2.0 - 1.0, 0 );
   norm.z = sqrt( 1 - norm.x * norm.x - norm.y * norm.y );
   return norm;
}

float CalculateDiffuseStrength( float3 normal, float3 lightDir )
{
   return saturate( -dot( normal, lightDir ) );
}

float CalculateSpecularStrength( float3 normal, float3 lightDir, float3 eyeDir )
{
   float3 diff    = saturate( dot(normal, -lightDir) );
   float3 reflect = normalize( 2 * diff * normal + lightDir ); 
   
   return saturate( dot( reflect, eyeDir ) );
}

float3 CalculateLighting( float3 normal, float3 lightDir, float3 eyeDir, float specularPow, float specularMul )
{
   float3 light0 = normalize( lightDir );

   return g_lightColorAmbient +
            g_lightColorDiffuse * CalculateDiffuseStrength( normal, light0 )
            + g_lightColorDiffuse * specularMul * pow( CalculateSpecularStrength( normal, light0, eyeDir ), specularPow );
}

void computeSobel3x3Offsets( float2 uv, float2 pixelSize, out float2 uv00, out float2 uv10, out float2 uv20, 
                                                         out float2 uv01, out float2 uv21,
                                                         out float2 uv02, out float2 uv12, out float2 uv22 )
{
   // Sobel 3x3
	//    0,0 | 1,0 | 2,0
	//    ----+-----+----
	//    0,1 | 1,1 | 2,1
	//    ----+-----+----
	//    0,2 | 1,2 | 2,2

	uv00 = uv + float2( -pixelSize.x,   -pixelSize.y );
	uv10 = uv + float2(  0.0,           -pixelSize.y );
	uv20 = uv + float2(  pixelSize.x,   -pixelSize.y );

	uv01 = uv + float2( -pixelSize.x,   0.0          );
	uv21 = uv + float2(  pixelSize.x,   0.0          );

	uv02 = uv + float2( -pixelSize.x,   pixelSize.y );
	uv12 = uv + float2(          0.0,   pixelSize.y );
	uv22 = uv + float2(  pixelSize.x,   pixelSize.y );
}

void sampleSobel3x3CombinedHeights( sampler tex, float2 uv00, float2 uv10, float2 uv20, float2 uv01, float2 uv21,
                                                   float2 uv02, float2 uv12, float2 uv22,
                                                   out float4 a, out float4 b, float timeLerpK )
{
   float2 sample00 = tex2D( tex, uv00 );
   float2 sample10 = tex2D( tex, uv10 );
   float2 sample20 = tex2D( tex, uv20 );
   float2 sample01 = tex2D( tex, uv01 );
   float2 sample21 = tex2D( tex, uv21 );
   float2 sample02 = tex2D( tex, uv02 );
   float2 sample12 = tex2D( tex, uv12 );
   float2 sample22 = tex2D( tex, uv22 );
   
   float4 ca = float4 ( sample00.x, sample10.x, sample20.x, sample01.x );
   float4 cb = float4 ( sample21.x, sample02.x, sample12.x, sample22.x );

   float4 pa = float4 ( sample00.y, sample10.y, sample20.y, sample01.y );
   float4 pb = float4 ( sample21.y, sample02.y, sample12.y, sample22.y );

	a = lerp( pa, ca, timeLerpK );
	b = lerp( pb, cb, timeLerpK );
}

void sampleSobel3x3HeightsX( sampler tex, float2 uv00, float2 uv10, float2 uv20, float2 uv01, float2 uv21,
                                                   float2 uv02, float2 uv12, float2 uv22,
                                                   out float4 a, out float4 b )
{
	a.x      = tex2D( tex, uv00 ).x;
	a.y      = tex2D( tex, uv10 ).x;
	a.z      = tex2D( tex, uv20 ).x;
	a.w      = tex2D( tex, uv01 ).x;
	b.x      = tex2D( tex, uv21 ).x;
	b.y      = tex2D( tex, uv02 ).x;
	b.z      = tex2D( tex, uv12 ).x;
	b.w      = tex2D( tex, uv22 ).x;
}

void sampleSobel3x3HeightsY( sampler tex, float2 uv00, float2 uv10, float2 uv20, float2 uv01, float2 uv21,
                                                   float2 uv02, float2 uv12, float2 uv22,
                                                   out float4 a, out float4 b )
{
	a.x      = tex2D( tex, uv00 ).y;
	a.y      = tex2D( tex, uv10 ).y;
	a.z      = tex2D( tex, uv20 ).y;
	a.w      = tex2D( tex, uv01 ).y;
	b.x      = tex2D( tex, uv21 ).y;
	b.y      = tex2D( tex, uv02 ).y;
	b.z      = tex2D( tex, uv12 ).y;
	b.w      = tex2D( tex, uv22 ).y;
}

float3 computeSobel3x3Normal( float4 a, float4 b, float normalStrengthK )
{
	float h00 = a.x;
	float h10 = a.y;
	float h20 = a.z;

	float h01 = a.w;
	float h21 = b.x;

	float h02 = b.y;
	float h12 = b.z;
	float h22 = b.w;
	
	// The Sobel X kernel is:
	//
	// [ 1.0  0.0  -1.0 ]
	// [ 2.0  0.0  -2.0 ]
	// [ 1.0  0.0  -1.0 ]
	
	float Gx = h00 - h20 + 2.0 * h01 - 2.0 * h21 + h02 - h22;
				
	// The Sobel Y kernel is:
	//
	// [  1.0    2.0    1.0 ]
	// [  0.0    0.0    0.0 ]
	// [ -1.0   -2.0   -1.0 ]
	
	float Gy = h00 + 2.0 * h10 + h20 - h02 - 2.0 * h12 - h22;
	
	//Gx += sign( Gx ) * pow( abs( Gx * 2 ), 6 );
	//Gy += sign( Gy ) * pow( abs( Gy * 2 ), 6 );

   return normalize( float3( Gx, Gy, normalStrengthK ) );
}

#endif

float4   g_justFillColor_Color;

float4 JustFillColor( FixedVertexOutput input ) : COLOR0
{
   return g_justFillColor_Color;
}