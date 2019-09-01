#pragma once
#include "Vertex.h"
#include <vector>
#include <d3d11.h>
#include <typeinfo>
#include <wrl/client.h>

// A base class used for making custom meshes
class Mesh
{
protected:
	DirectX::XMFLOAT3 pos;

	std::vector<Vertex> vertices;
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	D3D11_BUFFER_DESC vertexDesc = { 0 };
	D3D11_SUBRESOURCE_DATA vertexSubData = { 0 };

	std::vector<unsigned int> indices;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	D3D11_BUFFER_DESC indexDesc = { 0 };
	D3D11_SUBRESOURCE_DATA indexSubData = { 0 };

	float size; // An arbitrary size value for the mesh, use this one for circles etc.
	float width; // Use these for rectangles etc.
	float height;
	int textureIndex = -1; // Index for a texture in the texture list
	std::string meshClassName = "Undefined name";

public:
	DirectX::XMFLOAT3 GetPos();

	ID3D11Buffer** GetVertexBuffer();
	D3D11_BUFFER_DESC GetVertexDesc();
	D3D11_SUBRESOURCE_DATA GetVertexSubData();

	ID3D11Buffer** GetIndexBuffer();
	D3D11_BUFFER_DESC GetIndexDesc();
	D3D11_SUBRESOURCE_DATA GetIndexSubData();

	int GetNumIndices();
	float GetSize();
	int GetTextureIndex();
	std::string GetMeshClassName();
};