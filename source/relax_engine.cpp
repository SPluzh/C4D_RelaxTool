#include "relax_engine.h"
#include "description/toolrelaxtool.h"
#include "c4d_baseobject.h"
#include "c4d_basedraw.h"
#include "c4d_baseselect.h"
#include <cmath>
#include <algorithm>
#include <vector>

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

Vector RelaxEngine::ClosestPointOnTriangle(const Vector& p, const Vector& a, const Vector& b, const Vector& c)
{
    Vector ab = b - a;
    Vector ac = c - a;
    Vector ap = p - a;
    Float d1 = Dot(ab, ap);
    Float d2 = Dot(ac, ap);
    if (d1 <= 0.0 && d2 <= 0.0) return a;

    Vector bp = p - b;
    Float d3 = Dot(ab, bp);
    Float d4 = Dot(ac, bp);
    if (d3 >= 0.0 && d4 <= d3) return b;

    Float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0)
    {
        Float v = d1 / (d1 - d3);
        return a + ab * v;
    }

    Vector cp = p - c;
    Float d5 = Dot(ab, cp);
    Float d6 = Dot(ac, cp);
    if (d6 >= 0.0 && d5 <= d6) return c;

    Float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0)
    {
        Float w = d2 / (d2 - d6);
        return a + ac * w;
    }

    Float va = d3 * d6 - d5 * d4;
    if (va <= 0.0 && (d4 - d3) >= 0.0 && (d5 - d6) >= 0.0)
    {
        Float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return b + (c - b) * w;
    }

    Float denomSum = va + vb + vc;
    if (denomSum <= 1e-12) return a;
    Float denom = 1.0 / denomSum;
    Float v = vb * denom;
    Float w = vc * denom;
    return a + ab * v + ac * w;
}

RelaxEngine::RelaxEngine() = default;

RelaxEngine::~RelaxEngine()
{
    EndStroke();
}

Bool RelaxEngine::BeginStroke(PolygonObject* mesh, Int32 algorithm, Bool preserveCreases, Float creaseAngleDeg)
{
    m_strokeActive = true;
    m_cachedAlgorithm = algorithm;
    return EnsureMeshTopology(mesh, preserveCreases, creaseAngleDeg);
}

void RelaxEngine::EndStroke()
{
    m_strokeActive = false;
    m_cachedMesh = nullptr;
    m_initialPositions.Reset();
    m_allNeighbors.Reset();
    m_boundaryNeighbors.Reset();
    m_creaseNeighbors.Reset();
    m_incidentPolys.Reset();
    m_boundaryEdges.Reset();
}

