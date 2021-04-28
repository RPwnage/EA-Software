//--------------------------------------------------------------------------------------
// IGO FX DX9 Renderer
//--------------------------------------------------------------------------------------
float4x4 ViewProj : VIEWPROJECTION;
float4x4 World    : WORLD;

#define StretchFactor Color.x
#define TransparencyFactor Color.a
float4   Color;


Texture2D WindowTexture;
sampler TextureSampler = sampler_state
{
    Texture = <WindowTexture>;
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
    AddressU = Clamp;
    AddressV = Clamp;
};

struct VS_INPUT
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
};

struct PS_INPUT_BACKGROUND
{
    float4 Pos             : POSITION;
    float4 ScreenCoords    : TEXCOORD0;
    float  SmoothStepLimit : TEXCOORD1;
};

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output;
    float4 tmp = mul(input.Pos, World);
    output.Pos = mul(tmp, ViewProj);
    output.Tex = input.Tex;

    return output;
}

PS_INPUT_BACKGROUND VS_Background(VS_INPUT input)
{
    PS_INPUT_BACKGROUND output;
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

float4 PS(PS_INPUT input) : COLOR
{
    float4 result = tex2D(TextureSampler, input.Tex);
    result.a *= TransparencyFactor;

    // we swap red and blue
    return float4(result.r, result.g, result.b, result.a);
}

float4 PS_Background(PS_INPUT_BACKGROUND input) : COLOR
{
    float2 diff = abs(input.ScreenCoords.xy - input.ScreenCoords.zw);
    float2 diff2 = diff * diff;
    float2 diff4 = diff2 * diff2;

    float radius2 = diff4.x + diff4.y;
    float alpha = 0.68 + smoothstep(0.0, input.SmoothStepLimit, radius2) * 3.2 * StretchFactor;
    alpha *= TransparencyFactor;

    return float4(0.0, 0.0, 0.0, alpha);
}

technique Render
{
    pass P0
    {
        vertexShader = compile vs_2_0 VS();
        pixelShader = compile ps_2_a PS();

        ZEnable = FALSE;
        ZWriteEnable = FALSE;

        FillMode = SOLID;
        CullMode = CCW;

        AlphaBlendEnable = TRUE;
        BlendOp = ADD;
        SrcBlendAlpha = INVDESTALPHA;
        DestBlendAlpha = ONE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;
    }

    pass P1
    {
        vertexShader = compile vs_2_0 VS_Background();
        pixelShader = compile ps_2_a PS_Background();

        ZEnable = FALSE;
        ZWriteEnable = FALSE;

        FillMode = SOLID;
        CullMode = CCW;

        AlphaBlendEnable = TRUE;
        BlendOp = ADD;
        SrcBlendAlpha = INVDESTALPHA;
        DestBlendAlpha = ONE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;
    }
}
