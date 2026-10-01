#include "relax_tool.h"
#include "description/toolrelaxtool.h"
#include "c4d_basecontainer.h"
#include "c4d_baseobject.h"
#include "c4d_general.h"
#include "c4d_gui.h"
#include "gui.h"

namespace cinema
{

Int32 RelaxToolData::GetState(BaseDocument* doc)
{
    return CMD_ENABLED;
}

Bool RelaxToolData::InitTool(BaseDocument* doc, BaseContainer& data, BaseThread* bt)
{
    if (!DescriptionToolData::InitTool(doc, data, bt))
        return false;

    m_isRelaxDragging = false;
    m_isResizingBrush = false;
    m_cursorInView = false;
    m_relaxLockBorder = false;
    m_relaxLockInterior = false;

    if (data.FindIndex(RELAX_RADIUS) == NOTOK)
    {
        InitDefaultSettings(doc, data);
    }

    return true;
}

void RelaxToolData::FreeTool(BaseDocument* doc, BaseContainer& data)
{
    m_engine.EndStroke();
    m_isRelaxDragging = false;
    m_isResizingBrush = false;
    m_cursorInView = false;
    DescriptionToolData::FreeTool(doc, data);
}

void RelaxToolData::InitDefaultSettings(BaseDocument* doc, BaseContainer& data)
{
    data.SetFloat(RELAX_RADIUS, 50.0);
    data.SetFloat(RELAX_STRENGTH, 0.35);
    data.SetInt32(RELAX_ITERATIONS, 1);
    data.SetInt32(RELAX_MODE, RELAX_MODE_AUTOLOCK);
    data.SetInt32(RELAX_ALGORITHM, RELAX_ALGO_TANGENTIAL);
    data.SetBool(RELAX_PRESERVE_CREASES, true);
    data.SetFloat(RELAX_CREASE_ANGLE, 45.0);
    data.SetBool(RELAX_USE_SELECTION, false);
    data.SetVector(RELAX_BRUSH_COLOR, Vector(0.3, 0.75, 1.0));
    data.SetVector(RELAX_ACTIVE_COLOR, Vector(0.15, 0.9, 1.0));
}

PolygonObject* RelaxToolData::GetActiveMesh(BaseDocument* doc)
{
    if (!doc) return nullptr;
    BaseObject* activeObj = doc->GetActiveObject();
    if (!activeObj || !activeObj->IsInstanceOf(Opolygon))
        return nullptr;
    return static_cast<PolygonObject*>(activeObj);
}

Bool RelaxToolData::GetCursorInfo(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, Float x, Float y, BaseContainer& bc)
{
    if (bc.GetId() == BFM_CURSORINFO_REMOVE)
    {
        m_cursorInView = false;
        DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW | DRAWFLAGS::NO_THREAD | DRAWFLAGS::NO_ANIMATION);
        return true;
    }

    m_cursorX = x;
    m_cursorY = y;
    m_cursorInView = true;

    PolygonObject* mesh = GetActiveMesh(doc);
    Float radius = data.GetFloat(RELAX_RADIUS, 50.0);
    Float strength = data.GetFloat(RELAX_STRENGTH, 0.35);
    Int32 mode = data.GetInt32(RELAX_MODE, RELAX_MODE_AUTOLOCK);

    String modeStr;
    switch (mode)
    {
        case RELAX_MODE_AUTOLOCK: modeStr = "Auto-lock"_s; break;
        case RELAX_MODE_INTERIOR: modeStr = "Interior Only"_s; break;
        case RELAX_MODE_BORDER:   modeStr = "Border Only"_s; break;
        case RELAX_MODE_ALL:      modeStr = "All Vertices"_s; break;
        default:                  modeStr = "Auto-lock"_s; break;
    }

    if (mesh)
    {
        StatusSetText(FormatString("Relax Tool | Mesh: @ | Mode: @ | Radius: @ px | Strength: @ | LMB: Relax | MMB: Resize Brush"_s,
            mesh->GetName(), modeStr, (Int32)(radius + 0.5), strength));
    }
    else
    {
        StatusSetText(FormatString("Relax Tool | [Select Polygon Object] | Mode: @ | Radius: @ px | Strength: @"_s,
            modeStr, (Int32)(radius + 0.5), strength));
    }

    bc.SetInt32(RESULT_CURSOR, MOUSE_CROSS);
    DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW | DRAWFLAGS::NO_THREAD | DRAWFLAGS::NO_ANIMATION);
    return true;
}

