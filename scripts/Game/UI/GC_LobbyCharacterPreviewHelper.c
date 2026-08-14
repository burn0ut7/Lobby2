class GC_LobbyCharacterPreviewHelper : ScriptedWidgetComponent
{
    protected InputManager m_InputManager;
    protected bool m_bMouseRotate;
    protected bool m_bMouseRoll;
    protected bool m_bMouseWheel;
    protected ItemPreviewWidget m_wCharacter;
	protected Widget m_wInteractionArea;
    protected float m_fZoomInput;
    protected vector m_vRotationInput = vector.Zero;
    protected float m_fLastPanX;
    protected float m_fLastPanY;
    protected float m_fLastZoom;
    protected float m_fLastMouseX;
    protected float m_fLastMouseY;
    protected float m_fMouseDeltaX;
    protected float m_fMouseDeltaY;
    protected float m_fWheelInput;
    protected bool m_bHasMouseSample;
	protected int m_iDebugManipulationFrames;
	protected bool m_bDebugCameraUpdateLogged;

    protected bool CanProcessEvent(int x, int y)
    {
        if (!m_wInteractionArea)
            return false;

        float sizeX, sizeY;
        float posX, posY;
		m_wInteractionArea.GetScreenSize(sizeX, sizeY);
		m_wInteractionArea.GetScreenPos(posX, posY);

        return x >= posX && x <= posX + sizeX && y >= posY && y <= posY + sizeY;
    }

    protected void ProcessInput()
    {
        const float inputResetEpsilon = 10;

        m_InputManager.ActivateContext("CharacterFreelookMouseContext");
        m_fLastPanX = m_InputManager.GetActionValue("Inventory_InspectPanX");
        m_fLastPanY = m_InputManager.GetActionValue("Inventory_InspectPanY");
        m_fLastZoom = m_InputManager.GetActionValue("Inventory_InspectZoom");

        float mouseX = m_InputManager.GetActionValue("MouseX");
        float mouseY = m_InputManager.GetActionValue("MouseY");
        m_fMouseDeltaX = 0;
        m_fMouseDeltaY = 0;

        if (m_bMouseRotate || m_bMouseRoll)
        {
            if (!m_bHasMouseSample)
            {
                m_fLastMouseX = mouseX;
                m_fLastMouseY = mouseY;
                m_bHasMouseSample = true;
            }
            else
            {
                m_fMouseDeltaX = (mouseX - m_fLastMouseX) / 15;
                m_fMouseDeltaY = (mouseY - m_fLastMouseY) / 15;
                m_fLastMouseX = mouseX;
                m_fLastMouseY = mouseY;
            }

			if (m_iDebugManipulationFrames < 5)
			{
				PrintFormat("GC | Preview input sample frame=%1 mouse=(%2,%3) delta=(%4,%5) rotate=%6 roll=%7", m_iDebugManipulationFrames, mouseX, mouseY, m_fMouseDeltaX, m_fMouseDeltaY, m_bMouseRotate, m_bMouseRoll);
				m_iDebugManipulationFrames++;
			}
        }
        else
        {
            m_bHasMouseSample = false;
        }

        if (m_vRotationInput[0] * m_vRotationInput[0] < inputResetEpsilon)
            m_vRotationInput[0] = 0;
        if (m_vRotationInput[1] * m_vRotationInput[1] < inputResetEpsilon)
            m_vRotationInput[1] = 0;
        if (m_vRotationInput[2] * m_vRotationInput[2] < inputResetEpsilon)
            m_vRotationInput[2] = 0;
        if (m_fZoomInput * m_fZoomInput < inputResetEpsilon)
            m_fZoomInput = 0;

        if (m_bMouseRotate)
        {
            m_vRotationInput[1] = Math.Clamp(m_vRotationInput[1] + m_fMouseDeltaX, -10, 10);
            m_vRotationInput[0] = Math.Clamp(m_vRotationInput[0] - m_fMouseDeltaY, -10, 10);
        }

        if (m_bMouseRoll)
            m_vRotationInput[2] = Math.Clamp(m_vRotationInput[2] + m_fMouseDeltaX, -10, 10);

        if (m_bMouseWheel)
        {
            m_fZoomInput += m_fWheelInput * 0.1;
            m_fWheelInput = 0;
        }

        if (!m_InputManager.IsUsingMouseAndKeyboard())
        {
            m_vRotationInput[1] = 1000.0 * m_vRotationInput[1] + m_fLastPanX;
            m_vRotationInput[0] = 1000.0 * m_vRotationInput[0] - m_fLastPanY;
            m_fZoomInput += 0.01 * m_fLastZoom;
        }
    }

    override bool OnMouseButtonDown(Widget w, int x, int y, int button)
    {
		bool inside = CanProcessEvent(x, y);
		PrintFormat("GC | Preview input mouse-down callback target=%1 button=%2 cursor=(%3,%4) inside=%5", w, button, x, y, inside);
		if (!inside)
			return false;

		if (button == 0)
			m_bMouseRotate = true;
		else if (button == 1)
			m_bMouseRoll = true;
		else
			return false;

		m_iDebugManipulationFrames = 0;
		m_bDebugCameraUpdateLogged = false;
		PrintFormat("GC | Preview input mouse-down accepted rotate=%1 roll=%2", m_bMouseRotate, m_bMouseRoll);

        return true;
    }

    override bool OnMouseButtonUp(Widget w, int x, int y, int button)
    {
		bool handled = m_bMouseRotate || m_bMouseRoll;
		PrintFormat("GC | Preview input mouse-up callback target=%1 button=%2 cursor=(%3,%4) handled=%5", w, button, x, y, handled);
        if (button == 0)
            m_bMouseRotate = false;
        else if (button == 1)
            m_bMouseRoll = false;

		return handled;
    }

    override bool OnMouseWheel(Widget w, int x, int y, int wheel)
    {
		m_bMouseWheel = CanProcessEvent(x, y);
		PrintFormat("GC | Preview input mouse-wheel callback target=%1 wheel=%2 cursor=(%3,%4) inside=%5", w, wheel, x, y, m_bMouseWheel);
        if (m_bMouseWheel)
            m_fWheelInput += wheel;
		return m_bMouseWheel;
    }

    bool Update(float timeSlice, inout PreviewRenderAttributes playerRenderAttributes)
    {
        const float rotationSensitivity = 10000;
        const float zoomSensitivity = -10000;
        const float interpolationAlpha = 0.15;

        ProcessInput();

        vector updatedRotation = "0 0 0";
        updatedRotation[0] = m_vRotationInput[0] * interpolationAlpha;
        updatedRotation[1] = m_vRotationInput[1] * interpolationAlpha;
        updatedRotation[2] = m_vRotationInput[2] * interpolationAlpha;

        m_vRotationInput[0] = m_vRotationInput[0] - updatedRotation[0];
        m_vRotationInput[1] = m_vRotationInput[1] - updatedRotation[1];
        m_vRotationInput[2] = m_vRotationInput[2] - updatedRotation[2];

        float zoom = m_fZoomInput * interpolationAlpha;
        m_fZoomInput -= zoom;

        updatedRotation[0] = updatedRotation[0] * timeSlice * rotationSensitivity;
        updatedRotation[1] = updatedRotation[1] * timeSlice * rotationSensitivity;
        updatedRotation[2] = updatedRotation[2] * timeSlice * rotationSensitivity;
        zoom = zoom * timeSlice * zoomSensitivity;

        bool changesApplied = false;
        if (updatedRotation[0] * updatedRotation[0] + updatedRotation[1] * updatedRotation[1] + updatedRotation[2] * updatedRotation[2] > 0)
        {
            const vector limitsMin = "-90 -180 -180";
            const vector limitsMax = "90 180 180";
            playerRenderAttributes.RotateItemCamera(updatedRotation, limitsMin, limitsMax);
            changesApplied = true;
        }

        if (zoom != 0)
        {
            playerRenderAttributes.ZoomCamera(zoom);
            changesApplied = true;
        }

		if (changesApplied && !m_bDebugCameraUpdateLogged)
		{
			PrintFormat("GC | Preview camera update applied rotation=%1 zoom=%2", updatedRotation, zoom);
			m_bDebugCameraUpdateLogged = true;
		}

        return changesApplied;
    }

    bool IsManipulatingPreview()
    {
        return m_bMouseRotate || m_bMouseRoll;
    }

    void Destroy()
    {
		PrintFormat("GC | Preview input handler destroy interaction=%1", m_wInteractionArea);
        if (m_wInteractionArea)
            m_wInteractionArea.RemoveHandler(this);

        m_wCharacter = null;
		m_wInteractionArea = null;
        m_InputManager = null;
    }

    void GC_LobbyCharacterPreviewHelper(ItemPreviewWidget characterWidget, Widget interactionArea)
    {
        if (!characterWidget || !interactionArea)
            return;

        m_InputManager = GetGame().GetInputManager();
        m_wCharacter = characterWidget;
		m_wInteractionArea = interactionArea;
		m_wInteractionArea.AddHandler(this);

		float sizeX, sizeY;
		float posX, posY;
		m_wInteractionArea.GetScreenSize(sizeX, sizeY);
		m_wInteractionArea.GetScreenPos(posX, posY);
		PrintFormat("GC | Preview input handler attached interaction=%1 bounds=(%2,%3 %4x%5)", m_wInteractionArea, posX, posY, sizeX, sizeY);
    }
};
