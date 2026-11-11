//! Server-authored rows identify native entities; display names are never command targets.
class GC_BoundaryAdminRow
{
	string Key;
	string Name;
	string Kind;
	string Factions;
	string Behavior;
	string State;
	string Details;
	float Remaining;
	bool Enabled;
	bool Valid;
}

//! Mission-owned controls for zones and one countdown, optionally carrying a pending mission result.
class GC_BoundaryAdmin
{
	protected GC_LobbyGameMode m_Mode;
	protected ref array<IEntity> m_aZones = {};
	protected ref map<IEntity, int> m_mDisplayNumbers = new map<IEntity, int>();
	protected int m_iNextDisplayNumber;
	protected string m_sAnnouncement;
	protected ref array<string> m_aAnnouncementFactions = {};
	protected float m_fAnnouncementDeadline;
	protected bool m_bAnnouncing;
	protected ref SCR_GameModeEndData m_PendingEndData;
	protected string m_sPendingEndMessage;
	protected int m_iEndingRevision;

	//------------------------------------------------------------------------------------------------
	void Start(GC_LobbyGameMode mode)
	{
		m_Mode = mode;
		GetGame().GetCallqueue().CallLater(UpdateAnnouncement, 500, true);
	}

	//------------------------------------------------------------------------------------------------
	void Stop()
	{
		GetGame().GetCallqueue().Remove(UpdateAnnouncement);
		ClearAnnouncement();
		m_aZones.Clear();
		m_mDisplayNumbers.Clear();
		m_Mode = null;
	}

