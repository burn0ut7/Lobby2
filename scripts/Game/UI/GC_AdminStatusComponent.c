//! Owner-only server samples. No collection or UI timer while the tab is hidden.
class GC_AdminStatusComponent : SCR_ScriptedWidgetComponent
{
	protected GC_LobbyPlayerController m_Controller;
	protected CanvasWidget m_Canvas;
	protected GC_LobbyManager m_Manager;
	protected bool m_bActive;
	protected int m_iRequest = -1;
	protected int m_iReceivedAt;
	protected int m_iGraphSampleAt = -5000;
	protected int m_iRequestedAt;
	protected bool m_bPending;
	protected ref array<int> m_aFPS = {};
	protected ref array<ref CanvasWidgetCommand> m_aCommands = {};

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_Controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		m_Manager = GC_LobbyManager.GetManager();
		if (m_Manager)
		{
			m_Manager.m_OnPlayerConnected.Insert(OnPopulationChanged);
			m_Manager.m_OnPlayerDisconnected.Insert(OnPopulationChanged);
			m_Manager.m_OnPlayerRoleChanged.Insert(OnPopulationChanged);
		}
		m_Canvas = CanvasWidget.Cast(w.FindAnyWidget("StatusGraph"));
		if (m_Controller)
			m_Controller.m_OnGC_ServerStatus.Insert(OnSample);
	}

	//------------------------------------------------------------------------------------------------
	void SetActive(bool active)
	{
		m_wRoot.SetVisible(active);
		if (m_bActive == active)
			return;
		m_bActive = active;
		GetGame().GetCallqueue().Remove(Poll);
		GetGame().GetCallqueue().Remove(DrawGraph);
		GetGame().GetCallqueue().Remove(RequestAfterPopulationChange);
		m_iRequest = -1;
		m_bPending = false;
		if (active)
		{
			if (!m_iReceivedAt)
				ClearValues("Waiting for server...");
			else
				GetGame().GetCallqueue().CallLater(DrawGraph, 0);
			Poll();
			if (m_bActive)
				GetGame().GetCallqueue().CallLater(Poll, 1000, true);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void SetText(string name, string text)
	{
		TextWidget label = TextWidget.Cast(m_wRoot.FindAnyWidget(name));
		if (label.GetText() != text)
			label.SetText(text);
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearValues(string status)
	{
		SetText("StatusPlayers", "—");
		SetText("StatusAdmins", "—");
		SetText("StatusFPS", "—");
		SetText("StatusUptime", "—");
		SetText("StatusAI", "—");
		SetText("StatusFeedback", status);
		m_aCommands.Clear();
		m_Canvas.SetDrawCommands(m_aCommands);
	}

	//------------------------------------------------------------------------------------------------
	protected void Poll()
	{
		if (!m_bActive)
			return;
		if (!m_Controller || !GC_LobbyManager.GetManager() || !SCR_Global.IsAdmin(m_Controller.GetPlayerId()))
		{
			SetActive(false);
			return;
		}
		int now = System.GetTickCount();
		if (m_bPending && now - m_iRequestedAt < 3000)
			return;
		if (m_iRequest >= 0 && m_iReceivedAt && now - m_iReceivedAt > 10000)
			ClearValues("Waiting for server...");
		m_iRequestedAt = now;
		m_bPending = true;
		m_iRequest = m_Controller.ReserveAdminActionRequest();
		m_Controller.GC_RequestServerStatus(m_iRequest);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPopulationChanged(int playerId)
	{
		if (!m_bActive)
			return;
		// Coalesce bursts and let native role/roster state settle before the authoritative read.
		GetGame().GetCallqueue().Remove(RequestAfterPopulationChange);
		GetGame().GetCallqueue().CallLater(RequestAfterPopulationChange, 100);
	}

	//------------------------------------------------------------------------------------------------
	protected void RequestAfterPopulationChange()
	{
		Poll();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSample(int requestId, int players, int admins, int fps, int uptime, int activeAI)
	{
		if (!m_bActive || requestId != m_iRequest)
			return;
		m_bPending = false;
		m_iReceivedAt = System.GetTickCount();
		SetText("StatusPlayers", players.ToString());
		SetText("StatusAdmins", admins.ToString());
		SetText("StatusFPS", fps.ToString());
		int hours = uptime / 3600;
		int minutes = (uptime / 60) % 60;
		int seconds = uptime % 60;
		string duration = string.Format("%1m %2s", minutes, seconds);
		if (hours > 0)
			duration = string.Format("%1h %2m %3s", hours, minutes, seconds);
		SetText("StatusUptime", duration);
		string ai = "—";
		if (activeAI >= 0)
			ai = activeAI.ToString();
		SetText("StatusAI", ai);
		SetText("StatusFeedback", "");
		// Metrics refresh every second; history records only one sample per five seconds.
		if (!m_aFPS.IsEmpty() && m_iReceivedAt - m_iGraphSampleAt < 5000)
			return;
		m_iGraphSampleAt = m_iReceivedAt;
		if (m_aFPS.Count() >= 60)
			m_aFPS.RemoveOrdered(0);
		m_aFPS.Insert(fps);
		// Defer once so the first sample uses the newly visible panel's resolved size.
		GetGame().GetCallqueue().Remove(DrawGraph);
		GetGame().GetCallqueue().CallLater(DrawGraph, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void AddLine(float x1, float y1, float x2, float y2, int color, float width)
	{
		LineDrawCommand line = new LineDrawCommand();
		line.m_iColor = color;
		line.m_fWidth = width;
		line.m_bShouldEnclose = false;
		line.m_Vertices = {x1, y1, x2, y2};
		m_aCommands.Insert(line);
	}

	//------------------------------------------------------------------------------------------------
	protected void DrawGraph()
	{
		float width, height;
		m_Canvas.GetScreenSize(width, height);
		if (width <= 0 || height <= 0 || m_aFPS.IsEmpty())
			return;
		m_aCommands = {};
		int ceiling = 30;
		foreach (int value : m_aFPS)
			ceiling = Math.Max(ceiling, Math.Ceil(value * 1.15 / 2) * 2);
		SetText("StatusGraphMax", ceiling.ToString());
		SetText("StatusGraphMid", (ceiling / 2).ToString());
		for (int grid; grid <= 4; grid++)
			AddLine(0, grid * (height - 2) / 4, width, grid * (height - 2) / 4, ARGB(45, 255, 255, 255), 1);
		// Fill all segments before drawing their outline, as in the native canvas pattern.
		for (int i = 1; i < m_aFPS.Count(); i++)
		{
			AddAreaSegment(i, width, height, ceiling);
		}
		for (int point; point < m_aFPS.Count(); point++)
		{
			float x = point / 59.0 * (width - 4) + 2;
			float y = (1 - m_aFPS[point] / (float)ceiling) * (height - 4) + 2;
			if (point > 0)
			{
				float previousX = (point - 1) / 59.0 * (width - 4) + 2;
				float previousY = (1 - m_aFPS[point - 1] / (float)ceiling) * (height - 4) + 2;
				AddLine(previousX, previousY, x, y, ARGB(255, 242, 186, 77), 2);
			}
			else
				AddLine(x - 1, y, x + 1, y, ARGB(255, 242, 186, 77), 3);
		}
		m_Canvas.SetDrawCommands(m_aCommands);
	}

	//------------------------------------------------------------------------------------------------
	protected void AddAreaSegment(int index, float width, float height, float ceiling)
	{
		float x0 = (index - 1) / 59.0 * (width - 4) + 2;
		float x1 = index / 59.0 * (width - 4) + 2;
		float y0 = (1 - m_aFPS[index - 1] / ceiling) * (height - 4) + 2;
		float y1 = (1 - m_aFPS[index] / ceiling) * (height - 4) + 2;
		PolygonDrawCommand fill = new PolygonDrawCommand();
		fill.m_iColor = ARGB(100, 242, 186, 77);
		fill.m_Vertices = {x0, y0, x1, y1, x1, height - 2, x0, height - 2};
		m_aCommands.Insert(fill);
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		GetGame().GetCallqueue().Remove(Poll);
		GetGame().GetCallqueue().Remove(DrawGraph);
		GetGame().GetCallqueue().Remove(RequestAfterPopulationChange);
		if (m_Manager)
		{
			m_Manager.m_OnPlayerConnected.Remove(OnPopulationChanged);
			m_Manager.m_OnPlayerDisconnected.Remove(OnPopulationChanged);
			m_Manager.m_OnPlayerRoleChanged.Remove(OnPopulationChanged);
		}
		if (m_Controller)
			m_Controller.m_OnGC_ServerStatus.Remove(OnSample);
		super.HandlerDeattached(w);
	}
}
