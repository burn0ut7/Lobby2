[ComponentEditorProps(category: "Lobby2/Mission", description: "Authoritative mission staging countdown and boundaries")]
class GC_LobbyFreezeComponentClass : ScriptComponentClass
{
}

//! Installed only on the Lobby2 game mode. Base-game possession and faction
//! remain authoritative; cached positions are only safe boundary return points.
class GC_LobbyFreezeComponent : ScriptComponent
{
	[Attribute("", UIWidgets.Object, "Staging areas; author a safe center and radius for every participating faction")]
	protected ref array<ref GC_LobbyStagingArea> m_aAreas;
	[RplProp(onRplName: "OnStateChanged")]
	protected int m_iConfiguredSeconds;
	[RplProp(onRplName: "OnStateChanged")]
	protected int m_iRemainingSeconds;
	[RplProp(onRplName: "OnStateChanged")]
	protected int m_iRevision;
	ref ScriptInvoker m_OnChanged = new ScriptInvoker();
	protected float m_fSecondElapsed;
	protected float m_fBoundaryElapsed;
	protected bool m_bShutdown;
	protected ref map<IEntity, vector> m_mSafePositions = new map<IEntity, vector>();
	protected Widget m_wCountdown;
	protected TextWidget m_wCountdownTime;
	protected int m_iDisplayedSeconds = -1;
	protected bool m_bCountdownCreationFailed;
	protected static const ResourceName COUNTDOWN_LAYOUT = "{79C8E2442DF142AB}UI/layouts/Mission/Lobby_StagingCountdown.layout";

