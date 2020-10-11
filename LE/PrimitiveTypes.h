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
		static const bool hasPos = true;
		static const bool hasTex = false;
		static const bool hasNor = false;
		static const bool hasCol = false;
		static const bool hasTan = false;

		static const unsigned PosStartByte = 0;
		static const unsigned TexStartByte = 0;
		static const unsigned NorStartByte = 0;
		static const unsigned ColStartByte = 0;
		static const unsigned TanStartByte = 0;
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
		static const bool hasPos = true;
		static const bool hasTex = true;
		static const bool hasNor = false;
		static const bool hasCol = false;
		static const bool hasTan = false;

		static const unsigned PosStartByte = 0;
		static const unsigned TexStartByte = sizeof(DirectX::XMFLOAT3);
		static const unsigned NorStartByte = 0;
		static const unsigned ColStartByte = 0;
		static const unsigned TanStartByte = 0;
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
		static const bool hasPos = true;
		static const bool hasTex = false;
		static const bool hasNor = false;
		static const bool hasCol = true;
		static const bool hasTan = false;

		static const unsigned PosStartByte = 0;
		static const unsigned TexStartByte = 0;
		static const unsigned NorStartByte = 0;
		static const unsigned ColStartByte = sizeof(DirectX::XMFLOAT3);
		static const unsigned TanStartByte = 0;
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
		static const bool hasPos = true;
		static const bool hasTex = true;
		static const bool hasNor = false;
		static const bool hasCol = true;
		static const bool hasTan = false;

		static const unsigned PosStartByte = 0;
		static const unsigned TexStartByte = sizeof(DirectX::XMFLOAT3);
		static const unsigned NorStartByte = 0;
		static const unsigned ColStartByte = sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT2);
		static const unsigned TanStartByte = 0;
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
		static const bool hasPos = true;
		static const bool hasTex = true;
		static const bool hasNor = true;
		static const bool hasCol = false;
		static const bool hasTan = false;

		static const unsigned PosStartByte = 0;
		static const unsigned TexStartByte = sizeof(DirectX::XMFLOAT3);
		static const unsigned NorStartByte = sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT2);
		static const unsigned ColStartByte = 0;
		static const unsigned TanStartByte = 0;
	};

	struct PosNorColVertex
	{
		PosNorColVertex() {}
		PosNorColVertex(float x, float y, float z,
			float nx, float ny, float nz,
			float r, float g, float b, float a)
			:
			Position(x, y, z),
			normal(nx, ny, nz),
			color(r, g, b, a)
		{}
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT4 color;
		static const bool hasPos = true;
		static const bool hasTex = false;
		static const bool hasNor = true;
		static const bool hasCol = true;
		static const bool hasTan = false;

		static const unsigned PosStartByte = 0;
		static const unsigned TexStartByte = 0;
		static const unsigned NorStartByte = sizeof(DirectX::XMFLOAT3);
		static const unsigned ColStartByte = sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT3);
		static const unsigned TanStartByte = 0;
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
		static const bool hasPos = true;
		static const bool hasTex = true;
		static const bool hasNor = true;
		static const bool hasCol = true;
		static const bool hasTan = false;

		static const unsigned PosStartByte = 0;
		static const unsigned TexStartByte = sizeof(DirectX::XMFLOAT3);
		static const unsigned NorStartByte = sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT2);
		static const unsigned ColStartByte = sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT2) + sizeof(DirectX::XMFLOAT3);
		static const unsigned TanStartByte = 0;
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

		static const bool hasPos = true;
		static const bool hasTex = true;
		static const bool hasNor = true;
		static const bool hasCol = true;
		static const bool hasTan = true;

		static const unsigned PosStartByte = 0;
		static const unsigned NorStartByte = sizeof(DirectX::XMFLOAT3);
		static const unsigned TanStartByte = sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT3);
		static const unsigned TexStartByte = sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT3);
		static const unsigned ColStartByte = sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT2);
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