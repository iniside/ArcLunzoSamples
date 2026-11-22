// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "CQTest.h"
#include "Render/ArcRmlEdgeFringe.h"

namespace ArcRmlEdgeFringeTestsPrivate
{
	constexpr uint32 White = 0xFFFFFFFFu;

	FArcRmlVertex MakeVertex(float X, float Y, float U, float V)
	{
		FArcRmlVertex Vertex;
		Vertex.Position = FVector2f(X, Y);
		Vertex.Color = White;
		Vertex.TexCoord = FVector2f(U, V);
		return Vertex;
	}

	void AddQuad(TArray<FArcRmlVertex>& Vertices, TArray<uint32>& Indices, float Left, float Top, float Right, float Bottom, const FVector2f& UvMin = FVector2f(0.0f, 0.0f),
		const FVector2f& UvMax = FVector2f(1.0f, 1.0f))
	{
		const uint32 Base = static_cast<uint32>(Vertices.Num());
		Vertices.Add(MakeVertex(Left, Top, UvMin.X, UvMin.Y));
		Vertices.Add(MakeVertex(Right, Top, UvMax.X, UvMin.Y));
		Vertices.Add(MakeVertex(Right, Bottom, UvMax.X, UvMax.Y));
		Vertices.Add(MakeVertex(Left, Bottom, UvMin.X, UvMax.Y));
		Indices.Append({Base, Base + 1, Base + 2, Base, Base + 2, Base + 3});
	}

	FMatrix44f MakeRotation(float Degrees)
	{
		const float Radians = FMath::DegreesToRadians(Degrees);
		FMatrix44f Matrix = FMatrix44f::Identity;
		Matrix.M[0][0] = FMath::Cos(Radians);
		Matrix.M[0][1] = FMath::Sin(Radians);
		Matrix.M[1][0] = -FMath::Sin(Radians);
		Matrix.M[1][1] = FMath::Cos(Radians);
		return Matrix;
	}

	FVector2f MapTexCoord(const FArcRmlFringeVertex& Vertex, const FVector2f& Offset)
	{
		const FVector4f& PerPosition = Vertex.TexCoordPerPosition;
		return Vertex.TexCoord + FVector2f(PerPosition.X * Offset.X + PerPosition.Y * Offset.Y, PerPosition.Z * Offset.X + PerPosition.W * Offset.Y);
	}

	FString DescribeVertex(const FArcRmlFringeVertex& Vertex)
	{
		return FString::Printf(TEXT("vertex at (%.2f, %.2f) uv (%.3f, %.3f) side %.0f normals (%.2f, %.2f) (%.2f, %.2f) duv/dpos (%.3f, %.3f, %.3f, %.3f)"), Vertex.Position.X,
			Vertex.Position.Y, Vertex.TexCoord.X, Vertex.TexCoord.Y, Vertex.Side, Vertex.EdgeNormalA.X, Vertex.EdgeNormalA.Y, Vertex.EdgeNormalB.X, Vertex.EdgeNormalB.Y,
			Vertex.TexCoordPerPosition.X, Vertex.TexCoordPerPosition.Y, Vertex.TexCoordPerPosition.Z, Vertex.TexCoordPerPosition.W);
	}

	FMatrix44f MakePixelTransform(float A, float B, float C, float D, const FVector2f& Translation, const FVector2f& ViewSize)
	{
		FMatrix44f Matrix = FMatrix44f::Identity;
		Matrix.M[0][0] = A * 2.0f / ViewSize.X;
		Matrix.M[0][1] = -B * 2.0f / ViewSize.Y;
		Matrix.M[1][0] = C * 2.0f / ViewSize.X;
		Matrix.M[1][1] = -D * 2.0f / ViewSize.Y;
		Matrix.M[3][0] = Translation.X * 2.0f / ViewSize.X - 1.0f;
		Matrix.M[3][1] = 1.0f - Translation.Y * 2.0f / ViewSize.Y;
		return Matrix;
	}

