class GC_RoundedCanvasComponent : SCR_ScriptedWidgetComponent
{
    protected CanvasWidget m_wCanvas;
    protected ref array<float> m_aVertices = {};
    protected ref array<ref CanvasWidgetCommand> m_aDrawCommands = {};

    override void HandlerAttached(Widget w)
    {
        super.HandlerAttached(w);

        m_wCanvas = CanvasWidget.Cast(w);
        if (!m_wCanvas)
            return;

        GetGame().GetCallqueue().CallLater(Redraw, 0);
    }

    protected void Redraw()
    {
        if (!m_wCanvas)
            return;

        vector canvasSize = m_wCanvas.GetSizeInUnits();
        if (canvasSize[0] <= 0 || canvasSize[1] <= 0)
        {
            GetGame().GetCallqueue().CallLater(Redraw, 0);
            return;
        }

        float smallerDimension = canvasSize[0];
        if (canvasSize[1] < smallerDimension)
            smallerDimension = canvasSize[1];

        float cornerRadius = smallerDimension * 0.08;
        if (cornerRadius < 8)
            cornerRadius = 8;
        if (cornerRadius > 16)
            cornerRadius = 16;

        m_aVertices = {};
        m_wCanvas.TessellateRoundedRectangle(
            Vector(0, 0, 0),
            Vector(canvasSize[0], canvasSize[1], 0),
            cornerRadius,
            24,
            RectangleCorner.All,
            m_aVertices
        );

        PolygonDrawCommand roundedRectangle = new PolygonDrawCommand();
        roundedRectangle.m_iColor = 0xcc101820;
        roundedRectangle.m_Vertices = m_aVertices;

        m_aDrawCommands = {};
        m_aDrawCommands.Insert(roundedRectangle);
        m_wCanvas.SetDrawCommands(m_aDrawCommands);
    }
}
