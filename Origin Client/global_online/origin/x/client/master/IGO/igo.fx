//--------------------------------------------------------------------------------------
// IGO FX Renderer
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Buffer Variables
//--------------------------------------------------------------------------------------
cbuffer cbNeverChanges : register( b0 )
{
    float4x4 ViewProj;
};

cbuffer cbDX12ProjMatrix : register(b0)
{
	float cbDX12ProjMatrix_11;
	float cbDX12ProjMatrix_22;
	float cbDX12ProjMatrix_33;
	float cbDX12ProjMatrix_34;
};

cbuffer cbChangesEveryFrame : register( b1 )
{
    float4x4 World;

#define StretchFactor Color.x
#define TransparencyFactor Color.a
    float4 Color;
};

cbuffer cbDX12XFormAndAlpha : register(b1)
{
	float cbDX12XFormAndAlpha_X;
	float cbDX12XFormAndAlpha_Y;
	int   cbDX12XFormAndAlpha_WH;
	float cbDX12XFormAndAlpha_alpha;
	float cbDX12StretchFactor;
};

// texture
Texture2D Texture : register( t0 );
Texture2D TexCoordMap : register( t1 );
Texture2D YUVIndexMap : register( t2 );

SamplerState samPoint : register(s0)
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Clamp;
    AddressV = Clamp;
};

SamplerState samLinear: register(s0)
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
};
//--------------------------------------------------------------------------------------
// Depth State
//--------------------------------------------------------------------------------------
DepthStencilState DisableDepth
{
	DepthEnable = FALSE;
	DepthWriteMask = ZERO;
};

BlendState BlendAlpha
{
	AlphaToCoverageEnable = false;
	BlendEnable[0] = true;
	SrcBlendAlpha = INV_DEST_ALPHA;
	DestBlendAlpha = ONE;
	SrcBlend = SRC_ALPHA;
	DestBlend = INV_SRC_ALPHA;
};

BlendState BlendNoAlpha
{
	AlphaToCoverageEnable = false;
	BlendEnable[0] = false;
	SrcBlendAlpha = ONE;
	DestBlendAlpha = ZERO;
	SrcBlend = ONE;
	DestBlend = ZERO;	
};

RasterizerState SetCWCulling
{
	FillMode = SOLID;
	CullMode = BACK;
	FrontCounterClockwise = FALSE;
};

//--------------------------------------------------------------------------------------
// Input/Output structs
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
};

struct DX12PS_INPUT
{
	float4 Pos : SV_POSITION;
	float3 Tex : TEXCOORD0;
};

struct PS_INPUT_BACKGROUND
{
    float4 Pos              : SV_POSITION;
    float4 ScreenCoords     : TEXCOORD0;
    float  SmoothStepLimit  : TEXCOORD1;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;
    float4 tmp = mul(input.Pos, World);
    output.Pos = mul(tmp, ViewProj);
    output.Tex = input.Tex;
    
    return output;
}

PS_INPUT_BACKGROUND VS_Background(VS_INPUT input)
{
    PS_INPUT_BACKGROUND output = (PS_INPUT_BACKGROUND)0;
    float4 tmp = mul(input.Pos, World);
    output.Pos = mul(tmp, ViewProj);
    
    float2 screenSize = float2(World[0][0], World[1][1]);
    //float screenRatio = screenSize.y / screenSize.x;
    float screenAnimStretch = StretchFactor + 0.8;
    float4 resizedDims = float4(/* screenSize.x * screenRatio */ screenSize.y * screenAnimStretch, -screenSize.y * screenAnimStretch, screenSize.y, screenSize.y);
    output.ScreenCoords.xy = input.Pos.xy * resizedDims.xy;
    output.ScreenCoords.zw = resizedDims.xy * float2(0.5, -0.5);

    float2 tmp2 = resizedDims.zw * resizedDims.zw;
    output.SmoothStepLimit = tmp2.x * tmp2.y * 0.03;

    return output;
}