Bool RelaxToolData::MouseInput(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg)
{
    if (!doc || !bd || !win) return false;

    Int32 channel = msg.GetInt32(BFM_INPUT_CHANNEL);
    Float mx = msg.GetFloat(BFM_INPUT_X);
    Float my = msg.GetFloat(BFM_INPUT_Y);
    Int32 qual = msg.GetInt32(BFM_INPUT_QUALIFIER);

    // =========================================================================
    // RESIZE BRUSH RADIUS: MMB DRAG OR CTRL + RMB DRAG
    // =========================================================================
    Bool isMmb = (channel == BFM_INPUT_MOUSEMIDDLE);
    Bool isCtrlRmb = (channel == BFM_INPUT_MOUSERIGHT && (qual & QCTRL) != 0);

    if (isMmb || isCtrlRmb)
    {
        Int32 dragButton = isMmb ? KEY_MMIDDLE : KEY_MRIGHT;
        Float initialRadius = data.GetFloat(RELAX_RADIUS, 50.0);
        Float currentRadius = initialRadius;

        m_isResizingBrush = true;
        m_brushResizeCenterX = mx;
        m_brushResizeCenterY = my;
        m_cursorX = mx;
        m_cursorY = my;

        BaseContainer device;
        win->MouseDragStart(dragButton, mx, my, MOUSEDRAGFLAGS::DONTHIDEMOUSE);

        Float dx, dy;
        while (win->MouseDrag(&dx, &dy, &device) == MOUSEDRAGRESULT::CONTINUE)
        {
            if (dx == 0.0 && dy == 0.0) continue;

            currentRadius += dx;
            currentRadius = maxon::ClampValue(currentRadius, 5.0_f, 500.0_f);

            data.SetFloat(RELAX_RADIUS, currentRadius);

            m_cursorX += dx;
            m_cursorY += dy;

            StatusSetText(FormatString("Relax Tool [RESIZE BRUSH] | Radius: @ px (Drag Left/Right to adjust)"_s, (Int32)(currentRadius + 0.5)));
            DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW | DRAWFLAGS::NO_THREAD | DRAWFLAGS::NO_ANIMATION);
        }

        win->MouseDragEnd();
        m_isResizingBrush = false;

        data.SetFloat(RELAX_RADIUS, currentRadius);
        StatusSetText(FormatString("Relax Tool: Radius set to @ px"_s, (Int32)(currentRadius + 0.5)));

        EventAdd();
        DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW | DRAWFLAGS::NO_THREAD | DRAWFLAGS::NO_ANIMATION);
        return true;
    }

    if (channel != BFM_INPUT_MOUSELEFT)
        return false;

    // =========================================================================
    // LMB DRAG -> RELAX MESH
    // =========================================================================
    PolygonObject* mesh = GetActiveMesh(doc);
    if (!mesh)
    {
        StatusSetText("Relax Tool: Please select a Polygon Object to relax."_s);
        return true;
    }

    Float brushRadius = data.GetFloat(RELAX_RADIUS, 50.0);
    Float strength = data.GetFloat(RELAX_STRENGTH, 0.35);
    Int32 iterations = data.GetInt32(RELAX_ITERATIONS, 1);
    Int32 relaxMode = data.GetInt32(RELAX_MODE, RELAX_MODE_AUTOLOCK);
    Bool useSelection = data.GetBool(RELAX_USE_SELECTION, false);
    Int32 algorithm = data.GetInt32(RELAX_ALGORITHM, RELAX_ALGO_TANGENTIAL);
    Bool preserveCreases = data.GetBool(RELAX_PRESERVE_CREASES, true);
    Float creaseAngle = data.GetFloat(RELAX_CREASE_ANGLE, 45.0);

    m_engine.BeginStroke(mesh, algorithm, preserveCreases, creaseAngle);

    Bool lockBorder = false;
    Bool lockInterior = false;

    if (relaxMode == RELAX_MODE_INTERIOR)
    {
        lockBorder = true;
        lockInterior = false;
    }
    else if (relaxMode == RELAX_MODE_BORDER)
    {
        lockBorder = false;
        lockInterior = true;
    }
    else if (relaxMode == RELAX_MODE_ALL)
    {
        lockBorder = false;
        lockInterior = false;
    }
    else // RELAX_MODE_AUTOLOCK
    {
        Bool startOnBorder = m_engine.IsCursorNearBorder(mesh, bd, mx, my, brushRadius);
        if (startOnBorder)
        {
            lockBorder = false;
            lockInterior = true;
        }
        else
        {
            lockBorder = true;
            lockInterior = false;
        }
    }

    doc->StartUndo();
    doc->AddUndo(UNDOTYPE::CHANGE, mesh);

    m_isRelaxDragging = true;
    m_relaxLockBorder = lockBorder;
    m_relaxLockInterior = lockInterior;
    m_cursorX = mx;
    m_cursorY = my;

    // Initial relaxation step at click position
    m_engine.RelaxVertices(mesh, bd, mx, my, brushRadius, strength, lockBorder, lockInterior, iterations, useSelection, algorithm, preserveCreases, creaseAngle);
    DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW | DRAWFLAGS::NO_THREAD | DRAWFLAGS::NO_ANIMATION);

    BaseContainer device;
    win->MouseDragStart(KEY_MLEFT, mx, my, MOUSEDRAGFLAGS::DONTHIDEMOUSE | MOUSEDRAGFLAGS::NOMOVE);

    Float dx, dy;
    while (win->MouseDrag(&dx, &dy, &device) == MOUSEDRAGRESULT::CONTINUE)
    {
        if (dx == 0.0 && dy == 0.0) continue;
        mx += dx;
        my += dy;
        m_cursorX = mx;
        m_cursorY = my;

        m_engine.RelaxVertices(mesh, bd, mx, my, brushRadius, strength, lockBorder, lockInterior, iterations, useSelection, algorithm, preserveCreases, creaseAngle);
        DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW | DRAWFLAGS::NO_THREAD | DRAWFLAGS::NO_ANIMATION);
    }

    win->MouseDragEnd();
    m_engine.EndStroke();
    m_isRelaxDragging = false;
    m_relaxLockBorder = false;
    m_relaxLockInterior = false;
    doc->EndUndo();
    EventAdd();
    DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW | DRAWFLAGS::NO_THREAD | DRAWFLAGS::NO_ANIMATION);

    String algoName = "Tangential"_s;
    if (algorithm == RELAX_ALGO_PROJECT)
        algoName = "Projected"_s;
    else if (algorithm == RELAX_ALGO_LAPLACIAN)
        algoName = "Laplacian"_s;

    if (lockInterior)
        StatusSetText(FormatString("Relax Tool [@]: Relaxed border vertices (interior locked)."_s, algoName));
    else if (lockBorder)
        StatusSetText(FormatString("Relax Tool [@]: Relaxed interior vertices (border locked)."_s, algoName));
    else
        StatusSetText(FormatString("Relax Tool [@]: Relaxed vertices."_s, algoName));

    return true;
}

