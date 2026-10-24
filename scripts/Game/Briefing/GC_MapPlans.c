//! Server-owned recovery and admin operations for the shared mission map.
class GC_MapPlans
{
	protected GC_LobbyGameMode m_Mode;
	protected ref GC_MapPlanDrawings m_Drawings = new GC_MapPlanDrawings();
	protected bool m_bReady;
	protected bool m_bReplacing;
	protected bool m_bRecoveryBlocked;
	protected int m_iGeneration;
	protected string m_sRecoveryStatus = "Recovery has not started.";
	protected static bool s_bSameLastMission;
	static const string LAST_MISSION_FILE = "$profile:Lobby2LastMission.txt";
	static const string RECOVERY_FILE = "$profile:Lobby2MapRecovery";
	static const int RECOVERY_SECONDS = 7200;

	//------------------------------------------------------------------------------------------------
	//! Only eligibility is global: unrelated missions invalidate Lobby2 recovery
	//! without changing their gameplay or overwriting a recoverable map snapshot.
	static void NoteMission()
	{
		string previous;
		string world = GetGame().GetWorldFile();
		s_bSameLastMission = GC_MapPlanData.ReadFile(LAST_MISSION_FILE, previous) && previous == world;
		if (!GC_MapPlanData.WriteFile(LAST_MISSION_FILE, world))
		{
			s_bSameLastMission = false;
			Print("GC | Map recovery | Could not record last mission; automatic recovery disabled.", LogLevel.ERROR);
		}
	}