PS_INPUT VS_Quad( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;
     
    output.Pos = input.Pos;
    output.Tex = input.Tex;
   
    return output;
}

DX12PS_INPUT DX12VS(VS_INPUT input)
{
	DX12PS_INPUT output = (DX12PS_INPUT)0;

	int width = cbDX12XFormAndAlpha_WH & 0x0000ffff;
	int height = cbDX12XFormAndAlpha_WH >> 16;
	float4 xForm0 = float4(float(width), 0.0, 0.0, 0.0);
	float4 xForm1 = float4(0.0, float(height), 0.0, 0.0);
	float4 xForm2 = float4(0.0, 0.0, 1.0, 0.0);
	float4 xForm3 = float4(cbDX12XFormAndAlpha_X, cbDX12XFormAndAlpha_Y, 0.0, 1.0);

	float4x4 dx12XForm = float4x4(xForm0, xForm1, xForm2, xForm3);
	float4 tmp = mul(input.Pos, dx12XForm);

	float4 proj0 = float4(cbDX12ProjMatrix_11, 0.0, 0.0, 0.0);
	float4 proj1 = float4(0.0, cbDX12ProjMatrix_22, 0.0, 0.0);
	float4 proj2 = float4(0.0, 0.0, cbDX12ProjMatrix_33, 0.0);
	float4 proj3 = float4(0.0, 0.0, cbDX12ProjMatrix_34, 1.0);
	float4x4 dx12ProjMatrix = float4x4(proj0, proj1, proj2, proj3);

	output.Pos = mul(tmp, dx12ProjMatrix);
	output.Tex.xy = input.Tex;
	output.Tex.z  = cbDX12XFormAndAlpha_alpha;

	return output;
}

