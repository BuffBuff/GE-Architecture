#pragma pack_matrix(row_major)
//################################
//		Shared Structs
//################################
struct VSLightInput
{
	float3	vposition		: POSITION;
	float3	lightPos		: LPOSITION;
    float3	lightColor		: COLOR;
	float3	lightDirection	: DIRECTION;
    float2	spotlightAngles	: ANGLE;
    float	lightRange		: RANGE;
	float	lightIntensity	: INTENSITY;
};

struct VSLightOutput
{
	float4	vposition		: SV_Position;
	float3	lightPos		: LPOSITION;
    float3	lightColor		: COLOR;
	float3	lightDirection	: DIRECTION;
    float2	spotlightAngles	: ANGLE;
    float	lightRange		: RANGE;
	float	lightIntensity	: INTENSITY;
};

//################################
//		HELPER FUNCTIONS
//################################

float4 GetGBufferDiffuseSpec(in int3 p_SampleIndex, in Texture2D p_DiffuseTex)
{
	return p_DiffuseTex.Load(p_SampleIndex);
}

float3 GetGBufferNormal(in int3 p_SampleIndex, in Texture2D p_NormalTex)
{
	return p_NormalTex.Load(p_SampleIndex).xyz * 2.f - 1.f;
}

float4 GetWorldPosition(in int3 p_SampleIndex, in Texture2D p_vPosTex)
{
	return p_vPosTex.Load(p_SampleIndex);	
}

void GetGBufferAttributes(in float2 p_ScreenPos,in float p_SSAOScale, in Texture2D p_NormalTex, in Texture2D p_DiffuseTex,
	in Texture2D p_SSAO_Tex, in Texture2D p_WPosTex, out float3 p_Normal, out float3 p_DiffuseAlbedo,
	out float3 p_SSAO, out float3 p_Position)
{
	int3 sampleIndex = int3(p_ScreenPos,0);

	p_Normal = GetGBufferNormal(sampleIndex, p_NormalTex);

	float4 diffuseTexSample = GetGBufferDiffuseSpec(sampleIndex, p_DiffuseTex);
	p_DiffuseAlbedo = diffuseTexSample.xyz;	

	int3 scaledScreenPos = int3(p_ScreenPos * p_SSAOScale, 0);
	//scaledScreenPos *= p_SSAOScale;
	p_SSAO = p_SSAO_Tex.Load(scaledScreenPos).xyz;

	float4 wPosTexSample = GetWorldPosition(sampleIndex, p_WPosTex);
	p_Position = wPosTexSample.xyz;
}

Texture2D gDistanceTex : register(t0);
Texture2D gColorTex : register(t1);

#ifndef FOG_COLOR
#define FOG_COLOR float4(0.3f, 0.3f, 0.45f, 1.0f)
#endif
#ifndef MIN_DISTANCE
#define MIN_DISTANCE 3000.0f
#endif
#ifndef MAX_DISTANCE
#define MAX_DISTANCE 20000.0f
#endif

cbuffer cb : register(b0)
{
	float4x4	cViewMatrix;
	float4x4	cProjectionMatrix;
	float3		cCameraPosition;
	float		cSSAO_Scale;
}

float4 DistanceFogVS(float3 screenCoord : POSITION) : SV_POSITION
{
	return float4(screenCoord, 1.0f);
}

float4 DistanceFogPS(float4 input : SV_POSITION) : SV_TARGET
{
	int3 sampleIndex = int3(input.xy, 0);
	
	float4 position = float4(GetWorldPosition(sampleIndex, gDistanceTex).xyz, 1.0f);
	position = mul(cViewMatrix, position);
	float distance = position.z;

	float4 color = gColorTex.Load(sampleIndex);

	//Get a fog factor (thickness of fog) based on the distance
	float fogFactor = (distance - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE);
	fogFactor = saturate(fogFactor);
	
	color = lerp(color, FOG_COLOR, fogFactor);

	return color;
}