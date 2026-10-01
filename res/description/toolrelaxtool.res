CONTAINER toolrelaxtool
{
    NAME toolrelaxtool;
    INCLUDE ToolBase;

    GROUP MDATA_MAINGROUP
    {
        GROUP RELAX_GROUP_BRUSH
        {
            DEFAULT 1;

            REAL RELAX_RADIUS     { MIN 5.0; MAX 500.0; MINSLIDER 10.0; MAXSLIDER 200.0; STEP 5.0; CUSTOMGUI REALSLIDER; }
            REAL RELAX_STRENGTH   { MIN 0.01; MAX 1.0; MINSLIDER 0.05; MAXSLIDER 1.0; STEP 0.05; CUSTOMGUI REALSLIDER; }
            LONG RELAX_ITERATIONS { MIN 1; MAX 20; MINSLIDER 1; MAXSLIDER 10; }
        }

        GROUP RELAX_GROUP_MODE
        {
            DEFAULT 1;

            LONG RELAX_MODE
            {
                CYCLE
                {
                    RELAX_MODE_AUTOLOCK;
                    RELAX_MODE_INTERIOR;
                    RELAX_MODE_BORDER;
                    RELAX_MODE_ALL;
                }
            }
        }

        GROUP RELAX_GROUP_SELECTION
        {
            DEFAULT 1;

            BOOL RELAX_USE_SELECTION { }
        }

        GROUP RELAX_GROUP_VISUAL
        {
            DEFAULT 0;

            COLOR RELAX_BRUSH_COLOR  { }
            COLOR RELAX_ACTIVE_COLOR { }
        }
    }
}
