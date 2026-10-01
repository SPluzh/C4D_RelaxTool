#ifndef TOOLRELAXTOOL_H__
#define TOOLRELAXTOOL_H__

enum
{
    // Brush
    RELAX_GROUP_BRUSH       = 3000,
    RELAX_RADIUS            = 3001,
    RELAX_STRENGTH          = 3002,
    RELAX_ITERATIONS        = 3003,

    // Mode
    RELAX_GROUP_MODE        = 3010,
    RELAX_MODE              = 3011,
    RELAX_MODE_AUTOLOCK     = 0,
    RELAX_MODE_INTERIOR     = 1,
    RELAX_MODE_BORDER       = 2,
    RELAX_MODE_ALL          = 3,

    // Shape Preservation / Algorithm
    RELAX_GROUP_SHAPE       = 3040,
    RELAX_ALGORITHM         = 3041,
    RELAX_ALGO_LAPLACIAN    = 0,
    RELAX_ALGO_TANGENTIAL   = 1,
    RELAX_ALGO_PROJECT      = 2,

    RELAX_PRESERVE_CREASES  = 3042,
    RELAX_CREASE_ANGLE      = 3043,

    // Selection
    RELAX_GROUP_SELECTION   = 3020,
    RELAX_USE_SELECTION     = 3021,

    // Visual
    RELAX_GROUP_VISUAL      = 3030,
    RELAX_BRUSH_COLOR       = 3031,
    RELAX_ACTIVE_COLOR      = 3032
};

#endif // TOOLRELAXTOOL_H__
