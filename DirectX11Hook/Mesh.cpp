#include "Mesh.h"

using namespace DirectX;
using namespace Microsoft::WRL;

XMFLOAT3 Mesh::GetPos()
{
	return pos;
}

ID3D11Buffer** Mesh::GetVertexBuffer()
{
	return vertexBuffer.GetAddressOf();
}

D3D11_BUFFER_DESC Mesh::GetVertexDesc()
{
	return vertexDesc;
}

D3D11_SUBRESOURCE_DATA Mesh::GetVertexSubData()
{
	return vertexSubData;
}

ID3D11Buffer** Mesh::GetIndexBuffer()
{
	return indexBuffer.GetAddressOf();
}

D3D11_BUFFER_DESC Mesh::GetIndexDesc()
{
	return indexDesc;
}

D3D11_SUBRESOURCE_DATA Mesh::GetIndexSubData()
{
	return indexSubData;
}

int Mesh::GetNumIndices()
{
	return indices.size();
}

float Mesh::GetSize()
{
	return size;
}

int Mesh::GetTextureIndex()
{
	return textureIndex;
}

std::string Mesh::GetMeshClassName()
{
	return meshClassName;
}
