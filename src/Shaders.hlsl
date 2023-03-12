// These are some generic shaders that we need to render the example triangle
// Make sure that this file is excluded from the build to avoid compile time errors!
R""(

Texture2D tex;
SamplerState sampleType;

cbuffer constantBuffer
{
    matrix wvp;
};

struct VS_Input
{
    float4 pos : POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

struct VS_Output
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

VS_Output VS(VS_Input input)
{
    VS_Output vsout;
    vsout.pos = mul(input.pos, wvp);
    vsout.color = input.color;
    vsout.texcoord = input.texcoord;
    return vsout;
}

float4 PSTex(VS_Output input) : SV_Target
{
    float4 textureColor;
    textureColor = tex.Sample(sampleType, input.texcoord);
    return textureColor;
}

float4 PS(VS_Output input) : SV_Target
{
    return input.color;
}

)""