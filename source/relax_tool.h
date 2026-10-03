#ifndef RELAX_TOOL_H__
#define RELAX_TOOL_H__

#include "c4d.h"
#include "c4d_descriptiondialog.h"
#include "relax_engine.h"

// Registered Plugin ID from Plugincafe
#define PLUGIN_ID_RELAXTOOL 1070822

namespace cinema
{

class RelaxToolData : public DescriptionToolData
{
public:
    virtual Int32 GetToolPluginId() const override { return PLUGIN_ID_RELAXTOOL; }
    virtual const String GetResourceSymbol() const override { return "toolrelaxtool"_s; }

    virtual Int32    GetState(BaseDocument* doc) override;
    virtual Bool     GetCursorInfo(BaseDocument* doc, BaseContainer& data,
                                   BaseDraw* bd, Float x, Float y,
                                   BaseContainer& bc) override;
    virtual Bool     MouseInput(BaseDocument* doc, BaseContainer& data,
                                BaseDraw* bd, EditorWindow* win,
                                const BaseContainer& msg) override;
    virtual TOOLDRAW Draw(BaseDocument* doc, BaseContainer& data,
                          BaseDraw* bd, BaseDrawHelp* bh,
                          BaseThread* bt, TOOLDRAWFLAGS flags) override;
    virtual Bool     InitTool(BaseDocument* doc, BaseContainer& data,
                              BaseThread* bt) override;
    virtual void     FreeTool(BaseDocument* doc, BaseContainer& data) override;
    virtual void     InitDefaultSettings(BaseDocument* doc,
                                         BaseContainer& data) override;
    virtual Bool     Message(BaseDocument* doc, BaseContainer& data,
                             Int32 type, void* t_data) override;
    virtual Bool     KeyboardInput(BaseDocument* doc, BaseContainer& data,
                                   BaseDraw* bd, EditorWindow* win,
                                   const BaseContainer& msg) override;

private:
    PolygonObject* GetActiveMesh(BaseDocument* doc);

    RelaxEngine m_engine;

    // Brush state
    Float m_cursorX = 0.0;
    Float m_cursorY = 0.0;
    Bool  m_cursorInView = false;
    Bool  m_isRelaxDragging = false;
    Bool  m_isResizingBrush = false;
    Bool  m_relaxLockBorder = false;
    Bool  m_relaxLockInterior = false;
    Float m_brushResizeCenterX = 0.0;
    Float m_brushResizeCenterY = 0.0;
};

Bool RegisterRelaxTool();

} // namespace cinema

#endif // RELAX_TOOL_H__