	FVector2f ToPixels(const FMatrix44f& Transform, const FVector2f& Position, const FVector2f& ViewSize)
	{
		const FVector4f Clip = Transform.TransformFVector4(FVector4f(Position.X, Position.Y, 0.0f, 1.0f));
		return FVector2f((Clip.X / Clip.W * 0.5f + 0.5f) * ViewSize.X, (Clip.Y / Clip.W * -0.5f + 0.5f) * ViewSize.Y);
	}

	float SignedDistanceToLine(const FVector2f& Point, const FVector2f& LineStart, const FVector2f& LineEnd, const FVector2f& Inside)
	{
		const FVector2f Along = (LineEnd - LineStart).GetSafeNormal();
		const float Distance = FVector2f::CrossProduct(Along, Point - LineStart);
		return FVector2f::CrossProduct(Along, Inside - LineStart) > 0.0f ? -Distance : Distance;
	}
}

TEST_CLASS(ArcRmlEdgeFringeTests, "ArcRmlUI.Render.EdgeFringe")
{
	void ExpectTextureMapping(const FArcRmlFringeVertex& Vertex, const FVector2f& Origin, const FVector2f& UvOrigin, const FVector2f& UvPerPixel)
	{
		using namespace ArcRmlEdgeFringeTestsPrivate;
		const FString Context = DescribeVertex(Vertex);
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Vertex.TexCoordPerPosition.X, UvPerPixel.X, 1.0e-4f) && FMath::IsNearlyZero(Vertex.TexCoordPerPosition.Y, 1.0e-4f)
			&& FMath::IsNearlyZero(Vertex.TexCoordPerPosition.Z, 1.0e-4f) && FMath::IsNearlyEqual(Vertex.TexCoordPerPosition.W, UvPerPixel.Y, 1.0e-4f), Context));
		const FVector2f Offset = (Vertex.EdgeNormalA + Vertex.EdgeNormalB).GetSafeNormal() * (0.37f * Vertex.Side);
		const FVector2f Moved = Vertex.Position + Offset;
		const FVector2f Expected = UvOrigin + (Moved - Origin) * UvPerPixel;
		ASSERT_THAT(IsTrue(MapTexCoord(Vertex, Offset).Equals(Expected, 1.0e-4f),
			FString::Printf(TEXT("a vertex moved by the inset samples the texture where its new position lies: %s"), *Context)));
	}

	TEST_METHOD(SingleQuad_InsetsItsOutlineAndRampsAcrossEachSide)
	{
		using namespace ArcRmlEdgeFringeTestsPrivate;
		TArray<FArcRmlVertex> Vertices;
		TArray<uint32> Indices;
		AddQuad(Vertices, Indices, 0.0f, 0.0f, 10.0f, 4.0f);

		FArcRmlAntialiasedMesh Mesh;
		ArcRmlEdgeFringe::Build(Vertices, Indices, Mesh);

		ASSERT_THAT(AreEqual(4 + 16, Mesh.Vertices.Num(), TEXT("the quad's own vertices, then four per side")));
		ASSERT_THAT(AreEqual(6 + 24, Mesh.Indices.Num(), TEXT("the quad's two triangles, then two per side")));
		ASSERT_THAT(AreEqual(6u, Mesh.NumBaseIndices, TEXT("the inset quad comes first, so a clip mask can use it alone")));
		for (uint32 Index = 0; Index < Mesh.NumBaseIndices; ++Index)
		{
			ASSERT_THAT(IsTrue(Mesh.Indices[Index] < 4u, TEXT("the base part draws only the quad's own inset vertices")));
		}
		const FVector2f Centre(5.0f, 2.0f);
		for (int32 Index = 0; Index < Mesh.Vertices.Num(); ++Index)
		{
			const FArcRmlFringeVertex& Vertex = Mesh.Vertices[Index];
			const FString Context = DescribeVertex(Vertex);
			ASSERT_THAT(IsTrue(FMath::IsNearlyZero(FVector2f::DotProduct(Vertex.EdgeNormalA, Vertex.EdgeNormalB)),
				FString::Printf(TEXT("every vertex of a quad is a right-angle corner between two sides: %s"), *Context)));
			ASSERT_THAT(AreEqual(White, Vertex.Color, TEXT("every vertex keeps the colour; the side ramps the edge")));
			const float Outward = FVector2f::DotProduct(Vertex.EdgeNormalA + Vertex.EdgeNormalB, Vertex.Position - Centre);
			ASSERT_THAT(IsTrue(Outward > 0.0f, FString::Printf(TEXT("both side normals point out of the quad: %s"), *Context)));
			if (Vertex.Side > 0.0f)
			{
				ASSERT_THAT(IsTrue(Index >= 4, TEXT("only fringe vertices form the uncovered outer row")));
			}
			else
			{
				ASSERT_THAT(IsTrue(Vertex.Side == -1.0f, FString::Printf(TEXT("the quad and the inner row move into the quad: %s"), *Context)));
			}
		}
	}

	TEST_METHOD(RotatedImage_KeepsItsTextureMappingUnderTheInset)
	{
		using namespace ArcRmlEdgeFringeTestsPrivate;
		TArray<FArcRmlVertex> Vertices;
		TArray<uint32> Indices;
		AddQuad(Vertices, Indices, 20.0f, 10.0f, 60.0f, 30.0f);

		FArcRmlAntialiasedMesh Mesh;
		ArcRmlEdgeFringe::Build(Vertices, Indices, Mesh);

		ASSERT_THAT(AreEqual(20, Mesh.Vertices.Num()));
		for (const FArcRmlFringeVertex& Vertex : Mesh.Vertices)
		{
			ExpectTextureMapping(Vertex, FVector2f(20.0f, 10.0f), FVector2f(0.0f, 0.0f), FVector2f(1.0f / 40.0f, 1.0f / 20.0f));
		}
	}

	TEST_METHOD(RotatedText_EachGlyphKeepsItsOwnTextureMapping)
	{
		using namespace ArcRmlEdgeFringeTestsPrivate;
		TArray<FArcRmlVertex> Vertices;
		TArray<uint32> Indices;
		AddQuad(Vertices, Indices, 0.0f, 0.0f, 8.0f, 12.0f, FVector2f(0.25f, 0.5f), FVector2f(0.375f, 0.6875f));
		AddQuad(Vertices, Indices, 9.0f, 0.0f, 15.0f, 12.0f, FVector2f(0.5f, 0.0f), FVector2f(0.59375f, 0.1875f));

		FArcRmlAntialiasedMesh Mesh;
		ArcRmlEdgeFringe::Build(Vertices, Indices, Mesh);

		ASSERT_THAT(AreEqual(8 + 32, Mesh.Vertices.Num(), TEXT("two separate glyph quads, four sides each")));
		for (const FArcRmlFringeVertex& Vertex : Mesh.Vertices)
		{
			if (Vertex.Position.X <= 8.0f)
			{
				ExpectTextureMapping(Vertex, FVector2f(0.0f, 0.0f), FVector2f(0.25f, 0.5f), FVector2f(0.125f / 8.0f, 0.1875f / 12.0f));
			}
			else
			{
				ExpectTextureMapping(Vertex, FVector2f(9.0f, 0.0f), FVector2f(0.5f, 0.0f), FVector2f(0.09375f / 6.0f, 0.1875f / 12.0f));
			}
		}
	}

	TEST_METHOD(QuadsSharingAnEdge_GetNoFringeOnTheSharedEdge)
	{
		using namespace ArcRmlEdgeFringeTestsPrivate;
		TArray<FArcRmlVertex> Vertices;
		TArray<uint32> Indices;
		AddQuad(Vertices, Indices, 0.0f, 0.0f, 10.0f, 4.0f);
		AddQuad(Vertices, Indices, 10.0f, 0.0f, 20.0f, 4.0f);

		FArcRmlAntialiasedMesh Mesh;
		ArcRmlEdgeFringe::Build(Vertices, Indices, Mesh);

		ASSERT_THAT(AreEqual(8 + 24, Mesh.Vertices.Num(), TEXT("the outline of two adjacent quads has six sides; the shared side has duplicated vertices")));
		for (const FArcRmlFringeVertex& Vertex : Mesh.Vertices)
		{
			if (FMath::IsNearlyEqual(Vertex.Position.X, 10.0f))
			{
				ASSERT_THAT(IsTrue(FMath::IsNearlyZero(Vertex.EdgeNormalA.X) && FMath::IsNearlyZero(Vertex.EdgeNormalB.X),
					TEXT("vertices on the shared side move only across the outline, never into the other quad")));
			}
		}
	}

	TEST_METHOD(DegenerateTriangle_AddsNoFringe)
	{
		using namespace ArcRmlEdgeFringeTestsPrivate;
		TArray<FArcRmlVertex> Vertices = {MakeVertex(0.0f, 0.0f, 0.0f, 0.0f), MakeVertex(5.0f, 0.0f, 0.5f, 0.0f), MakeVertex(10.0f, 0.0f, 1.0f, 0.0f)};
		TArray<uint32> Indices = {0, 1, 2};

		FArcRmlAntialiasedMesh Mesh;
		ArcRmlEdgeFringe::Build(Vertices, Indices, Mesh);

		ASSERT_THAT(AreEqual(0, Mesh.Vertices.Num(), TEXT("a mesh without area has no antialiased variant")));
		ASSERT_THAT(AreEqual(0, Mesh.Indices.Num(), TEXT("a mesh without area has no antialiased variant")));
	}

	TEST_METHOD(OnlyRotatedOrSkewedTransforms_NeedTheFringe)
	{
		using namespace ArcRmlEdgeFringeTestsPrivate;
		FMatrix44f ScaleTranslate = FMatrix44f::Identity;
		ScaleTranslate.M[0][0] = 2.0f;
		ScaleTranslate.M[1][1] = -0.5f;
		ScaleTranslate.M[3][0] = 12.0f;
		ScaleTranslate.M[3][1] = -3.0f;
		ASSERT_THAT(IsTrue(ArcRmlEdgeFringe::IsAxisAligned(FMatrix44f::Identity)));
		ASSERT_THAT(IsTrue(ArcRmlEdgeFringe::IsAxisAligned(ScaleTranslate)));
		for (const float Degrees : {90.0f, 180.0f, 270.0f, -90.0f})
		{
			ASSERT_THAT(IsTrue(ArcRmlEdgeFringe::IsAxisAligned(MakeRotation(Degrees)), FString::Printf(TEXT("rotate(%.0fdeg) keeps edges on pixel rows"), Degrees)));
		}
		ASSERT_THAT(IsFalse(ArcRmlEdgeFringe::IsAxisAligned(MakeRotation(45.0f))));
		ASSERT_THAT(IsFalse(ArcRmlEdgeFringe::IsAxisAligned(MakeRotation(89.5f))));
		FMatrix44f Perspective = FMatrix44f::Identity;
		Perspective.M[0][3] = 0.001f;
		ASSERT_THAT(IsFalse(ArcRmlEdgeFringe::IsAxisAligned(Perspective)));
	}

	TEST_METHOD(AxisAlignedUse_BuildsNoVariantAndRotatedUseBuildsItOnce)
	{
		using namespace ArcRmlEdgeFringeTestsPrivate;
		TArray<FArcRmlVertex> Vertices;
		TArray<uint32> Indices;
		AddQuad(Vertices, Indices, 0.0f, 0.0f, 10.0f, 4.0f);

		FArcRmlAntialiasVariants Variants;
		Variants.Retain(7, Vertices, Indices);
		FArcRmlAntialiasedMesh Mesh;
		ASSERT_THAT(IsFalse(Variants.BuildOnFirstRotatedUse(7, FMatrix44f::Identity, Mesh), TEXT("an axis-aligned use builds no variant")));
		ASSERT_THAT(IsFalse(Variants.BuildOnFirstRotatedUse(7, MakeRotation(90.0f), Mesh), TEXT("a quarter turn builds no variant")));
		ASSERT_THAT(AreEqual(0, Mesh.Vertices.Num()));
#if !UE_BUILD_SHIPPING
		ASSERT_THAT(AreEqual(1, Variants.NumRetained(), TEXT("the mesh stays retained until a rotated use")));
#endif

		ASSERT_THAT(IsTrue(Variants.BuildOnFirstRotatedUse(7, MakeRotation(30.0f), Mesh), TEXT("the first rotated use builds the variant")));
		ASSERT_THAT(AreEqual(20, Mesh.Vertices.Num()));
#if !UE_BUILD_SHIPPING
		ASSERT_THAT(AreEqual(0, Variants.NumRetained(), TEXT("the retained mesh is dropped once its variant exists")));
#endif
		ASSERT_THAT(IsFalse(Variants.BuildOnFirstRotatedUse(7, MakeRotation(60.0f), Mesh), TEXT("the variant is built once")));

		Variants.Retain(8, Vertices, Indices);
		Variants.Release(8);
		ASSERT_THAT(IsFalse(Variants.BuildOnFirstRotatedUse(8, MakeRotation(30.0f), Mesh), TEXT("a released geometry builds no variant")));
	}

