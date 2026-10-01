#ifndef RELAX_ENGINE_H__
#define RELAX_ENGINE_H__

#include "c4d.h"

namespace cinema
{

class RelaxEngine
{
public:
    // Pure Laplacian relax within screen-space brush radius.
    // Operates directly on the active PolygonObject with optional boundary/interior locking,
    // multi-pass iterations, and selection filtering.
    Bool RelaxVertices(
        PolygonObject* mesh,
        BaseDraw* bd,
        Float screenX, Float screenY,
        Float brushRadius,
        Float strength,
        Bool lockBorder,
        Bool lockInterior,
        Int32 iterations = 1,
        Bool useSelection = false
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
};

} // namespace cinema

#endif // RELAX_ENGINE_H__
