//! Server catalogue browser. Selection is local; loading creates an authenticated website request.
class GC_AdminMissionsComponent : SCR_ScriptedWidgetComponent
{
	protected GC_LobbyPlayerController m_Controller;
	protected EditBoxWidget m_Search;
	protected SCR_EventHandlerComponent m_SearchEvents;
	protected ref map<SCR_ButtonBaseComponent, string> m_mRows = new map<SCR_ButtonBaseComponent, string>();
	protected string m_sSelected;
	protected string m_sLaunchId;
	protected string m_sAction;
	protected int m_iPending = -1;
	protected int m_iOffset;
	protected int m_iNext = -1;
	protected bool m_bActive;
	protected bool m_bAvailable;
	protected bool m_bSearchChanged;

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_Controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		m_Search = EditBoxWidget.Cast(w.FindAnyWidget("MissionSearch"));
		m_SearchEvents = SCR_EventHandlerComponent.Cast(m_Search.FindHandler(SCR_EventHandlerComponent));
		m_SearchEvents.GetOnChange().Insert(OnSearch);
		Bind("MissionPrevious", "Previous");
		Bind("MissionNext", "Next");
		Bind("MissionRefresh", "Refresh");
		Bind("MissionLoad", "Load mission");
		if (m_Controller)
			m_Controller.m_GC_OnMissionResponse.Insert(OnResponse);
		RefreshButtons();
	}

	//------------------------------------------------------------------------------------------------
	protected void Bind(string name, string label)
	{
		SCR_ButtonTextComponent button = SCR_ButtonTextComponent.GetButtonText(name, m_wRoot);
		button.SetText(label);
		button.m_OnClicked.Insert(OnAction);
	}

	//------------------------------------------------------------------------------------------------
	protected bool HasAccess()
	{
		return m_Controller && GC_LobbyManager.GetManager() && SCR_Global.IsAdmin(m_Controller.GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	void SetActive(bool active)
	{
		bool opening = active && !m_bActive;
		m_bActive = active;
		m_wRoot.SetVisible(active);
		if (opening)
			LoadPage();
	}

	//------------------------------------------------------------------------------------------------
	protected void Status(string value)
	{
		TextWidget.Cast(m_wRoot.FindAnyWidget("MissionStatus")).SetText(value);
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshButtons()
	{
		bool enabled = HasAccess() && m_iPending < 0;
		SCR_ButtonTextComponent.GetButtonText("MissionLoad", m_wRoot).SetEnabled(enabled && m_bAvailable && !m_sSelected.IsEmpty());
		SCR_ButtonTextComponent.GetButtonText("MissionPrevious", m_wRoot).SetEnabled(enabled && m_iOffset > 0);
		SCR_ButtonTextComponent.GetButtonText("MissionNext", m_wRoot).SetEnabled(enabled && m_iNext >= 0);
		SCR_ButtonTextComponent.GetButtonText("MissionRefresh", m_wRoot).SetEnabled(enabled);
	}

	//------------------------------------------------------------------------------------------------
	protected void Request(string action)
	{
		if (!HasAccess() || m_iPending >= 0)
			return;
		m_sAction = action;
		m_iPending = m_Controller.ReserveAdminPlayerToolRequest();
		Status("Waiting for server...");
		RefreshButtons();
		m_Controller.GC_RequestMissions(action, m_Search.GetText(), m_iOffset, m_sSelected, m_sLaunchId, m_iPending);
	}

	//------------------------------------------------------------------------------------------------
	protected void LoadPage()
	{
		if (!m_bActive || !HasAccess() || m_iPending >= 0)
			return;
		m_bSearchChanged = false;
		m_sSelected = "";
		m_sLaunchId = "";
		m_bAvailable = false;
		TextWidget.Cast(m_wRoot.FindAnyWidget("MissionPreviewName")).SetText("Select a mission");
		TextWidget.Cast(m_wRoot.FindAnyWidget("MissionDescription")).SetText("");
		TextWidget.Cast(m_wRoot.FindAnyWidget("MissionAddons")).SetText("");
		m_wRoot.FindAnyWidget("MissionThumbnail").SetVisible(false);
		Request("query");
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSearch(Widget widget)
	{
		m_iOffset = 0;
		m_bSearchChanged = true;
		GetGame().GetCallqueue().Remove(LoadPage);
		GetGame().GetCallqueue().CallLater(LoadPage, 300);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnAction(SCR_ButtonBaseComponent button)
	{
		if (!HasAccess() || m_iPending >= 0)
			return;
		string name = button.GetRootWidget().GetName();
		if (name == "MissionLoad")
		{
			if (m_bAvailable && !m_sSelected.IsEmpty())
			{
				if (m_sLaunchId.IsEmpty())
					m_sLaunchId = PersistenceIdUtils.Generate();
				Request("launch");
			}
			return;
		}
		if (name == "MissionPrevious")
			m_iOffset = Math.Max(0, m_iOffset - 25);
		else if (name == "MissionNext" && m_iNext >= 0)
			m_iOffset = m_iNext;
		LoadPage();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRow(SCR_ButtonBaseComponent button)
	{
		if (!HasAccess() || m_iPending >= 0 || !m_mRows.Find(button, m_sSelected))
			return;
		m_sLaunchId = "";
		m_bAvailable = false;
		foreach (SCR_ButtonBaseComponent row, string id : m_mRows)
			row.SetToggled(id == m_sSelected, false, false);
		Request("detail");
	}

	//------------------------------------------------------------------------------------------------
	protected void OnResponse(int requestId, string payload, string error)
	{
		if (requestId != m_iPending)
			return;
		m_iPending = -1;
		if (!HasAccess())
			return;
		JsonLoadContext json = new JsonLoadContext(false);
		if (!error.IsEmpty())
			Status(error);
		else if (!json.LoadFromString(payload))
			Status("Invalid mission response.");
		else if (m_sAction == "query")
		{
			array<string> rows = {};
			json.ReadValue("rows", rows);
			json.ReadValue("nextOffset", m_iNext);
			RenderRows(rows);
			Status(rows.Count().ToString() + " missions shown");
		}
		else if (m_sAction == "detail")
		{
			string id;
			json.ReadValue("id", id);
			if (id == m_sSelected)
				RenderDetail(json);
		}
		else
		{
			string state;
			json.ReadValue("status", state);
			if (state == "queued")
				Status("Launch request queued. Restart execution will be added in a future update.");
			else
				Status("Launch request: " + state);
		}
		RefreshButtons();
		if (m_bSearchChanged)
			GetGame().GetCallqueue().CallLater(LoadPage, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void RenderRows(array<string> rows)
	{
		Widget root = m_wRoot.FindAnyWidget("MissionRows");
		m_mRows.Clear();
		SCR_WidgetHelper.RemoveAllChildren(root);
		foreach (string value : rows)
		{
			JsonLoadContext json = new JsonLoadContext(false);
			if (!json.LoadFromString(value))
				continue;
			string id, name, tags;
			int minimum, maximum;
			bool available;
			json.ReadValue("id", id);
			json.ReadValue("name", name);
			json.ReadValue("tags", tags);
			json.ReadValue("minPlayers", minimum);
			json.ReadValue("maxPlayers", maximum);
			json.ReadValue("available", available);
			Widget row = GetGame().GetWorkspace().CreateWidgets("{E37A21004D960003}UI/layouts/Admin/Lobby_AdminMissionRow.layout", root);
			SCR_ButtonBaseComponent button = SCR_ButtonBaseComponent.Cast(row.FindHandler(SCR_ButtonBaseComponent));
			button.m_OnClicked.Insert(OnRow);
			m_mRows.Set(button, id);
			TextWidget.Cast(row.FindAnyWidget("Cell1")).SetText(name);
			string range = "Unknown";
			if (minimum > 0 && maximum >= minimum)
				range = maximum.ToString() + "-" + minimum.ToString();
			TextWidget.Cast(row.FindAnyWidget("Cell2")).SetText(range);
			TextWidget.Cast(row.FindAnyWidget("Cell3")).SetText(tags);
			string state = "Available";
			if (!available)
				state = "Unavailable";
			TextWidget.Cast(row.FindAnyWidget("Cell4")).SetText(state);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void RenderDetail(JsonLoadContext json)
	{
		string name, description, tags, thumbnail, addons;
		json.ReadValue("name", name);
		json.ReadValue("description", description);
		json.ReadValue("tags", tags);
		json.ReadValue("thumbnail", thumbnail);
		json.ReadValue("addons", addons);
		json.ReadValue("available", m_bAvailable);
		TextWidget.Cast(m_wRoot.FindAnyWidget("MissionPreviewName")).SetText(name);
		TextWidget.Cast(m_wRoot.FindAnyWidget("MissionDescription")).SetText(description + "\n\n" + tags);
		string addonText = "Required add-ons not declared";
		if (!addons.IsEmpty())
			addonText = "Declared add-ons: " + addons;
		TextWidget.Cast(m_wRoot.FindAnyWidget("MissionAddons")).SetText(addonText);
		ImageWidget preview = ImageWidget.Cast(m_wRoot.FindAnyWidget("MissionThumbnail"));
		bool imageLoaded = !thumbnail.IsEmpty() && preview.LoadImageTexture(0, thumbnail);
		if (imageLoaded)
			SCR_WidgetHelper.ResizeToImage(preview);
		preview.SetVisible(imageLoaded);
		m_wRoot.FindAnyWidget("MissionNoThumbnail").SetVisible(!imageLoaded);
		GetGame().GetCallqueue().CallLater(ResizeDescription, 0);
		Status("");
	}

	//------------------------------------------------------------------------------------------------
	protected void ResizeDescription()
	{
		m_wRoot.Update();
		float width, height;
		TextWidget.Cast(m_wRoot.FindAnyWidget("MissionDescription")).GetTextSize(width, height);
		SizeLayoutWidget.Cast(m_wRoot.FindAnyWidget("MissionDescriptionSize")).SetHeightOverride(height + 8);
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		GetGame().GetCallqueue().Remove(LoadPage);
		GetGame().GetCallqueue().Remove(ResizeDescription);
		if (m_SearchEvents)
			m_SearchEvents.GetOnChange().Remove(OnSearch);
		if (m_Controller)
			m_Controller.m_GC_OnMissionResponse.Remove(OnResponse);
		super.HandlerDeattached(w);
	}
}