#if !UE_BUILD_SHIPPING
	TEST_METHOD(SkewedAndScaledQuad_RampsHalfAPixelAcrossEachProjectedSide)
	{
		using namespace ArcRmlEdgeFringeTestsPrivate;
		TArray<FArcRmlVertex> Vertices;
		TArray<uint32> Indices;
		AddQuad(Vertices, Indices, 0.0f, 0.0f, 40.0f, 20.0f);
		FArcRmlAntialiasedMesh Mesh;
		ArcRmlEdgeFringe::Build(Vertices, Indices, Mesh);

		const FVector2f ViewSize(800.0f, 600.0f);
		const FMatrix44f Transform = MakePixelTransform(1.5f, 0.2f, 0.5f, 0.75f, FVector2f(100.0f, 80.0f), ViewSize);
		const FVector2f Corners[4] = {FVector2f(0.0f, 0.0f), FVector2f(40.0f, 0.0f), FVector2f(40.0f, 20.0f), FVector2f(0.0f, 20.0f)};
		const FVector2f PixelCentre = ToPixels(Transform, FVector2f(20.0f, 10.0f), ViewSize);
		for (const FArcRmlFringeVertex& Vertex : Mesh.Vertices)
		{
			const FString Context = DescribeVertex(Vertex);
			ASSERT_THAT(IsTrue(Vertex.Side != 0.0f, FString::Printf(TEXT("every vertex of a quad lies on its outline: %s"), *Context)));
			const FVector2f Moved = ToPixels(Transform, Vertex.Position + ArcRmlEdgeFringe::GetInsetOffset(Vertex, Transform, FVector2f::ZeroVector, ViewSize), ViewSize);
			int32 SidesThrough = 0;
			for (int32 Side = 0; Side < 4; ++Side)
			{
				const FVector2f From = Corners[Side];
				const FVector2f To = Corners[(Side + 1) % 4];
				if (!FMath::IsNearlyZero(FVector2f::CrossProduct((To - From).GetSafeNormal(), Vertex.Position - From), 1.0e-3f))
				{
					continue;
				}
				++SidesThrough;
				const float Distance = SignedDistanceToLine(Moved, ToPixels(Transform, From, ViewSize), ToPixels(Transform, To, ViewSize), PixelCentre);
				ASSERT_THAT(IsNear(0.5f * Vertex.Side, Distance, 1.0e-3f,
					FString::Printf(TEXT("the vertex lies half a pixel outside (side 1) or inside (side -1) the projected side through (%.0f, %.0f): %s"), From.X, From.Y,
						*Context)));
			}
			ASSERT_THAT(AreEqual(2, SidesThrough, TEXT("every vertex of a quad is a corner of two sides")));
		}
	}
#endif
};
