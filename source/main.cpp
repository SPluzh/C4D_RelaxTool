#include "c4d.h"
#include "relax_tool.h"

namespace cinema
{

Bool PluginStart()
{
    return RegisterRelaxTool();
}

void PluginEnd()
{
}

Bool PluginMessage(Int32 id, void* data)
{
    switch (id)
    {
        case C4DPL_INIT_SYS:
            if (!g_resource.Init()) return false;
            return true;
        case C4DMSG_PRIORITY:
            return true;
        case C4DPL_BUILDMENU:
            break;
    }
    return false;
}

} // namespace cinema
