
sampler     g_skyCubeMapTexture;
float4x4    g_worldViewProjection;

struct VS_SKYBOXOUTPUT
{
   float4 Position   : POSITION;   // vertex position 
   float3 CubeUV     : TEXCOORD0;  // vertex texture coords 
};

VS_SKYBOXOUTPUT vsSkybox( float4 vPos : POSITION )
{
   VS_SKYBOXOUTPUT Output;
   
   float3 pos = float3( vPos.x, vPos.y, vPos.z );
   
   Output.Position = mul( vPos, g_worldViewProjection );
   Output.CubeUV = normalize( pos ); 
   
   return Output;    
}

void psSkybox( VS_SKYBOXOUTPUT In, out float4 outColor : COLOR0, out float4 outDepth : COLOR1  )
{ 
   //return float4( 1, 1, 1, 1 );
   outColor = texCUBE( g_skyCubeMapTexture, In.CubeUV );
   outDepth = 1e6;
}