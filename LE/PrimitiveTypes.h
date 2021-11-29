#pragma once
#include <vector>
#include <DirectXMath.h>

class PrimitiveTypes
{
public:
	struct PosVertex
	{
		PosVertex()
			:
			Position(0.0f, 0.0f, 0.0f)
		{}
		PosVertex(float x, float y, float z)
			:
			Position(x, y, z)
		{}
		DirectX::XMFLOAT3 Position;
	};

	struct PosTexVertex
	{
		PosTexVertex()
			:
			Position(0.0f, 0.0f, 0.0f),
			TexCoord(0.0f, 0.0f)
		{}
		PosTexVertex(float x, float y, float z, float u, float v)
			:
			Position(x, y, z),
			TexCoord(u, v)
		{}
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT2 TexCoord;
	};

	struct PosColVertex
	{
		PosColVertex()
			:
			Position(0.0f, 0.0f, 0.0f),
			Color(0.0f, 0.0f, 0.0f, 1.0f)
		{}
		PosColVertex(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT4& col)
			:
			Position(pos),
			Color(col)
		{}
		PosColVertex(float x, float y, float z, float r, float g, float b, float a)
			:
			Position(x, y, z),
			Color(r, g, b, a)
		{}
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT4 Color;
	};

	struct PosTexColVertex
	{
		PosTexColVertex()
			:
			Position(0.0f, 0.0f, 0.0f),
			TexCoord(0.0f, 0.0f),
			Color(0.0f, 0.0f, 0.0f, 0.0f)
		{}
		PosTexColVertex(float x, float y, float z, float u, float v,
			float r, float g, float b, float a)
			:
			Position(x, y, z),
			TexCoord(u, v),
			Color(r, g, b, a)
		{}
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT2 TexCoord;
		DirectX::XMFLOAT4 Color;
	};

	struct PosTexNorVertex
	{
		PosTexNorVertex() {}
		PosTexNorVertex(float x, float y, float z,
			float u, float v,
			float nx, float ny, float nz)
			:
			Position(x, y, z),
			TexCoord(u, v),
			Normal(nx, ny, nz)
		{}
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT2 TexCoord;
		DirectX::XMFLOAT3 Normal;
	};

	struct PosNorColVertex
	{
		PosNorColVertex() {}
		PosNorColVertex(float x, float y, float z,
			float nx, float ny, float nz,
			float r, float g, float b, float a)
			:
			Position(x, y, z),
			Normal(nx, ny, nz),
			Color(r, g, b, a)
		{}
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT3 Normal;
		DirectX::XMFLOAT4 Color;
	};

	struct PosTexNorColVertex
	{
		PosTexNorColVertex() {}
		PosTexNorColVertex(float x, float y, float z,
			float u, float v,
			float nx, float ny, float nz,
			float r, float g, float b, float a)
			:
			Position(x, y, z),
			TexCoord(u, v),
			Normal(nx, ny, nz),
			Color(r, g, b, a)
		{}
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT2 TexCoord;
		DirectX::XMFLOAT3 Normal;
		DirectX::XMFLOAT4 Color;
	};

	struct PosNorTanTexColVertex
	{
		PosNorTanTexColVertex()
		{}

		PosNorTanTexColVertex(
			float px, float py, float pz,
			float nx, float ny, float nz,
			float tx, float ty, float tz,
			float u, float v,
			float r, float g, float b, float a)
			:
			Position(px, py, pz),
			Normal(nx, ny, nz),
			TangentU(tx, ty, tz),
			TexCoord(u, v),
			Color(r, g, b, a)
		{}

		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT3 Normal;
		DirectX::XMFLOAT3 TangentU;
		DirectX::XMFLOAT2 TexCoord;
		DirectX::XMFLOAT4 Color;
	};

	struct Vertex
	{
		Vertex()
		{}

		Vertex(const DirectX::XMFLOAT3& p,
			const DirectX::XMFLOAT3& n,
			const DirectX::XMFLOAT3& t,
			const DirectX::XMFLOAT2& uv,
			const DirectX::XMFLOAT4& col)
			:
			Position(p),
			Normal(n),
			TangentU(t),
			TexCoord(uv),
			Color(col)
		{}

		Vertex(
			float px, float py, float pz,
			float nx, float ny, float nz,
			float tx, float ty, float tz,
			float u, float v,
			float r, float g, float b, float a)
			:
			Position(px, py, pz),
			Normal(nx, ny, nz),
			TangentU(tx, ty, tz),
			TexCoord(u, v),
			Color(r, g, b, a)
		{}

		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT3 Normal;
		DirectX::XMFLOAT3 TangentU;
		DirectX::XMFLOAT2 TexCoord;
		DirectX::XMFLOAT4 Color;
	};

	struct MeshData
	{
		std::vector<Vertex> Vertices;
		std::vector<unsigned> Indices;
		// 最大最小点不一定在几何体面上，而是取x\y\z的最大最小值组合而成
		DirectX::XMFLOAT3 Min;
		DirectX::XMFLOAT3 Max;
	};
};