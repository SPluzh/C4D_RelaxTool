#include "relax_engine.h"
#include "c4d_baseobject.h"
#include "c4d_basedraw.h"
#include "c4d_baseselect.h"
#include "maxon/basearray.h"
#include <cmath>
#include <algorithm>

namespace cinema
{

static Float DistToSegment2D(Float px, Float py, Float ax, Float ay, Float bx, Float by)
{
    Float abx = bx - ax;
    Float aby = by - ay;
    Float lenSq = abx * abx + aby * aby;
    if (lenSq < 1e-6)
    {
        Float dx = px - ax;
        Float dy = py - ay;
        return std::sqrt(dx * dx + dy * dy);
    }
    Float apx = px - ax;
    Float apy = py - ay;
    Float t = (apx * abx + apy * aby) / lenSq;
    t = std::max(0.0, std::min(1.0, t));
    Float qx = ax + t * abx;
    Float qy = ay + t * aby;
    Float dx = px - qx;
    Float dy = py - qy;
    return std::sqrt(dx * dx + dy * dy);
}

Bool RelaxEngine::IsCursorNearBorder(PolygonObject* mesh, BaseDraw* bd, Float screenX, Float screenY, Float brushRadius)
{
    if (!mesh || !bd)
        return false;

    Int32 ptCount = mesh->GetPointCount();
    Int32 polyCount = mesh->GetPolygonCount();
    if (ptCount < 3 || polyCount == 0)
        return false;

    const Vector* pts = mesh->GetPointR();
    const CPolygon* polys = mesh->GetPolygonR();
    Matrix mg = mesh->GetMg();

    struct EdgeEntry
    {
        Int32 u;
        Int32 v;
        Int32 count;
    };
    maxon::BaseArray<EdgeEntry> edges;

    auto addOrIncEdge = [&](Int32 a, Int32 b) {
        Int32 u = std::min(a, b);
        Int32 v = std::max(a, b);
        for (Int32 k = 0; k < (Int32)edges.GetCount(); ++k)
        {
            if (edges[k].u == u && edges[k].v == v)
            {
                edges[k].count++;
                return;
            }
        }
        EdgeEntry ee;
        ee.u = u;
        ee.v = v;
        ee.count = 1;
        edges.Append(ee) iferr_ignore("Append edge");
    };

    for (Int32 p = 0; p < polyCount; ++p)
    {
        const CPolygon& poly = polys[p];
        addOrIncEdge(poly.a, poly.b);
        addOrIncEdge(poly.b, poly.c);
        if (poly.c != poly.d)
        {
            addOrIncEdge(poly.c, poly.d);
            addOrIncEdge(poly.d, poly.a);
        }
        else
        {
            addOrIncEdge(poly.c, poly.a);
        }
    }

    Float minBorderDist = 1e30;
    Float minInteriorDist = 1e30;

    for (Int32 k = 0; k < (Int32)edges.GetCount(); ++k)
    {
        const EdgeEntry& ee = edges[k];
        if (ee.u < 0 || ee.u >= ptCount || ee.v < 0 || ee.v >= ptCount) continue;

        Vector sA = bd->WS(mg * pts[ee.u]);
        Vector sB = bd->WS(mg * pts[ee.v]);
        if (sA.z <= 0.0 || sB.z <= 0.0) continue;

        Float d = DistToSegment2D(screenX, screenY, sA.x, sA.y, sB.x, sB.y);
        if (ee.count == 1)
        {
            if (d < minBorderDist) minBorderDist = d;
        }
        else
        {
            if (d < minInteriorDist) minInteriorDist = d;
        }
    }

    if (minBorderDist > brushRadius && minInteriorDist > brushRadius)
        return false;

    if (minInteriorDist >= 1e29)
        return (minBorderDist <= brushRadius);

    if (minBorderDist <= 10.0)
        return true;

    return (minBorderDist <= minInteriorDist);
}

Bool RelaxEngine::RelaxVertices(
    PolygonObject* mesh,
    BaseDraw* bd,
    Float screenX, Float screenY,
    Float brushRadius,
    Float strength,
    Bool lockBorder,
    Bool lockInterior,
    Int32 iterations,
    Bool useSelection)
{
    if (!mesh || !bd || brushRadius <= 0.0 || strength <= 0.0)
        return false;

    Int32 ptCount = mesh->GetPointCount();
    Int32 polyCount = mesh->GetPolygonCount();
    if (ptCount < 3 || polyCount == 0)
        return false;

    const Vector* pts = mesh->GetPointR();
    const CPolygon* polys = mesh->GetPolygonR();
    Matrix meshMg = mesh->GetMg();

    const BaseSelect* pointSel = mesh->GetPointS();
    if (useSelection && pointSel && pointSel->GetCount() == 0)
        return false;

    // 1. Find all vertices within brush radius in screen space
    maxon::BaseArray<Int32> affectedVertices;
    maxon::BaseArray<Float> vertexWeights;

    for (Int32 i = 0; i < ptCount; ++i)
    {
        if (useSelection && pointSel && !pointSel->IsSelected(i))
            continue;

        Vector worldPos = meshMg * pts[i];
        Vector screenPos = bd->WS(worldPos);
        if (screenPos.z <= 0.0) continue; // Behind camera

        Float dx = screenPos.x - screenX;
        Float dy = screenPos.y - screenY;
        Float dist = std::sqrt(dx * dx + dy * dy);
        if (dist <= brushRadius)
        {
            Float t = 1.0 - (dist / brushRadius);
            Float falloff = t * t * (3.0 - 2.0 * t); // Smoothstep falloff
            affectedVertices.Append(i) iferr_ignore("Append affected vertex");
            vertexWeights.Append(falloff * strength) iferr_ignore("Append weight");
        }
    }

    if (affectedVertices.GetCount() == 0)
        return false;

    // 2. Build vertex adjacency (all neighbors and boundary neighbors)
    struct EdgeEntry
    {
        Int32 u;
        Int32 v;
        Int32 count;
    };
    maxon::BaseArray<EdgeEntry> edges;

    auto addOrIncEdge = [&](Int32 a, Int32 b) {
        Int32 u = std::min(a, b);
        Int32 v = std::max(a, b);
        for (Int32 k = 0; k < (Int32)edges.GetCount(); ++k)
        {
            if (edges[k].u == u && edges[k].v == v)
            {
                edges[k].count++;
                return;
            }
        }
        EdgeEntry ee;
        ee.u = u;
        ee.v = v;
        ee.count = 1;
        edges.Append(ee) iferr_ignore("Append edge");
    };

    for (Int32 p = 0; p < polyCount; ++p)
    {
        const CPolygon& poly = polys[p];
        addOrIncEdge(poly.a, poly.b);
        addOrIncEdge(poly.b, poly.c);
        if (poly.c != poly.d)
        {
            addOrIncEdge(poly.c, poly.d);
            addOrIncEdge(poly.d, poly.a);
        }
        else
        {
            addOrIncEdge(poly.c, poly.a);
        }
    }

    maxon::BaseArray<maxon::BaseArray<Int32>> allNeighbors;
    maxon::BaseArray<maxon::BaseArray<Int32>> boundaryNeighbors;
    allNeighbors.Resize(ptCount) iferr_ignore("Resize allNeighbors");
    boundaryNeighbors.Resize(ptCount) iferr_ignore("Resize boundaryNeighbors");

    for (Int32 k = 0; k < (Int32)edges.GetCount(); ++k)
    {
        const EdgeEntry& ee = edges[k];
        if (ee.u >= 0 && ee.u < ptCount && ee.v >= 0 && ee.v < ptCount)
        {
            allNeighbors[ee.u].Append(ee.v) iferr_ignore("Append neighbor");
            allNeighbors[ee.v].Append(ee.u) iferr_ignore("Append neighbor");

            if (ee.count == 1) // Boundary edge
            {
                boundaryNeighbors[ee.u].Append(ee.v) iferr_ignore("Append boundary neighbor");
                boundaryNeighbors[ee.v].Append(ee.u) iferr_ignore("Append boundary neighbor");
            }
        }
    }

    // 3. Multi-pass Laplacian smoothing
    Vector* ptsW = mesh->GetPointW();
    if (!ptsW)
        return false;

    Int32 clampedIters = maxon::ClampValue(iterations, 1, 20);

    maxon::BaseArray<Vector> nextPositions;
    nextPositions.Resize(affectedVertices.GetCount()) iferr_ignore("Resize nextPositions");

    for (Int32 iter = 0; iter < clampedIters; ++iter)
    {
        for (Int32 ai = 0; ai < (Int32)affectedVertices.GetCount(); ++ai)
        {
            Int32 idx = affectedVertices[ai];
            Float w = vertexWeights[ai];
            if (w <= 0.001)
            {
                nextPositions[ai] = ptsW[idx];
                continue;
            }

            const maxon::BaseArray<Int32>& bNeighbors = boundaryNeighbors[idx];
            const maxon::BaseArray<Int32>& nNeighbors = allNeighbors[idx];

            Bool isBoundary = (bNeighbors.GetCount() > 0);

            if (lockBorder && isBoundary)
            {
                nextPositions[ai] = ptsW[idx];
                continue;
            }

            if (lockInterior && !isBoundary)
            {
                nextPositions[ai] = ptsW[idx];
                continue;
            }

            Vector targetLocalPos = ptsW[idx];

            if (isBoundary)
            {
                if (bNeighbors.GetCount() == 2)
                {
                    // Boundary vertex: smooth along boundary contour
                    targetLocalPos = (ptsW[bNeighbors[0]] + ptsW[bNeighbors[1]]) * 0.5;
                }
                else
                {
                    // Corner or non-manifold on boundary: keep fixed to preserve silhouette
                    nextPositions[ai] = ptsW[idx];
                    continue;
                }
            }
            else
            {
                if (nNeighbors.GetCount() > 0)
                {
                    // Interior vertex: smooth towards centroid of all topological neighbors
                    Vector sum(0.0);
                    for (Int32 ni = 0; ni < (Int32)nNeighbors.GetCount(); ++ni)
                    {
                        sum += ptsW[nNeighbors[ni]];
                    }
                    targetLocalPos = sum / (Float)nNeighbors.GetCount();
                }
                else
                {
                    nextPositions[ai] = ptsW[idx];
                    continue;
                }
            }

            nextPositions[ai] = ptsW[idx] + (targetLocalPos - ptsW[idx]) * w;
        }

        // Apply iteration results to ptsW
        for (Int32 ai = 0; ai < (Int32)affectedVertices.GetCount(); ++ai)
        {
            Int32 idx = affectedVertices[ai];
            ptsW[idx] = nextPositions[ai];
        }
    }

    NotifyMeshUpdated(mesh);
    return true;
}

void RelaxEngine::NotifyMeshUpdated(PolygonObject* mesh)
{
    if (!mesh) return;
    mesh->Message(MSG_UPDATE);
}

} // namespace cinema
