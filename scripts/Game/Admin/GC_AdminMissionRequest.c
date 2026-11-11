//! One bounded server request per controller; callbacks recheck login and mode ownership.
class GC_AdminMissionRequest
{
	protected GC_LobbyPlayerController m_Controller;
	protected GC_LobbyManager m_Manager;
	protected RestContext m_Context;
	protected ref RestCallback m_Callback;
	protected bool m_bBusy;
	protected int m_iRequest;
	protected string m_sAction;
	protected string m_sMission;
	protected string m_sIdentity;
	protected int m_iPlayer;

	//------------------------------------------------------------------------------------------------
	bool HasAccess()
	{
		return Replication.IsServer() && m_Manager && GC_LobbyManager.GetManager() == m_Manager && m_Controller
			&& GetGame().GetPlayerManager().IsPlayerConnected(m_iPlayer)
			&& GetGame().GetPlayerManager().GetPlayerController(m_iPlayer) == m_Controller
			&& SCR_Global.IsAdmin(m_iPlayer);
	}

	//------------------------------------------------------------------------------------------------
	void Request(GC_LobbyPlayerController controller, string action, string query, int offset, string mission, string launchId, int requestId)
	{
		int player = controller.GetPlayerId();
		if (m_bBusy)
		{
			controller.GC_SendMissionPart(requestId, "", "Another mission request is pending.", true);
			return;
		}
		m_Controller = controller;
		m_Manager = GC_LobbyManager.GetManager();
		m_iPlayer = player;
		m_iRequest = requestId;
		m_sAction = action;
		m_sMission = mission;
		if (!HasAccess())
		{
			Fail("Rejected: administrator access required.");
			return;
		}
		if (query.Length() > 200 || offset < 0 || offset > 100000
			|| (action != "query" && action != "detail" && action != "launch")
			|| (action != "query" && !ValidId(mission)) || (action == "launch" && !ValidId(launchId)))
		{
			Fail("Invalid mission request.");
			return;
		}
		string status;
		m_Context = GC_MissionServerConnection.Open(status);
		if (!m_Context)
		{
			Fail(status);
			return;
		}
		m_sIdentity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(player);
		if (action == "launch" && m_sIdentity.IsEmpty())
		{
			Fail("Player identity unavailable.");
			return;
		}
		m_bBusy = true;
		m_Callback = new RestCallback();
		m_Callback.SetOnSuccess(OnSuccess);
		m_Callback.SetOnError(OnError);
		JsonSaveContext json = new JsonSaveContext();
		if (action == "query")
		{
			json.WriteValue("search", query);
			json.WriteValue("offset", offset);
			m_Context.POST(m_Callback, "api/v1/catalog/selection/query", json.SaveToString());
		}
		else if (action == "detail")
			m_Context.GET(m_Callback, "api/v1/catalog/selection/" + mission);
		else
		{
			json.WriteValue("requestId", launchId);
			json.WriteValue("missionId", mission);
			json.WriteValue("requesterIdentity", m_sIdentity);
			GC_AdminActionLogger.Log(player, "mission-launch", mission, "submitted-to-website");
			m_Context.POST(m_Callback, "api/v1/catalog/selection/launch", json.SaveToString());
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool ValidId(string value)
	{
		if (value.Length() != 36)
			return false;
		for (int i; i < value.Length(); i++)
		{
			if (!"0123456789abcdefABCDEF-".Contains(value.Substring(i, 1)))
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void Fail(string reason)
	{
		m_bBusy = false;
		GC_AdminActionLogger.Log(m_iPlayer, "mission-" + m_sAction, m_sMission, reason);
		if (m_Controller && GC_LobbyManager.GetManager() == m_Manager)
			m_Controller.GC_SendMissionPart(m_iRequest, "", reason, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnError(RestCallback callback)
	{
		Fail("Website request failed. Refresh before retrying a launch; its outcome may be unknown.");
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSuccess(RestCallback callback)
	{
		if (!HasAccess())
		{
			Fail("Result withheld: administrator access ended.");
			return;
		}
		JsonLoadContext json = new JsonLoadContext(false);
		array<string> chunks = {};
		int version;
		if (callback.GetData().Length() > 524288 || !json.LoadFromString(callback.GetData())
			|| !json.ReadValue("version", version) || version != 1 || !json.ReadValue("chunks", chunks) || chunks.Count() > 512)
		{
			Fail("Invalid mission response.");
			return;
		}
		foreach (string part : chunks)
		{
			if (part.Length() > 3600)
			{
				Fail("Mission response exceeds transport limit.");
				return;
			}
		}
		foreach (string chunk : chunks)
			m_Controller.GC_SendMissionPart(m_iRequest, chunk, "", false);
		m_Controller.GC_SendMissionPart(m_iRequest, "", "", true);
		m_bBusy = false;
		GC_AdminActionLogger.Log(m_iPlayer, "mission-" + m_sAction, m_sMission, "website-response-received");
	}
}