	//------------------------------------------------------------------------------------------------
	bool StartMissionEnding(SCR_GameModeEndData data, string message)
	{
		if (!Replication.IsServer() || !m_Mode || GC_LobbyGameMode.GetGameMode() != m_Mode
			|| m_Mode.GetState() != SCR_EGameModeState.GAME || m_PendingEndData)
			return false;
		m_PendingEndData = data;
		if (!m_PendingEndData)
			m_PendingEndData = SCR_GameModeEndData.CreateSimple();
		m_sPendingEndMessage = message;
		m_iEndingRevision++;
		m_sAnnouncement = "Mission ending in";
		m_aAnnouncementFactions.Clear();
		m_fAnnouncementDeadline = GetGame().GetWorld().GetWorldTime() * 0.001 + 120;
		UpdateAnnouncement();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Trusted server completion. Admin callers must authorize before reaching this method.
	bool ForceMissionEnd()
	{
		if (!Replication.IsServer() || !m_Mode || GC_LobbyGameMode.GetGameMode() != m_Mode
			|| m_Mode.GetState() != SCR_EGameModeState.GAME)
			return false;
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!manager)
			return false;
		// Retain the pending result through synchronous phase callbacks and collection finalization.
		ref SCR_GameModeEndData data = m_PendingEndData;
		string message = m_sPendingEndMessage;
		ClearAnnouncement();
		bool ended = manager.CompletePostGame(data, message);
		UpdateAnnouncement();
		return ended;
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearAnnouncement()
	{
		m_fAnnouncementDeadline = 0;
		m_sAnnouncement = string.Empty;
		m_aAnnouncementFactions.Clear();
		m_PendingEndData = null;
		m_sPendingEndMessage = string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	void Register(IEntity zone)
	{
		if (Replication.IsServer() && zone && !m_aZones.Contains(zone))
		{
			m_aZones.Insert(zone);
			m_mDisplayNumbers.Set(zone, ++m_iNextDisplayNumber);
			m_Mode.GetMapShapes().Changed();
		}
	}

	//------------------------------------------------------------------------------------------------
	void Unregister(IEntity zone)
	{
		m_aZones.RemoveItem(zone);
		m_mDisplayNumbers.Remove(zone);
		if (m_Mode && m_Mode.GetMapShapes())
			m_Mode.GetMapShapes().Changed();
	}

	//------------------------------------------------------------------------------------------------
	protected bool HasAccess(int playerId)
	{
		return Replication.IsServer() && m_Mode && GC_LobbyGameMode.GetGameMode() == m_Mode
			&& playerId > 0 && GetGame().GetPlayerManager().IsPlayerConnected(playerId) && SCR_Global.IsAdmin(playerId);
	}

	//------------------------------------------------------------------------------------------------
	static string FactionText(array<string> keys)
	{
		if (!keys || keys.IsEmpty())
			return "All factions";
		string text;
		foreach (string key : keys)
		{
			if (!text.IsEmpty())
				text += ", ";
			Faction faction = GetGame().GetFactionManager().GetFactionByKey(key);
			if (faction)
				text += WidgetManager.Translate(faction.GetFactionName()) + " (" + key + ")";
			else
				text += key;
		}
		return text;
	}

	//------------------------------------------------------------------------------------------------
	protected GC_BoundaryAdminRow Describe(IEntity zone)
	{
		GC_BoundaryAdminRow row = new GC_BoundaryAdminRow();
		row.Key = zone.GetID().ToString();
		GC_AOLimitEntity ao = GC_AOLimitEntity.Cast(zone);
		GC_SetupZoneEntity setup = GC_SetupZoneEntity.Cast(zone);
		if (ao)
			ao.GetAdminInfo(row);
		else if (setup)
			setup.GetAdminInfo(row);
		if (row.Name.IsEmpty())
			row.Name = zone.GetName();
		if (row.Name.IsEmpty())
			row.Name = row.Kind + " " + m_mDisplayNumbers.Get(zone).ToString();
		return row;
	}

	//------------------------------------------------------------------------------------------------
	string Snapshot(int playerId)
	{
		if (!HasAccess(playerId))
			return string.Empty;
		array<ref GC_BoundaryAdminRow> rows = {};
		foreach (IEntity zone : m_aZones)
		{
			if (zone)
				rows.Insert(Describe(zone));
		}
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		JsonSaveContext json = new JsonSaveContext();
		json.WriteValue("zones", rows);
		bool showMap = controller && controller.GC_ShowBoundaryMap();
		json.WriteValue("showMap", showMap);
		json.WriteValue("announcement", m_sAnnouncement);
		json.WriteValue("remaining", Math.Max(0, m_fAnnouncementDeadline - GetGame().GetWorld().GetWorldTime() * 0.001));
		json.WriteValue("factions", FactionText(m_aAnnouncementFactions));
		bool ending = m_PendingEndData != null;
		json.WriteValue("ending", ending);
		json.WriteValue("endingRevision", m_iEndingRevision);
		return json.SaveToString();
	}

	//------------------------------------------------------------------------------------------------
	//! The only admin mutation entry point. Authority rechecks the authenticated requester.
	string Execute(int playerId, string action, string key, string title, int seconds, array<string> requestedFactions)
	{
		array<string> factions = {};
		if (requestedFactions)
			factions.Copy(requestedFactions);
		string result;
		if (!HasAccess(playerId))
			result = "Rejected: administrator access required.";
		else if (action == "status")
			return string.Empty;
		else if (action == "panic")
		{
			int disabled, ended;
			foreach (IEntity zone : m_aZones)
			{
				GC_AOLimitEntity ao = GC_AOLimitEntity.Cast(zone);
				GC_SetupZoneEntity setup = GC_SetupZoneEntity.Cast(zone);
				if (ao && ao.SetEnabled(false))
					disabled++;
				if (setup && setup.EndSetup())
					ended++;
			}
			ClearAnnouncement();
			UpdateAnnouncement();
			result = string.Format("Stopped all boundaries: %1 AOs disabled, %2 setups ended.", disabled, ended);
		}
		else if (action == "map-on" || action == "map-off")
		{
			GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			controller.GC_SetBoundaryMap(action == "map-on");
			result = "Map visibility updated for you only.";
		}
		else if (action.StartsWith("ending-"))
		{
			if (!m_PendingEndData || key != m_iEndingRevision.ToString() || m_Mode.GetState() != SCR_EGameModeState.GAME)
				result = "Not applied: the pending ending changed. Refresh the timer.";
			else if (action == "ending-cancel")
			{
				ClearAnnouncement();
				UpdateAnnouncement();
				result = "Mission ending cancelled. The mission remains active.";
			}
			else if (action == "ending-force")
			{
				if (ForceMissionEnd())
					result = "Mission ended.";
				else
					result = "Mission could not end; the countdown was cancelled.";
			}
			else if ((action != "ending-set" && action != "ending-extend") || seconds < 1 || seconds > 3600)
				result = "Use 1-3600 seconds to set or extend the ending timer.";
			else
			{
				float now = GetGame().GetWorld().GetWorldTime() * 0.001;
				float remaining = Math.Max(0, m_fAnnouncementDeadline - now);
				if (action == "ending-extend" && remaining + seconds > 3600)
					result = "Not applied: the ending timer cannot exceed 60 minutes.";
				else
				{
					m_fAnnouncementDeadline = now + seconds;
					if (action == "ending-extend")
						m_fAnnouncementDeadline += remaining;
					UpdateAnnouncement();
					result = "Mission ending timer updated.";
				}
			}
		}
		else if (m_PendingEndData && (action == "announce-stop" || action == "announce-start"))
			result = "A mission ending is pending. Use its Set time, Extend, Cancel ending or End now controls.";
		else if (action == "announce-stop")
		{
			ClearAnnouncement();
			UpdateAnnouncement();
			result = "Announcement timer stopped.";
		}
		else if (action == "announce-start")
		{
			title.TrimInPlace();
			bool valid = m_Mode.GetState() == SCR_EGameModeState.GAME && seconds >= 1 && seconds <= 3600
				&& !title.IsEmpty() && title.Length() <= 32 && !title.Contains("\n") && !title.Contains("\r")
				&& !title.Contains("<") && !title.Contains(">");
			foreach (string factionKey : factions)
				valid = valid && !factionKey.IsEmpty() && GetGame().GetFactionManager().GetFactionByKey(factionKey) != null;
			if (!valid)
				result = "Use 1-3600 seconds, a short heading (1-32 characters), and current factions during GAME.";
			else
			{
				m_sAnnouncement = title;
				m_aAnnouncementFactions.Copy(factions);
				m_fAnnouncementDeadline = GetGame().GetWorld().GetWorldTime() * 0.001 + seconds;
				UpdateAnnouncement();
				result = "Announcement started for " + FactionText(factions) + ".";
			}
		}
		else if (action == "extend" && (seconds < 1 || seconds > 3600))
			result = "Not applied: add between 1 and 3600 seconds.";
		else
		{
			IEntity target;
			foreach (IEntity candidate : m_aZones)
			{
				if (candidate && candidate.GetID().ToString() == key)
					target = candidate;
			}
			GC_AOLimitEntity ao = GC_AOLimitEntity.Cast(target);
			GC_SetupZoneEntity setup = GC_SetupZoneEntity.Cast(target);
			bool applied;
			if (ao && (action == "enable" || action == "disable"))
				applied = ao.SetEnabled(action == "enable");
			else if (setup && action == "end")
				applied = setup.EndSetup();
			else if (setup && action == "extend")
				applied = setup.ExtendSetup(seconds);
			if (applied)
			{
				result = Describe(target).Name + ": " + action + " applied.";
				if (action == "extend")
					result = Describe(target).Name + ": added " + seconds.ToString() + " seconds.";
			}
			else
				result = "Not applied: zone removed, invalid, ended, or action unavailable. Refresh the list.";
		}
		string context = key + " " + title + " " + FactionText(factions);
		if (action == "extend" || action == "ending-extend")
			context = key + " add " + seconds.ToString() + " seconds";
		else if (action == "ending-set")
			context = key + " set " + seconds.ToString() + " seconds";
		GC_AdminActionLogger.Log(playerId, "boundary-" + action, context, result);
		return result;
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateAnnouncement()
	{
		if (!Replication.IsServer() || !m_Mode || GC_LobbyGameMode.GetGameMode() != m_Mode)
			return;
		float seconds = Math.Max(0, m_fAnnouncementDeadline - GetGame().GetWorld().GetWorldTime() * 0.001);
		if (m_PendingEndData && m_Mode.GetState() == SCR_EGameModeState.GAME && seconds <= 0)
		{
			ForceMissionEnd();
			return;
		}
		if (m_Mode.GetState() != SCR_EGameModeState.GAME)
			seconds = 0;
		if (seconds <= 0)
		{
			ClearAnnouncement();
		}
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		foreach (int playerId : players)
		{
			GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			if (!controller)
				continue;
			if (!SCR_Global.IsAdmin(playerId))
				controller.GC_SetBoundaryMap(false);
			if (seconds <= 0 && !m_bAnnouncing)
				continue;
			Faction faction;
			if (manager)
				faction = manager.GetPlayerFaction(playerId);
			bool matches = m_aAnnouncementFactions.IsEmpty() || (faction && m_aAnnouncementFactions.Contains(faction.GetFactionKey()));
			float remaining;
			if (matches)
				remaining = seconds;
			controller.GC_SendAnnouncement(m_sAnnouncement, remaining);
		}
		m_bAnnouncing = seconds > 0;
	}

	//------------------------------------------------------------------------------------------------
	void AppendMap(int playerId, array<int> counts, array<int> colors, array<int> fills, array<bool> inverted, array<float> widths, array<vector> points, array<string> names)
	{
		if (!HasAccess(playerId))
			return;
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!controller || !controller.GC_ShowBoundaryMap())
			return;
		foreach (IEntity zone : m_aZones)
		{
			if (!zone)
				continue;
			array<vector> polygon = {};
			GC_AOLimitEntity ao = GC_AOLimitEntity.Cast(zone);
			GC_SetupZoneEntity setup = GC_SetupZoneEntity.Cast(zone);
			if (ao)
				ao.GetMapPolygon(polygon);
			else if (setup)
				setup.GetBoundaryPolygon(polygon);
			if (polygon.Count() < 3)
				continue;
			GC_BoundaryAdminRow row = Describe(zone);
			int color = 0xFFFFB347;
			if (ao)
				color = 0xFF60BFFF;
			if (!row.Enabled)
				color = 0xFF999999;
			counts.Insert(polygon.Count());
			colors.Insert(color);
			fills.Insert(0);
			inverted.Insert(false);
			widths.Insert(3);
			points.InsertAll(polygon);
			string label = row.Name;
			if (!label.StartsWith(row.Kind + " "))
				label = row.Kind + ": " + label;
			names.Insert(label + " (" + row.State + ")");
		}
	}
}