PS_INPUT_BACKGROUND DX12VS_Background(VS_INPUT input)
{
	PS_INPUT_BACKGROUND output = (PS_INPUT_BACKGROUND)0;

	int width = cbDX12XFormAndAlpha_WH & 0x0000ffff;
	int height = cbDX12XFormAndAlpha_WH >> 16;
	float4 xForm0 = float4(float(width), 0.0, 0.0, 0.0);
	float4 xForm1 = float4(0.0, float(height), 0.0, 0.0);
	float4 xForm2 = float4(0.0, 0.0, 1.0, 0.0);
	float4 xForm3 = float4(cbDX12XFormAndAlpha_X, cbDX12XFormAndAlpha_Y, 0.0, 1.0);

	float4x4 dx12XForm = float4x4(xForm0, xForm1, xForm2, xForm3);
	float4 tmp = mul(input.Pos, dx12XForm);

	float4 proj0 = float4(cbDX12ProjMatrix_11, 0.0, 0.0, 0.0);
	float4 proj1 = float4(0.0, cbDX12ProjMatrix_22, 0.0, 0.0);
	float4 proj2 = float4(0.0, 0.0, cbDX12ProjMatrix_33, 0.0);
	float4 proj3 = float4(0.0, 0.0, cbDX12ProjMatrix_34, 1.0);
	float4x4 dx12ProjMatrix = float4x4(proj0, proj1, proj2, proj3);

	output.Pos = mul(tmp, dx12ProjMatrix);
  
    float2 screenSize = float2(width, height);
    float screenAnimStretch = cbDX12StretchFactor + 0.8;
    float4 resizedDims = float4(/* screenSize.x * screenRatio */ screenSize.y * screenAnimStretch, -screenSize.y * screenAnimStretch, screenSize.y, screenSize.y);
    output.ScreenCoords.xy = input.Pos.xy * resizedDims.xy;
    output.ScreenCoords.zw = resizedDims.xy * float2(0.5, -0.5);

    float2 tmp2 = resizedDims.zw * resizedDims.zw;
    output.SmoothStepLimit = tmp2.x * tmp2.y * 0.03;

    return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(PS_INPUT input) : SV_Target
{
   float4 result = Texture.Sample(samPoint, input.Tex);
   result.a *= TransparencyFactor;

   // we swap red and blue
   return float4(result.b, result.g, result.r, result.a);
   // return float4(1, 0.5, 0, 1); // this is only for testing rendering without a working texture upload
}

float4 PS_Background(PS_INPUT_BACKGROUND input) : SV_Target
{
    float2 diff = abs(input.ScreenCoords.xy - input.ScreenCoords.zw);
    float2 diff2 = diff * diff;
    float2 diff4 = diff2 * diff2;

    float radius2 = diff4.x + diff4.y;
    float alpha = 0.68 + smoothstep(0.0, input.SmoothStepLimit, radius2) * 3.2 * StretchFactor;
    alpha *= TransparencyFactor;

    return float4(0.0, 0.0, 0.0, alpha);
}

float4 PS_Quad(PS_INPUT input) : SV_Target
{
   float3 result = Texture.Sample(samLinear, input.Tex).rgb;
   // we swap red and blue
   return float4(result.b, result.g, result.r, 1.0f);
   // return float4(1, 0.5, 0, 1); // this is only for testing rendering without a working texture upload
}

// use a single YUV444 conversion
float4 PS_Quad_YUV(PS_INPUT input) : SV_Target
{
   float4 result = Texture.Sample(samLinear, input.Tex);
   
   // return luminance (Y)
   const float4x4 yuv = {0.257, -0.148,  0.439, 0, 
						 0.504, -0.291, -0.368, 0, 
						 0.098,  0.439, -0.071, 0,
						 0.0625, 0.5,    0.5,   1.0  };

	float4 outpix = mul(float4(result.rgb,1.0),yuv);
    return float4( outpix.x, outpix.y, 0, outpix.z);
}

float4 DX12PS(DX12PS_INPUT input) : SV_Target
{
	float4 color = float4(1.0, 1.0, 1.0, input.Tex.z);
	float4 result = Texture.Sample(samPoint, input.Tex.xy) * color;

	// we swap red and blue
	return float4(result.b, result.g, result.r, result.a);
	// return float4(1, 0.5, 0, 1); // this is only for testing rendering without a working texture upload
}

float4 DX12PS_Background(PS_INPUT_BACKGROUND input) : SV_Target
{
    float2 diff = abs(input.ScreenCoords.xy - input.ScreenCoords.zw);
    float2 diff2 = diff * diff;
    float2 diff4 = diff2 * diff2;

    float radius2 = diff4.x + diff4.y;
    float alpha = 0.68 + smoothstep(0.0, input.SmoothStepLimit, radius2) * 3.2 * cbDX12StretchFactor;
    alpha *= cbDX12XFormAndAlpha_alpha;

    return float4(0.0, 0.0, 0.0, alpha);
}

//--------------------------------------------------------------------------------------
// Technique
//--------------------------------------------------------------------------------------
technique10 Render
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS() ) );
		SetDepthStencilState(DisableDepth, 0);
		SetBlendState(BlendAlpha, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetRasterizerState(SetCWCulling);
    }
    pass P1
    {
        SetVertexShader(CompileShader(vs_4_0, VS_Background()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, PS_Background()));
        SetDepthStencilState(DisableDepth, 0);
        SetBlendState(BlendAlpha, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
        SetRasterizerState(SetCWCulling);
    }
}

technique10 RenderQuad
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, VS_Quad() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS_Quad() ) );
		SetDepthStencilState(DisableDepth, 0);
		SetBlendState(BlendNoAlpha, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetRasterizerState(SetCWCulling);
	}
}


technique10 RenderQuad_YUV
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, VS_Quad() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS_Quad_YUV() ) );
		SetDepthStencilState(DisableDepth, 0);
		SetBlendState(BlendNoAlpha, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetRasterizerState(SetCWCulling);
    }
}