	//------------------------------------------------------------------------------------------------
	static GC_MapPlans GetInstance()
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!mode || !Replication.IsServer())
			return null;
		return mode.GetMapPlans();
	}

	//------------------------------------------------------------------------------------------------
	void Start(GC_LobbyGameMode mode)
	{
		m_Mode = mode;
		GetGame().GetCallqueue().CallLater(Initialize, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsActive()
	{
		return Replication.IsServer() && m_Mode && m_Mode == GC_LobbyGameMode.GetGameMode();
	}

	//------------------------------------------------------------------------------------------------
	protected void Initialize()
	{
		if (!IsActive())
			return;
		GC_MapPlanData saved;
		// Alternating complete snapshots preserve the last committed save when a
		// process stops during a write. Both slots belong to one recovery journal.
		for (int slot; slot < 2; slot++)
		{
			string input;
			GC_MapPlanData candidate = new GC_MapPlanData();
			if (!GC_MapPlanData.ReadFile(RECOVERY_FILE + slot.ToString() + ".json", input) || !candidate.Decode(input))
				continue;
			if (!saved || candidate.m_iGeneration > saved.m_iGeneration)
				saved = candidate;
		}
		if (saved)
			m_iGeneration = saved.m_iGeneration;
		int now = System.GetUnixTime();
		if (saved && s_bSameLastMission
			&& saved.m_sWorld == GetGame().GetWorldFile()
			&& now >= saved.m_iLastActive && now - saved.m_iLastActive <= RECOVERY_SECONDS)
		{
			string error = Validate(saved);
			if (error.IsEmpty() && !GC_LobbyPhase.ArePlayerMapMarkersLocked())
				Replace(saved);
			else
			{
				m_bRecoveryBlocked = true;
				m_sRecoveryStatus = "Recovery paused; saved plan preserved. " + error;
				Print("GC | Map recovery | " + m_sRecoveryStatus, LogLevel.WARNING);
			}
		}
		m_bReady = true;
		SaveRecovery();
		// Keep last-active time fresh even when nobody changes the map.
		GetGame().GetCallqueue().CallLater(SaveRecovery, 30000, true);
	}

	//------------------------------------------------------------------------------------------------
	void Changed()
	{
		if (!IsActive() || !m_bReady || m_bReplacing)
			return;
		GetGame().GetCallqueue().Remove(SaveChanged);
		GetGame().GetCallqueue().CallLater(SaveChanged, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void SaveChanged()
	{
		SaveRecovery();
	}

	//------------------------------------------------------------------------------------------------
	protected GC_MapPlanData Capture()
	{
		GC_MapPlanData data = new GC_MapPlanData();
		data.m_sWorld = GetGame().GetWorldFile();
		data.m_iLastActive = System.GetUnixTime();
		FactionManager factions = GetGame().GetFactionManager();
		for (int index; factions && index < factions.GetFactionsCount(); index++)
			data.m_aFactions.Insert(factions.GetFactionByIndex(index).GetFactionKey());
		SCR_MapMarkerManagerComponent manager = SCR_MapMarkerManagerComponent.GetInstance();
		array<SCR_MapMarkerBase> markers = manager.GetStaticMarkers();
		markers.InsertAll(manager.GetDisabledMarkers());
		foreach (SCR_MapMarkerBase marker : markers)
		{
			if (marker.GetMarkerID() < 0 || (marker.GetType() != SCR_EMapMarkerType.PLACED_CUSTOM
				&& marker.GetType() != SCR_EMapMarkerType.PLACED_MILITARY))
				continue;
			int position[2];
			marker.GetWorldPos(position);
			array<int> row = {marker.GetType(), position[0], position[1], marker.GetMarkerConfigID(),
				marker.GetMarkerFactionFlags(), marker.GetColorEntry(), marker.GetIconEntry(),
				marker.GetRotation(), marker.GetFlags()};
			data.m_aMarkers.Insert(row);
			data.m_aTexts.Insert(marker.GetCustomText());
		}
		m_Drawings.Capture(data.m_aDrawings);
		return data;
	}

	//------------------------------------------------------------------------------------------------
	protected string Validate(GC_MapPlanData data)
	{
		if (!m_Drawings.IsAvailable())
			return "Load the Lobby2 Map Drawing Plans addon to save or load drawings.";
		if (data.m_sWorld != GetGame().GetWorldFile())
			return "This plan belongs to a different mission.";
		if (!data.m_aMarkers || !data.m_aTexts || !data.m_aFactions || !data.m_aDrawings
			|| data.m_aMarkers.Count() != data.m_aTexts.Count() || data.m_aMarkers.Count() > 512)
			return "The plan contains invalid marker data.";
		FactionManager factions = GetGame().GetFactionManager();
		if (!factions || data.m_aFactions.Count() != factions.GetFactionsCount())
			return "The mission factions do not match this plan.";
		for (int index; index < data.m_aFactions.Count(); index++)
		{
			if (data.m_aFactions[index] != factions.GetFactionByIndex(index).GetFactionKey())
				return "The mission faction order does not match this plan.";
		}
		SCR_MapMarkerManagerComponent manager = SCR_MapMarkerManagerComponent.GetInstance();
		if (!manager || !manager.GetMarkerConfig())
			return "The mission map is unavailable.";
		SCR_MapMarkerConfig config = manager.GetMarkerConfig();
		foreach (int index, array<int> row : data.m_aMarkers)
		{
			if (!row || row.Count() != 9 || data.m_aTexts[index].Length() > 256
				|| row[1] < -1000000 || row[1] > 1000000 || row[2] < -1000000 || row[2] > 1000000)
				return "The plan contains invalid marker data.";
			if (row[0] == SCR_EMapMarkerType.PLACED_CUSTOM)
			{
				SCR_MapMarkerEntryPlaced custom = SCR_MapMarkerEntryPlaced.Cast(config.GetMarkerEntryConfigByType(row[0]));
				if (!custom || !custom.GetColorEntries().IsIndexValid(row[5]) || !custom.GetIconEntries().IsIndexValid(row[6]))
					return "A saved marker style is unavailable.";
			}
			else if (row[0] == SCR_EMapMarkerType.PLACED_MILITARY)
			{
				SCR_MapMarkerEntryMilitary military = SCR_MapMarkerEntryMilitary.Cast(config.GetMarkerEntryConfigByType(row[0]));
				if (!military || row[3] < 0 || !military.GetFactionEntry(row[3] % 100)
					|| !military.GetDimensionEntry(row[3] / 100))
					return "A saved military marker style is unavailable.";
			}
			else
				return "This plan contains an unsupported marker type.";
		}
		if (!m_Drawings.Validate(data.m_aDrawings))
			return "Drawing data is invalid or MapDrawing support is unavailable.";
		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	protected void Replace(GC_MapPlanData data)
	{
		m_bReplacing = true;
		SCR_MapMarkerManagerComponent manager = SCR_MapMarkerManagerComponent.GetInstance();
		array<SCR_MapMarkerBase> markers = manager.GetStaticMarkers();
		markers.InsertAll(manager.GetDisabledMarkers());
		foreach (SCR_MapMarkerBase marker : markers)
		{
			if (marker.GetMarkerID() < 0 || (marker.GetType() != SCR_EMapMarkerType.PLACED_CUSTOM
				&& marker.GetType() != SCR_EMapMarkerType.PLACED_MILITARY))
				continue;
			PlayerController owner = GetGame().GetPlayerManager().GetPlayerController(marker.GetMarkerOwnerID());
			if (owner)
			{
				SCR_MapMarkerSyncComponent sync = SCR_MapMarkerSyncComponent.Cast(owner.FindComponent(SCR_MapMarkerSyncComponent));
				if (sync)
					sync.RemovePlayerMarker(marker.GetMarkerID());
			}
			manager.OnRemoveSynchedMarker(marker.GetMarkerID());
			manager.OnAskRemoveStaticMarker(marker.GetMarkerID());
		}
		foreach (int index, array<int> row : data.m_aMarkers)
		{
			SCR_MapMarkerBase marker = new SCR_MapMarkerBase();
			marker.SetType(row[0]);
			marker.SetWorldPos(row[1], row[2]);
			marker.SetMarkerConfigID(row[3]);
			marker.SetMarkerFactionFlags(row[4]);
			marker.SetColorEntry(row[5]);
			marker.SetIconEntry(row[6]);
			marker.SetRotation(row[7]);
			marker.SetFlags(row[8]);
			marker.SetCustomText(data.m_aTexts[index]);
			manager.InsertStaticMarker(marker, false, true);
		}
		m_Drawings.Replace(data.m_aDrawings);
		m_bReplacing = false;
	}

	//------------------------------------------------------------------------------------------------
	protected void SaveRecovery()
	{
		if (!IsActive() || !m_bReady || m_bReplacing || m_bRecoveryBlocked)
			return;
		GC_MapPlanData data = Capture();
		string error = Validate(data);
		if (!error.IsEmpty())
		{
			m_sRecoveryStatus = error;
			Print("GC | Map recovery | " + error, LogLevel.ERROR);
			return;
		}
		data.m_iGeneration = m_iGeneration + 1;
		int slot = data.m_iGeneration % 2;
		if (GC_MapPlanData.WriteFile(RECOVERY_FILE + slot.ToString() + ".json", data.Encode()))
		{
			m_iGeneration = data.m_iGeneration;
			m_sRecoveryStatus = "Recovery saved.";
		}
		else
		{
			m_sRecoveryStatus = "Recovery could not be saved. Check server storage and plan size.";
			Print("GC | Map recovery | " + m_sRecoveryStatus, LogLevel.ERROR);
		}
	}

	//------------------------------------------------------------------------------------------------
	string Execute(int playerId, string action, string input, out string output)
	{
		string result;
		if (!IsActive() || !GetGame().GetPlayerManager().IsPlayerConnected(playerId) || !SCR_Global.IsAdmin(playerId))
			result = "An active admin login is required.";
		else if (!m_bReady)
			result = "Map recovery is still initializing.";
		else if (action == "status")
			result = m_sRecoveryStatus;
		else if (action == "save")
		{
			GC_MapPlanData captured = Capture();
			result = Validate(captured);
			if (result.IsEmpty())
				output = captured.Encode();
			if (output.Length() > 131072)
			{
				output = string.Empty;
				result = "The plan exceeds the 128 KiB transfer limit.";
			}
			else if (result.IsEmpty())
				result = "Plan exported.";
		}
		else if (action != "load" && action != "clear")
			result = "Unknown map plan action.";
		else if (GC_LobbyPhase.ArePlayerMapMarkersLocked())
			result = "Load and Clear are available before gameplay only.";
		else if (!m_Drawings.IsAvailable())
			result = "Load the Lobby2 Map Drawing Plans addon to manage drawings.";
		else
		{
			GC_MapPlanData data = new GC_MapPlanData();
			if (action == "load")
			{
				if (!data.Decode(input))
					result = "The plan file is invalid or too large.";
				else
					result = Validate(data);
			}
			if (result.IsEmpty())
			{
				Replace(data);
				m_bRecoveryBlocked = false;
				SaveRecovery();
				if (action == "load")
					result = "Plan loaded. " + m_sRecoveryStatus;
				else
					result = "Shared map cleared. " + m_sRecoveryStatus;
			}
		}
		GC_AdminActionLogger.Log(playerId, "map-plan", action, result);
		return result;
	}

	//------------------------------------------------------------------------------------------------
	void Stop()
	{
		GetGame().GetCallqueue().Remove(Initialize);
		GetGame().GetCallqueue().Remove(SaveChanged);
		GetGame().GetCallqueue().Remove(SaveRecovery);
		m_bReady = false;
		m_Mode = null;
	}
}