TOOLDRAW RelaxToolData::Draw(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, BaseDrawHelp* bh, BaseThread* bt, TOOLDRAWFLAGS flags)
{
    if (!bd)
        return TOOLDRAW::NONE;

    if (!m_cursorInView && !m_isResizingBrush && !m_isRelaxDragging)
        return TOOLDRAW::NONE;

    Float relaxRadius = data.GetFloat(RELAX_RADIUS, 50.0);
    Float cx = m_isResizingBrush ? m_brushResizeCenterX : m_cursorX;
    Float cy = m_isResizingBrush ? m_brushResizeCenterY : m_cursorY;

    Vector defaultBrushColor = data.GetVector(RELAX_BRUSH_COLOR, Vector(0.3, 0.75, 1.0));
    Vector defaultActiveColor = data.GetVector(RELAX_ACTIVE_COLOR, Vector(0.15, 0.9, 1.0));

    bd->SetMatrix_Screen();
    Vector circleColor = m_isResizingBrush ? Vector(1.0, 1.0, 1.0) :
                        (m_isRelaxDragging ? defaultActiveColor : defaultBrushColor);
    bd->SetPen(circleColor);

    const Int32 numSegs = 48;
    for (Int32 i = 0; i < numSegs; ++i)
    {
        Float a0 = (Float)i * (2.0 * PI / (Float)numSegs);
        Float a1 = (Float)(i + 1) * (2.0 * PI / (Float)numSegs);
        Vector p0(cx + cos(a0) * relaxRadius, cy + sin(a0) * relaxRadius, 0.0);
        Vector p1(cx + cos(a1) * relaxRadius, cy + sin(a1) * relaxRadius, 0.0);
        bd->DrawLine(p0, p1, 0);
    }

    if (m_isResizingBrush)
    {
        // Center crosshair
        bd->DrawLine(Vector(cx - 5.0, cy, 0.0), Vector(cx + 5.0, cy, 0.0), 0);
        bd->DrawLine(Vector(cx, cy - 5.0, 0.0), Vector(cx, cy + 5.0, 0.0), 0);
        // Horizontal radius indicator line to the edge
        bd->DrawLine(Vector(cx, cy, 0.0), Vector(cx + relaxRadius, cy, 0.0), 0);
    }

    bd->SetMatrix_Matrix(nullptr, Matrix());
    return TOOLDRAW::NONE;
}

