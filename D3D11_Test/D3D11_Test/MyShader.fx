Texture2D texDiffuse;
SamplerState samLinear;

struct VertexIn
{
	float3 PosL  : POSITION;
	float4 Color : COLOR;
	
	float3 normal: NORMAL;

	float2 tex : TEXCOORD;
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
	float4 Color : COLOR;

	float4 normal : NORMAL;

	float2 tex : TEXCOORD;
};

cbuffer ConstantBuffer
{
	float4x4 wvp;
	float4x4 world;
	
	float4 lightDir;
	float4 lightColor;
};
/*VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	// Transform to homogeneous clip space.
	vout.PosH = float4(vin.PosL, 1.f);

	// Just pass vertex color into the pixel shader.
	vout.Color = vin.Color;

	return vout;
}*/

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	vout.PosH = mul(float4(vin.PosL, 1.f), wvp);
	vout.Color = vin.Color;

	vout.normal = mul(float4(vin.normal, 0.f), world);

	vout.tex = vin.tex;

	return vout;
}

float4 PS(VertexOut pin):SV_TARGET
{
	float4 finalColor = 0;

	finalColor = saturate((dot((float3) - lightDir, pin.normal)*0.5+0.5)*lightColor);
	
	float4 texColor = texDiffuse.Sample(samLinear, pin.tex) * finalColor;
	texColor.a = 1.f;

	return texColor;
}

technique11 ColorTech
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_5_0, VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, PS()));
	}
}
