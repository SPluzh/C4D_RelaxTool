#ifndef RELAX_ENGINE_H__
#define RELAX_ENGINE_H__

#include "c4d.h"
#include "maxon/basearray.h"

namespace cinema
{

class RelaxEngine
{
public:
    RelaxEngine();
    ~RelaxEngine();

    RelaxEngine(const RelaxEngine&) = delete;
    RelaxEngine& operator=(const RelaxEngine&) = delete;

    // Call when starting a brush stroke to precompute topology, incident polygons,
    // boundaries, creases, and cache initial point positions for reprojection.
    Bool BeginStroke(
        PolygonObject* mesh,
        Int32 algorithm,
        Bool preserveCreases,
        Float creaseAngleDeg
    );

    // Call when dragging ends to release stroke cache.
    void EndStroke();

    // Relax vertices within screen-space brush radius.
    // Supports Laplacian, Tangential Slide, and Surface Reprojection modes,
    // along with boundary locking, hard edge/crease preservation, multi-pass iterations,
    // and selection filtering.
    Bool RelaxVertices(
        PolygonObject* mesh,
        BaseDraw* bd,
        Float screenX, Float screenY,
        Float brushRadius,
        Float strength,
        Bool lockBorder,
        Bool lockInterior,
        Int32 iterations = 1,
        Bool useSelection = false,
        Int32 algorithm = 1,
        Bool preserveCreases = true,
        Float creaseAngleDeg = 45.0
    );

    // Determines whether the cursor is closer to boundary edges or interior edges.
    Bool IsCursorNearBorder(
        PolygonObject* mesh,
        BaseDraw* bd,
        Float screenX, Float screenY,
        Float brushRadius
    );

private:
    void NotifyMeshUpdated(PolygonObject* mesh);

    Bool EnsureMeshTopology(
        PolygonObject* mesh,
        Bool preserveCreases,
        Float creaseAngleDeg
    );

    static Vector ClosestPointOnTriangle(
        const Vector& p,
        const Vector& a,
        const Vector& b,
        const Vector& c
    );

    // Stroke cache state
    Bool m_strokeActive = false;
    PolygonObject* m_cachedMesh = nullptr;
    Int32 m_cachedAlgorithm = 1;
    Bool m_cachedPreserveCreases = true;
    Float m_cachedCreaseAngleDeg = 45.0;

    // Topology structures
    maxon::BaseArray<Vector> m_initialPositions;
    maxon::BaseArray<maxon::BaseArray<Int32>> m_allNeighbors;
    maxon::BaseArray<maxon::BaseArray<Int32>> m_boundaryNeighbors;
    maxon::BaseArray<maxon::BaseArray<Int32>> m_creaseNeighbors;
    maxon::BaseArray<maxon::BaseArray<Int32>> m_incidentPolys;

    struct BoundaryEdgeEntry
    {
        Int32 u;
        Int32 v;
    };
    maxon::BaseArray<BoundaryEdgeEntry> m_boundaryEdges;
};

} // namespace cinema

#endif // RELAX_ENGINE_H__