Bool RelaxEngine::EnsureMeshTopology(PolygonObject* mesh, Bool preserveCreases, Float creaseAngleDeg)
{
    if (!mesh)
        return false;

    Int32 ptCount = mesh->GetPointCount();
    Int32 polyCount = mesh->GetPolygonCount();
    if (ptCount < 3 || polyCount == 0)
        return false;

    if (m_cachedMesh == mesh && m_allNeighbors.GetCount() == ptCount &&
        m_cachedPreserveCreases == preserveCreases &&
        std::abs(m_cachedCreaseAngleDeg - creaseAngleDeg) < 0.01)
    {
        return true;
    }

    m_cachedMesh = mesh;
    m_cachedPreserveCreases = preserveCreases;
    m_cachedCreaseAngleDeg = creaseAngleDeg;

    const Vector* pts = mesh->GetPointR();
    const CPolygon* polys = mesh->GetPolygonR();

    // Cache initial positions for surface reprojection
    m_initialPositions.Resize(ptCount) iferr_ignore("Resize initial positions");
    for (Int32 i = 0; i < ptCount; ++i)
    {
        m_initialPositions[i] = pts[i];
    }

    // Incident polygons per vertex
    m_incidentPolys.Resize(ptCount) iferr_ignore("Resize incidentPolys");
    for (Int32 i = 0; i < ptCount; ++i)
    {
        m_incidentPolys[i].Reset();
    }

    for (Int32 p = 0; p < polyCount; ++p)
    {
        const CPolygon& poly = polys[p];
        m_incidentPolys[poly.a].Append(p) iferr_ignore("Append inc poly");
        m_incidentPolys[poly.b].Append(p) iferr_ignore("Append inc poly");
        m_incidentPolys[poly.c].Append(p) iferr_ignore("Append inc poly");
        if (poly.c != poly.d)
        {
            m_incidentPolys[poly.d].Append(p) iferr_ignore("Append inc poly");
        }
    }

    // Calculate unit polygon normals for crease dihedral angle detection
    std::vector<Vector> polyUnitNormals(polyCount);
    for (Int32 p = 0; p < polyCount; ++p)
    {
        const CPolygon& poly = polys[p];
        Vector A = pts[poly.a];
        Vector B = pts[poly.b];
        Vector C = pts[poly.c];
        Vector N = Cross(B - A, C - A);
        if (poly.c != poly.d)
        {
            Vector D = pts[poly.d];
            N += Cross(C - A, D - A);
        }
        Float len = Sqrt(N.x * N.x + N.y * N.y + N.z * N.z);
        if (len > 1e-8)
            polyUnitNormals[p] = N / len;
        else
            polyUnitNormals[p] = Vector(0.0, 1.0, 0.0);
    }

    struct RawEdge
    {
        Int32 u;
        Int32 v;
        Int32 polyIndex;

        Bool operator<(const RawEdge& other) const
        {
            if (u != other.u) return u < other.u;
            if (v != other.v) return v < other.v;
            return polyIndex < other.polyIndex;
        }
    };

    std::vector<RawEdge> rawEdges;
    rawEdges.reserve((size_t)polyCount * 4);

    auto addRaw = [&](Int32 pIdx, Int32 a, Int32 b)
    {
        RawEdge re;
        re.u = std::min(a, b);
        re.v = std::max(a, b);
        re.polyIndex = pIdx;
        rawEdges.push_back(re);
    };

    for (Int32 p = 0; p < polyCount; ++p)
    {
        const CPolygon& poly = polys[p];
        addRaw(p, poly.a, poly.b);
        addRaw(p, poly.b, poly.c);
        if (poly.c != poly.d)
        {
            addRaw(p, poly.c, poly.d);
            addRaw(p, poly.d, poly.a);
        }
        else
        {
            addRaw(p, poly.c, poly.a);
        }
    }

    std::sort(rawEdges.begin(), rawEdges.end());

    m_allNeighbors.Resize(ptCount) iferr_ignore("Resize allNeighbors");
    m_boundaryNeighbors.Resize(ptCount) iferr_ignore("Resize boundaryNeighbors");
    m_creaseNeighbors.Resize(ptCount) iferr_ignore("Resize creaseNeighbors");
    m_boundaryEdges.Reset();

    for (Int32 i = 0; i < ptCount; ++i)
    {
        m_allNeighbors[i].Reset();
        m_boundaryNeighbors[i].Reset();
        m_creaseNeighbors[i].Reset();
    }

    Float cosCreaseThreshold = std::cos(creaseAngleDeg * (PI / 180.0));

    size_t edgeCount = rawEdges.size();
    size_t i = 0;
    while (i < edgeCount)
    {
        size_t j = i + 1;
        while (j < edgeCount && rawEdges[j].u == rawEdges[i].u && rawEdges[j].v == rawEdges[i].v)
        {
            ++j;
        }

        Int32 u = rawEdges[i].u;
        Int32 v = rawEdges[i].v;
        size_t count = j - i;

        if (u >= 0 && u < ptCount && v >= 0 && v < ptCount)
        {
            m_allNeighbors[u].Append(v) iferr_ignore("Append allNeighbors");
            m_allNeighbors[v].Append(u) iferr_ignore("Append allNeighbors");

            if (count == 1)
            {
                // Boundary edge
                m_boundaryNeighbors[u].Append(v) iferr_ignore("Append boundary");
                m_boundaryNeighbors[v].Append(u) iferr_ignore("Append boundary");
                BoundaryEdgeEntry be;
                be.u = u;
                be.v = v;
                m_boundaryEdges.Append(be) iferr_ignore("Append boundaryEdges");
            }
            else if (count == 2 && preserveCreases)
            {
                Int32 p1 = rawEdges[i].polyIndex;
                Int32 p2 = rawEdges[i + 1].polyIndex;
                Float dotVal = Dot(polyUnitNormals[p1], polyUnitNormals[p2]);
                dotVal = maxon::ClampValue(dotVal, -1.0_f, 1.0_f);
                if (dotVal < cosCreaseThreshold)
                {
                    // Dihedral angle exceeds threshold -> Crease edge
                    m_creaseNeighbors[u].Append(v) iferr_ignore("Append crease");
                    m_creaseNeighbors[v].Append(u) iferr_ignore("Append crease");
                }
            }
        }

        i = j;
    }

    return true;
}

