#include "TexturedBox.h"

using namespace DirectX;

TexturedBox::TexturedBox(float posX, float posY, float width, float height, int textureIndex)
{
	this->pos = XMFLOAT3(posX, posY, 1.0f);
	this->width = width;
	this->height = height;
	this->textureIndex = textureIndex;
	this->meshClassName = typeid(this).name(); // Lets us retrieve the name of this class in a string

	// Texture coords must be included but if textureIndex is -1 (default value) then
	// the mesh will instead be rendered using the supplied color
	vertices =
	{
		{ XMFLOAT3(posX - (width / 2), posY - (height / 2), 0.0f), XMFLOAT4(0.5f, 0.0f, 0.5f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(posX - (width / 2), posY + (height / 2), 0.0f), XMFLOAT4(0.5f, 0.0f, 0.5f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(posX + (width / 2), posY + (height / 2), 0.0f), XMFLOAT4(0.5f, 0.0f, 0.5f, 1.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(posX + (width / 2), posY - (height / 2), 0.0f), XMFLOAT4(0.5f, 0.0f, 0.5f, 1.0f), XMFLOAT2(1.0f, 1.0f) }
	};

	ZeroMemory(&vertexDesc, sizeof(vertexDesc));
	vertexDesc.ByteWidth = sizeof(Vertex) * vertices.size();
	vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexDesc.StructureByteStride = sizeof(Vertex);

	vertexSubData = { vertices.data(), 0, 0 };

	indices = 
	{
		0, 1, 2,
		0, 2, 3
	};

	ZeroMemory(&indexDesc, sizeof(indexDesc));
	indexDesc.ByteWidth = sizeof(unsigned int) * indices.size();
	indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexDesc.Usage = D3D11_USAGE_DEFAULT;
	indexDesc.CPUAccessFlags = 0;
	indexDesc.MiscFlags = 0;

	indexSubData = { indices.data(), 0, 0 };
}
