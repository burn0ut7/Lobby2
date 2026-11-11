#ifdef WORKBENCH
modded class GC_AdminSearchFilter
{
	void GC_TestChoose(int index)
	{
		OpenList();
		if (index < 0 || index >= m_aElementWidgets.Count())
			return;
		Widget entry = m_aElementWidgets[index];
		SCR_ButtonBaseComponent.Cast(entry.FindHandler(SCR_ButtonBaseComponent)).OnClick(entry, 0, 0, 0);
	}
}

modded class GC_AdminMissionsComponent
{
	string GC_TestMissionBrowser(string query, bool edit)
	{
		if (edit)
		{
			m_Search.SetText(query);
			OnSearch(m_Search);
		}
		return string.Format("rows=%1 selected=%2 pending=%3 available=%4 status=%5", m_mRows.Count(), m_sSelected, m_iPending, m_bAvailable,
			TextWidget.Cast(m_wRoot.FindAnyWidget("MissionStatus")).GetText());
	}

	string GC_TestSelectMission(int index)
	{
		Widget row = m_wRoot.FindAnyWidget("MissionRows").GetChildren();
		for (int i; i < index && row; i++)
			row = row.GetSibling();
		if (!row)
			return "UNAVAILABLE row";
		float x, y, width, height;
		row.GetScreenPos(x, y);
		row.GetScreenSize(width, height);
		Widget root = GetGame().GetMenuManager().GetTopMenu().GetRootWidget();
		WidgetManager.ReportMouse(x + width * 0.5, y + height * 0.5, root);
		Widget hit = WidgetManager.GetWidgetUnderCursor();
		if (!hit)
			return "UNAVAILABLE hit";
		Widget ancestor = hit;
		while (ancestor && ancestor != row)
			ancestor = ancestor.GetParent();
		if (ancestor != row)
			return "BLOCKED hit=" + hit.GetName();
		SCR_ButtonBaseComponent.Cast(row.FindHandler(SCR_ButtonBaseComponent)).OnClick(row, x, y, 0);
		return "clicked=" + hit.GetName();
	}
}

modded class GC_LobbyManager
{
	protected string m_sGC_MissionTestResponse;

	string GC_TestMissionTransport(string action, string mission)
	{
		if (action == "report")
			return m_sGC_MissionTestResponse;
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		controller.m_GC_OnMissionResponse.Remove(GC_TestMissionResponse);
		controller.m_GC_OnMissionResponse.Insert(GC_TestMissionResponse);
		m_sGC_MissionTestResponse = "pending";
		controller.GC_RequestMissions(action, "", 0, mission, PersistenceIdUtils.Generate(), controller.ReserveAdminPlayerToolRequest());
		return m_sGC_MissionTestResponse;
	}

	protected void GC_TestMissionResponse(int requestId, string payload, string error)
	{
		m_sGC_MissionTestResponse = error + " " + payload;
		GC_LobbyPlayerController.Cast(GetGame().GetPlayerController()).m_GC_OnMissionResponse.Remove(GC_TestMissionResponse);
	}

	string GC_TestMissionBrowser(string query, bool edit)
	{
		Widget root = GetGame().GetMenuManager().GetTopMenu().GetRootWidget();
		return GC_AdminMissionsComponent.Cast(root.FindAnyWidget("MissionsPanel").FindHandler(GC_AdminMissionsComponent)).GC_TestMissionBrowser(query, edit);
	}

	string GC_TestSelectMission(int index)
	{
		Widget root = GetGame().GetMenuManager().GetTopMenu().GetRootWidget();
		return GC_AdminMissionsComponent.Cast(root.FindAnyWidget("MissionsPanel").FindHandler(GC_AdminMissionsComponent)).GC_TestSelectMission(index);
	}
}
#endif