Bool RelaxEngine::IsCursorNearBorder(PolygonObject* mesh, BaseDraw* bd, Float screenX, Float screenY, Float brushRadius)
{
    if (!mesh || !bd)
        return false;

    Int32 ptCount = mesh->GetPointCount();
    if (ptCount < 3)
        return false;

    if (!EnsureMeshTopology(mesh, m_cachedPreserveCreases, m_cachedCreaseAngleDeg))
        return false;

    if (m_boundaryEdges.GetCount() == 0)
        return false;

    const Vector* pts = mesh->GetPointR();
    Matrix mg = mesh->GetMg();

    Float minBorderDist = 1e30;

    for (Int32 k = 0; k < (Int32)m_boundaryEdges.GetCount(); ++k)
    {
        const BoundaryEdgeEntry& be = m_boundaryEdges[k];
        if (be.u < 0 || be.u >= ptCount || be.v < 0 || be.v >= ptCount) continue;

        Vector sA = bd->WS(mg * pts[be.u]);
        Vector sB = bd->WS(mg * pts[be.v]);
        if (sA.z <= 0.0 || sB.z <= 0.0) continue;

        Float d = DistToSegment2D(screenX, screenY, sA.x, sA.y, sB.x, sB.y);
        if (d < minBorderDist) minBorderDist = d;
    }

    if (minBorderDist <= brushRadius)
    {
        if (minBorderDist <= 15.0 || minBorderDist <= brushRadius * 0.4)
            return true;
    }

    return false;
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
    Bool useSelection,
    Int32 algorithm,
    Bool preserveCreases,
    Float creaseAngleDeg)
{
    if (!mesh || !bd || brushRadius <= 0.0 || strength <= 0.0)
        return false;

    Int32 ptCount = mesh->GetPointCount();
    Int32 polyCount = mesh->GetPolygonCount();
    if (ptCount < 3 || polyCount == 0)
        return false;

    if (!EnsureMeshTopology(mesh, preserveCreases, creaseAngleDeg))
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

    Float radiusSq = brushRadius * brushRadius;

    for (Int32 i = 0; i < ptCount; ++i)
    {
        if (useSelection && pointSel && !pointSel->IsSelected(i))
            continue;

        Vector worldPos = meshMg * pts[i];
        Vector screenPos = bd->WS(worldPos);
        if (screenPos.z <= 0.0) continue; // Behind camera

        Float dx = screenPos.x - screenX;
        Float dy = screenPos.y - screenY;
        Float distSq = dx * dx + dy * dy;
        if (distSq <= radiusSq)
        {
            Float dist = Sqrt(distSq);
            Float t = 1.0 - (dist / brushRadius);
            Float falloff = t * t * (3.0 - 2.0 * t); // Smoothstep falloff
            affectedVertices.Append(i) iferr_ignore("Append affected vertex");
            vertexWeights.Append(falloff * strength) iferr_ignore("Append weight");
        }
    }

    if (affectedVertices.GetCount() == 0)
        return false;

    // 2. Multi-pass smoothing iterations
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
            if (w <= 0.0001)
            {
                nextPositions[ai] = ptsW[idx];
                continue;
            }

            const maxon::BaseArray<Int32>& bNeighbors = m_boundaryNeighbors[idx];
            const maxon::BaseArray<Int32>& cNeighbors = m_creaseNeighbors[idx];
            const maxon::BaseArray<Int32>& nNeighbors = m_allNeighbors[idx];

            Bool isBoundary = (bNeighbors.GetCount() > 0);
            Bool isCrease = (preserveCreases && cNeighbors.GetCount() > 0 && !isBoundary);

            // =================================================================
            // 1. Boundary Vertex
            // =================================================================
            if (isBoundary)
            {
                if (lockBorder)
                {
                    nextPositions[ai] = ptsW[idx];
                    continue;
                }

                if (bNeighbors.GetCount() == 2)
                {
                    Vector pA = ptsW[bNeighbors[0]];
                    Vector pB = ptsW[bNeighbors[1]];
                    Vector targetPos = (pA + pB) * 0.5;

                    if (algorithm == RELAX_ALGO_TANGENTIAL || algorithm == RELAX_ALGO_PROJECT)
                    {
                        Vector segDir = pB - pA;
                        Float segLen = Sqrt(segDir.x * segDir.x + segDir.y * segDir.y + segDir.z * segDir.z);
                        if (segLen > 1e-6)
                        {
                            segDir /= segLen;
                            Vector delta = targetPos - ptsW[idx];
                            targetPos = ptsW[idx] + segDir * Dot(delta, segDir);
                        }
                    }

                    Vector candPos = ptsW[idx] + (targetPos - ptsW[idx]) * w;

                    if (algorithm == RELAX_ALGO_PROJECT && m_initialPositions.GetCount() == ptCount)
                    {
                        Vector iA = m_initialPositions[bNeighbors[0]];
                        Vector iB = m_initialPositions[bNeighbors[1]];
                        Vector ab = iB - iA;
                        Float abLenSq = ab.x * ab.x + ab.y * ab.y + ab.z * ab.z;
                        if (abLenSq > 1e-8)
                        {
                            Float t = Dot(candPos - iA, ab) / abLenSq;
                            t = maxon::ClampValue(t, 0.0_f, 1.0_f);
                            candPos = iA + ab * t;
                        }
                    }
                    nextPositions[ai] = candPos;
                }
                else
                {
                    // Boundary corner (1 or >2 boundary edges) -> keep fixed
                    nextPositions[ai] = ptsW[idx];
                }
                continue;
            }

            // =================================================================
            // 2. Crease / Feature Vertex
            // =================================================================
            if (isCrease)
            {
                if (lockInterior)
                {
                    nextPositions[ai] = ptsW[idx];
                    continue;
                }

                if (cNeighbors.GetCount() == 2)
                {
                    Vector pA = ptsW[cNeighbors[0]];
                    Vector pB = ptsW[cNeighbors[1]];
                    Vector targetPos = (pA + pB) * 0.5;

                    Vector segDir = pB - pA;
                    Float segLen = Sqrt(segDir.x * segDir.x + segDir.y * segDir.y + segDir.z * segDir.z);
                    if (segLen > 1e-6)
                    {
                        segDir /= segLen;
                        Vector delta = targetPos - ptsW[idx];
                        targetPos = ptsW[idx] + segDir * Dot(delta, segDir);
                    }

                    Vector candPos = ptsW[idx] + (targetPos - ptsW[idx]) * w;

                    if (algorithm == RELAX_ALGO_PROJECT && m_initialPositions.GetCount() == ptCount)
                    {
                        Vector iA = m_initialPositions[cNeighbors[0]];
                        Vector iB = m_initialPositions[cNeighbors[1]];
                        Vector ab = iB - iA;
                        Float abLenSq = ab.x * ab.x + ab.y * ab.y + ab.z * ab.z;
                        if (abLenSq > 1e-8)
                        {
                            Float t = Dot(candPos - iA, ab) / abLenSq;
                            t = maxon::ClampValue(t, 0.0_f, 1.0_f);
                            candPos = iA + ab * t;
                        }
                    }
                    nextPositions[ai] = candPos;
                }
                else
                {
                    // Crease corner / junction / end -> keep fixed
                    nextPositions[ai] = ptsW[idx];
                }
                continue;
            }

            // =================================================================
            // 3. Smooth Interior Vertex
            // =================================================================
            if (lockInterior)
            {
                nextPositions[ai] = ptsW[idx];
                continue;
            }

            if (nNeighbors.GetCount() == 0)
            {
                nextPositions[ai] = ptsW[idx];
                continue;
            }

            Vector sum(0.0);
            for (Int32 ni = 0; ni < (Int32)nNeighbors.GetCount(); ++ni)
            {
                sum += ptsW[nNeighbors[ni]];
            }
            Vector centroid = sum / (Float)nNeighbors.GetCount();
            Vector disp = centroid - ptsW[idx];

            Vector targetPos = centroid;

            if (algorithm == RELAX_ALGO_TANGENTIAL || algorithm == RELAX_ALGO_PROJECT)
            {
                // Current surface normal from incident polygons
                Vector vNormal(0.0);
                const maxon::BaseArray<Int32>& incPolys = m_incidentPolys[idx];
                for (Int32 pi = 0; pi < (Int32)incPolys.GetCount(); ++pi)
                {
                    Int32 polyIdx = incPolys[pi];
                    const CPolygon& poly = polys[polyIdx];
                    Vector A = ptsW[poly.a];
                    Vector B = ptsW[poly.b];
                    Vector C = ptsW[poly.c];
                    Vector N = Cross(B - A, C - A);
                    if (poly.c != poly.d)
                    {
                        Vector D = ptsW[poly.d];
                        N += Cross(C - A, D - A);
                    }
                    vNormal += N;
                }

                Float normLen = Sqrt(vNormal.x * vNormal.x + vNormal.y * vNormal.y + vNormal.z * vNormal.z);
                if (normLen > 1e-6)
                {
                    vNormal /= normLen;
                    Float normalDisp = Dot(disp, vNormal);
                    Vector tangentialDisp = disp - vNormal * normalDisp;
                    targetPos = ptsW[idx] + tangentialDisp;
                }
            }

            Vector candPos = ptsW[idx] + (targetPos - ptsW[idx]) * w;

            // =================================================================
            // 4. Project to Initial Surface (RELAX_ALGO_PROJECT)
            // =================================================================
            if (algorithm == RELAX_ALGO_PROJECT && m_initialPositions.GetCount() == ptCount)
            {
                const maxon::BaseArray<Int32>& incPolys = m_incidentPolys[idx];
                Float minDistSq = 1e30;
                Vector bestPoint = candPos;

                auto testPoly = [&](Int32 pIdx)
                {
                    if (pIdx < 0 || pIdx >= polyCount) return;
                    const CPolygon& p = polys[pIdx];
                    Vector a0 = m_initialPositions[p.a];
                    Vector b0 = m_initialPositions[p.b];
                    Vector c0 = m_initialPositions[p.c];

                    Vector q1 = ClosestPointOnTriangle(candPos, a0, b0, c0);
                    Vector diff1 = candPos - q1;
                    Float d1Sq = diff1.x * diff1.x + diff1.y * diff1.y + diff1.z * diff1.z;
                    if (d1Sq < minDistSq)
                    {
                        minDistSq = d1Sq;
                        bestPoint = q1;
                    }

                    if (p.c != p.d)
                    {
                        Vector d0 = m_initialPositions[p.d];
                        Vector q2 = ClosestPointOnTriangle(candPos, a0, c0, d0);
                        Vector diff2 = candPos - q2;
                        Float d2Sq = diff2.x * diff2.x + diff2.y * diff2.y + diff2.z * diff2.z;
                        if (d2Sq < minDistSq)
                        {
                            minDistSq = d2Sq;
                            bestPoint = q2;
                        }
                    }
                };

                // Test 1-ring incident polygons
                for (Int32 pi = 0; pi < (Int32)incPolys.GetCount(); ++pi)
                {
                    testPoly(incPolys[pi]);
                }

                // Also test 2-ring incident polygons from direct neighbors
                for (Int32 ni = 0; ni < (Int32)nNeighbors.GetCount(); ++ni)
                {
                    Int32 neighborIdx = nNeighbors[ni];
                    const maxon::BaseArray<Int32>& nPolys = m_incidentPolys[neighborIdx];
                    for (Int32 pi = 0; pi < (Int32)nPolys.GetCount(); ++pi)
                    {
                        testPoly(nPolys[pi]);
                    }
                }

                candPos = bestPoint;
            }

            nextPositions[ai] = candPos;
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