	//------------------------------------------------------------------------------------------------
	static GC_LobbyFreezeComponent GetInstance()
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!mode || !GC_LobbyManager.GetManager())
			return null;
		GC_LobbyFreezeComponent component = GC_LobbyFreezeComponent.Cast(mode.FindComponent(GC_LobbyFreezeComponent));
		if (!component || component.m_bShutdown)
			return null;
		return component;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnStateChanged()
	{
		m_OnChanged.Invoke();
	}

	//------------------------------------------------------------------------------------------------
	int GetRevision()
	{
		return m_iRevision;
	}

	//------------------------------------------------------------------------------------------------
	int GetConfiguredSeconds()
	{
		return m_iConfiguredSeconds;
	}

	//------------------------------------------------------------------------------------------------
	int GetRemainingSeconds()
	{
		return m_iRemainingSeconds;
	}

	//------------------------------------------------------------------------------------------------
	bool IsFreezeActive()
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		return !m_bShutdown && mode && mode == GetOwner() && mode.GetState() == SCR_EGameModeState.GAME && m_iRemainingSeconds > 0;
	}

	//------------------------------------------------------------------------------------------------
	//! Configuration never substitutes for an authenticated execution request.
	string ExecuteAdminAction(int requesterId, int expectedRevision, string action, int seconds)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		string context = string.Format("%1 seconds=%2 revision=%3", action, seconds, expectedRevision);
		if (!manager || !manager.AuthorizeAdminAction(requesterId, "mission-freeze", context))
			return "rejected-not-logged-in";
		string outcome = ApplyAdminAction(expectedRevision, action, seconds);
		GC_AdminActionLogger.Log(requesterId, "mission-freeze", context, outcome);
		return outcome;
	}

	//------------------------------------------------------------------------------------------------
	protected string ApplyAdminAction(int expectedRevision, string action, int seconds)
	{
		if (GetInstance() != this || expectedRevision != m_iRevision)
			return "rejected-stale-freeze";
		if (seconds < 0 || seconds > 3600)
			return "rejected-freeze-duration";
		SCR_EGameModeState phase = GC_LobbyGameMode.GetGameMode().GetState();
		if (phase == SCR_EGameModeState.POSTGAME)
			return "rejected-mission-ended";
		if (action == "configure")
		{
			if (phase == SCR_EGameModeState.GAME)
				return "rejected-mission-started";
			if (seconds > 0 && !HasStagingAreas())
				return "rejected-staging-areas-missing";
			m_iConfiguredSeconds = seconds;
		}
		else if (action == "start")
		{
			if (phase != SCR_EGameModeState.GAME || IsFreezeActive() || seconds == 0)
				return "rejected-freeze-state";
			if (!HasStagingAreas())
				return "rejected-staging-areas-missing";
			m_iRemainingSeconds = seconds;
			m_fSecondElapsed = 0;
		}
		else if (action == "extend")
		{
			if (!IsFreezeActive() || seconds == 0 || m_iRemainingSeconds + seconds > 3600)
				return "rejected-freeze-state";
			m_iRemainingSeconds += seconds;
		}
		else if (action == "end")
		{
			if (!IsFreezeActive() || seconds != 0)
				return "rejected-freeze-state";
			m_iRemainingSeconds = 0;
			m_mSafePositions.Clear();
		}
		else
			return "rejected-freeze-action";
		m_iRevision++;
		Replication.BumpMe();
		OnStateChanged();
		return "applied";
	}

	//------------------------------------------------------------------------------------------------
	bool HasStagingAreas()
	{
		if (!m_aAreas || m_aAreas.IsEmpty())
			return false;
		foreach (GC_LobbyStagingArea area : m_aAreas)
		{
			if (!area || !area.IsValid())
				return false;
		}
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!manager)
			return false;
		foreach (Faction faction : manager.GetRegisteredFactions())
		{
			bool covered;
			foreach (GC_LobbyStagingArea factionArea : m_aAreas)
			{
				if (factionArea.AllowsFaction(faction))
					covered = true;
			}
			if (!covered)
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Called from the owning game-mode transition, including native mission end.
	void OnPhaseChanged()
	{
		if (GetInstance() != this || !Replication.IsServer())
			return;
		m_mSafePositions.Clear();
		m_fSecondElapsed = 0;
		m_iRemainingSeconds = 0;
		if (GC_LobbyGameMode.GetGameMode().GetState() == SCR_EGameModeState.GAME && HasStagingAreas())
			m_iRemainingSeconds = m_iConfiguredSeconds;
		m_iRevision++;
		Replication.BumpMe();
		OnStateChanged();
	}

	//------------------------------------------------------------------------------------------------
	//! Called at native character and turret control-processing boundaries, after input
	//! collection. A game-mode frame callback runs too early to suppress firing.
	void FilterCombatActions(ActionManager actions)
	{
		if (!actions || GetInstance() != this || !IsFreezeActive())
			return;
		actions.SetActionValue("CharacterFire", 0);
		actions.SetActionValue("CharacterThrowGrenade", 0);
		actions.SetActionValue("CharacterMelee", 0);
		actions.SetActionValue("CharacterFireStatic", 0);
		actions.SetActionValue("TurretFire", 0);
		actions.SetActionValue("VehicleFire", 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void EnforceBoundaries()
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		map<IEntity, vector> currentPositions = new map<IEntity, vector>();
		foreach (int playerId : players)
		{
			GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			if (!controller || controller.GetLobbyPlayerState() != GC_ELobbyPlayerState.PLAYING)
				continue;
			IEntity character = controller.GetControlledEntity();
			if (!character)
				continue;
			Faction faction = manager.GetPlayerFaction(playerId);
			GC_LobbyStagingArea nearest;
			float nearestDistance = float.MAX;
			bool inside;
			foreach (GC_LobbyStagingArea area : m_aAreas)
			{
				if (!area.AllowsFaction(faction))
					continue;
				float distance = vector.DistanceSqXZ(character.GetOrigin(), area.m_vCenter);
				if (distance < nearestDistance)
				{
					nearest = area;
					nearestDistance = distance;
				}
				if (area.Contains(character.GetOrigin()))
					inside = true;
			}
			if (!nearest)
				continue;
			if (inside)
			{
				currentPositions.Set(character, character.GetOrigin());
				continue;
			}
			vector safePosition;
			if (!m_mSafePositions.Find(character, safePosition) || !nearest.Contains(safePosition))
				safePosition = nearest.m_vCenter;
			controller.ApplyFreezeBoundary(safePosition);
			currentPositions.Set(character, safePosition);
		}
		m_mSafePositions = currentPositions;
	}

	//------------------------------------------------------------------------------------------------
	//! Presentation reads the replicated seconds; it never advances a local clock.
	protected void UpdateCountdownPresentation()
	{
		if (GetInstance() != this || !IsFreezeActive() || RplSession.Mode() == RplMode.Dedicated || !GetGame().GetPlayerController())
		{
			CloseCountdownPresentation();
			return;
		}
		if (m_bCountdownCreationFailed)
			return;
		if (!m_wCountdown)
		{
			m_wCountdown = GetGame().GetWorkspace().CreateWidgets(COUNTDOWN_LAYOUT);
			if (m_wCountdown)
				m_wCountdownTime = TextWidget.Cast(m_wCountdown.FindAnyWidget("StagingCountdownTime"));
			if (!m_wCountdownTime)
			{
				CloseCountdownPresentation();
				m_bCountdownCreationFailed = true;
				Print("GC | Staging countdown layout unavailable", LogLevel.ERROR);
				return;
			}
		}
		if (m_iDisplayedSeconds == m_iRemainingSeconds)
			return;
		m_iDisplayedSeconds = m_iRemainingSeconds;
		int minutes = m_iRemainingSeconds / 60;
		int seconds = m_iRemainingSeconds % 60;
		string minuteText = minutes.ToString();
		string secondText = seconds.ToString();
		if (minutes < 10)
			minuteText = "0" + minuteText;
		if (seconds < 10)
			secondText = "0" + secondText;
		m_wCountdownTime.SetText(minuteText + ":" + secondText);
	}

	//------------------------------------------------------------------------------------------------
	protected void CloseCountdownPresentation()
	{
		if (m_wCountdown)
			m_wCountdown.RemoveFromHierarchy();
		m_wCountdown = null;
		m_wCountdownTime = null;
		m_iDisplayedSeconds = -1;
	}

	//------------------------------------------------------------------------------------------------
	override protected void EOnFrame(IEntity owner, float timeSlice)
	{
		UpdateCountdownPresentation();
		if (GetInstance() != this || !IsFreezeActive())
			return;
		if (!Replication.IsServer())
			return;
		m_fBoundaryElapsed += timeSlice;
		if (m_fBoundaryElapsed >= 0.1)
		{
			m_fBoundaryElapsed = 0;
			EnforceBoundaries();
		}
		m_fSecondElapsed += timeSlice;
		if (m_fSecondElapsed < 1)
			return;
		int elapsed = Math.Floor(m_fSecondElapsed);
		m_fSecondElapsed -= elapsed;
		m_iRemainingSeconds = Math.Max(0, m_iRemainingSeconds - elapsed);
		if (m_iRemainingSeconds == 0)
		{
			m_mSafePositions.Clear();
			GC_AdminActionLogger.Log(0, "mission-freeze", "countdown", "expired");
		}
		Replication.BumpMe();
		OnStateChanged();
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!SCR_Global.IsEditMode())
			SetEventMask(owner, EntityEvent.FRAME);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		m_bShutdown = true;
		CloseCountdownPresentation();
		m_mSafePositions.Clear();
		super.OnDelete(owner);
	}
}
