R""(

Texture2D tex;
SamplerState sampleType;

struct VertexShaderInput
{
    float4 pos : POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

struct PixelShaderInput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

PixelShaderInput VS(VertexShaderInput input)
{
    PixelShaderInput psi;
    psi.color = input.color;
    psi.pos = input.pos;
    psi.texcoord = input.texcoord;
    return psi;
}

float4 PSTex(PixelShaderInput input) : SV_Target
{
    float4 textureColor;
    textureColor = tex.Sample(sampleType, input.texcoord);
    return textureColor;
}

float4 PS(PixelShaderInput input) : SV_Target
{
    return input.color;
}

)""