Bool RelaxToolData::Message(BaseDocument* doc, BaseContainer& data, Int32 type, void* t_data)
{
    switch (type)
    {
        case MSG_TOOL_ASK:
        {
            ToolAskMsgData* ask = static_cast<ToolAskMsgData*>(t_data);
            if (ask)
            {
                ask->use_middlemouse = true;
                ask->resize_allowed = true;
            }
            return true;
        }

        case MSG_TOOL_RESIZE:
        {
            ToolResizeData* d = static_cast<ToolResizeData*>(t_data);
            if (!d || !d->data)
                return false;

            switch (d->pass)
            {
                case ToolResizeData::RESIZE_PASS_INIT:
                {
                    d->cross_type = true;
                    d->falloff.show = true;
                    d->falloff.size = data.GetFloat(RELAX_RADIUS, 50.0);
                    d->falloff.opacity = data.GetFloat(RELAX_STRENGTH, 0.35);
                    d->falloff.color = Vector(1.0, 1.0, 1.0);
                    d->falloff.position.off = Vector(m_cursorX, m_cursorY, 0.0);
                    m_isResizingBrush = true;
                    m_brushResizeCenterX = m_cursorX;
                    m_brushResizeCenterY = m_cursorY;
                    return true;
                }

                case ToolResizeData::RESIZE_PASS_RESIZE:
                {
                    if (d->horizontal)
                    {
                        Float radius = data.GetFloat(RELAX_RADIUS, 50.0);
                        radius += (Float)d->delta;
                        radius = maxon::ClampValue(radius, 5.0_f, 500.0_f);
                        data.SetFloat(RELAX_RADIUS, radius);
                        d->falloff.size = radius;
                        d->cursor_text = FormatString("Radius: @ px"_s, (Int32)(radius + 0.5));
                        StatusSetText(FormatString("Relax Tool [RESIZE BRUSH] | Radius: @ px"_s, (Int32)(radius + 0.5)));
                    }
                    else
                    {
                        Float strength = data.GetFloat(RELAX_STRENGTH, 0.35);
                        strength += (Float)d->delta * 0.005;
                        strength = maxon::ClampValue(strength, 0.01_f, 1.0_f);
                        data.SetFloat(RELAX_STRENGTH, strength);
                        d->falloff.opacity = strength;
                        d->cursor_text = FormatString("Strength: @"_s, strength);
                        StatusSetText(FormatString("Relax Tool [RESIZE BRUSH] | Strength: @"_s, strength));
                    }
                    DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW | DRAWFLAGS::NO_THREAD | DRAWFLAGS::NO_ANIMATION);
                    return true;
                }

                case ToolResizeData::RESIZE_PASS_END:
                case ToolResizeData::RESIZE_PASS_RESET:
                {
                    m_isResizingBrush = false;
                    EventAdd();
                    DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW | DRAWFLAGS::NO_THREAD | DRAWFLAGS::NO_ANIMATION);
                    return true;
                }

                default:
                    break;
            }
            return true;
        }
    }

    return DescriptionToolData::Message(doc, data, type, t_data);
}

Bool RelaxToolData::KeyboardInput(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg)
{
    Int32 key = msg.GetInt32(BFM_INPUT_CHANNEL);
    if (key == KEY_ESC)
    {
        m_engine.EndStroke();
        m_isRelaxDragging = false;
        m_isResizingBrush = false;
        DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW | DRAWFLAGS::NO_THREAD | DRAWFLAGS::NO_ANIMATION);
        return true;
    }

    return false;
}

Bool RegisterRelaxTool()
{
    return RegisterToolPlugin(
        PLUGIN_ID_RELAXTOOL,
        "Relax Tool"_s,
        PLUGINFLAG_TOOL_HIGHLIGHT,
        AutoBitmap("relaxtool.png"_s),
        "Relax Tool\n- LMB Drag: Relax mesh vertices\n- MMB Drag: Resize brush radius"_s,
        NewObjClear(RelaxToolData)
    );
}

} // namespace cinema
