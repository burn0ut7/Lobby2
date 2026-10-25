//! Setup state is sent only to the affected owner. The server owns enrollment and expiry.
modded class GC_LobbyPlayerController
{
	protected GC_SetupZoneEntity m_GC_SetupZone;
	protected IEntity m_GC_SetupCharacter;
	protected float m_fGC_SetupDeadline;
	protected float m_fGC_SetupOutsideDeadline;
	protected float m_fGC_SetupUpdated;
	protected bool m_bGC_SetupSafeStart;
	protected bool m_bGC_SetupFrozen;
	protected Widget m_GC_SetupHud;
	protected TextWidget m_GC_SetupTimer;
	protected ref GC_AOWarning m_GC_SetupWarning;
	protected bool m_bGC_SetupSent;
	protected string m_sGC_SetupText;
	protected int m_iGC_SetupDisplayed = -1;

	//------------------------------------------------------------------------------------------------
	GC_SetupZoneEntity GC_GetSetupZone()
	{
		if (m_GC_SetupCharacter != GetControlledEntity() || !GC_SetupActive())
			return null;
		return m_GC_SetupZone;
	}

	//------------------------------------------------------------------------------------------------
	bool GC_OwnsSetupZone(GC_SetupZoneEntity zone)
	{
		return m_GC_SetupZone == zone;
	}

	//------------------------------------------------------------------------------------------------
	bool GC_SetupActive()
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!mode || mode.GetState() != SCR_EGameModeState.GAME || !m_GC_SetupCharacter
			|| GetControlledEntity() != m_GC_SetupCharacter || !SCR_AIDamageHandling.IsAlive(m_GC_SetupCharacter))
			return false;
		float now = GetGame().GetWorld().GetWorldTime();
		return now < m_fGC_SetupDeadline;
	}

	//------------------------------------------------------------------------------------------------
	bool GC_IsSetupFrozen()
	{
		return GC_SetupActive() && m_bGC_SetupFrozen;
	}

	//------------------------------------------------------------------------------------------------
	bool GC_HasSetupReturnWarning()
	{
		return GC_SetupActive() && m_fGC_SetupOutsideDeadline > 0;
	}

	//------------------------------------------------------------------------------------------------
	//! Shared by normal weapon inputs and animation-driven flare firing.
	bool GC_IsCombatRestricted()
	{
		if (!GC_LobbyGameMode.GetGameMode())
			return false;
		GC_LobbyFreezeComponent freeze = GC_LobbyFreezeComponent.GetInstance();
		bool adminFreeze = freeze && freeze.AffectsPlayer(GetPlayerId());
		return GC_AreAdminControlsFrozen() || adminFreeze || (GC_SetupActive() && m_bGC_SetupSafeStart);
	}

	//------------------------------------------------------------------------------------------------
	void GC_FilterRestrictedActions(ActionManager actions)
	{
		if (!actions || !GC_IsCombatRestricted())
			return;
		if (GC_AreAdminControlsFrozen())
		{
			// Suppress the affected player's whole gameplay action set, including
			// vehicle controls. Keep communication and menu recovery available.
			int count = actions.GetActionCount();
			for (int i; i < count; i++)
			{
				string action = actions.GetActionName(i);
				if (action.StartsWith("Menu") || action.StartsWith("UI") || action.StartsWith("Chat")
					|| action.StartsWith("VON") || action == "GC_AdminVON" || action == "GC_OpenLobby")
					continue;
				actions.SetActionValue(action, 0);
			}
			return;
		}
		actions.SetActionValue("CharacterFire", 0);
		actions.SetActionValue("CharacterThrowGrenade", 0);
		actions.SetActionValue("CharacterMelee", 0);
		actions.SetActionValue("CharacterFireStatic", 0);
		actions.SetActionValue("TurretFire", 0);
		actions.SetActionValue("VehicleFire", 0);
	}

	//------------------------------------------------------------------------------------------------
	void GC_SetSetupZone(GC_SetupZoneEntity zone, IEntity character, float seconds, float outsideSeconds, bool safeStart, bool frozen)
	{
		if (!Replication.IsServer())
			return;
		string displayText;
		if (zone)
			displayText = zone.GetDisplayText();
		float now = GetGame().GetWorld().GetWorldTime();
		float deadline = now + seconds * 1000;
		float outsideDeadline;
		if (outsideSeconds >= 0)
			outsideDeadline = now + outsideSeconds * 1000;
		if (m_bGC_SetupSent && m_GC_SetupZone == zone && m_GC_SetupCharacter == character
			&& (!zone || Math.AbsFloat(deadline - m_fGC_SetupDeadline) < 50)
			&& Math.AbsFloat(outsideDeadline - m_fGC_SetupOutsideDeadline) < 50
			&& safeStart == m_bGC_SetupSafeStart && frozen == m_bGC_SetupFrozen && displayText == m_sGC_SetupText)
			return;
		m_bGC_SetupSent = true;
		m_sGC_SetupText = displayText;
		m_GC_SetupZone = zone;
		GC_ApplySetupState(character, seconds, outsideSeconds, safeStart, frozen, displayText);
		RplId characterId = RplId.Invalid();
		if (character)
			characterId = Replication.FindItemId(character);
		ChimeraWorld world = GetGame().GetWorld();
		Rpc(GC_RpcDoSetupState, characterId, seconds, outsideSeconds, safeStart, frozen, displayText, world.GetServerTimestamp());
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDoSetupState(RplId characterId, float seconds, float outsideSeconds, bool safeStart, bool frozen, string displayText, WorldTimestamp sentAt)
	{
		if (GetGame().GetPlayerController() != this || !GC_LobbyGameMode.GetGameMode())
			return;
		IEntity character = GetControlledEntity();
		if (!character || Replication.FindItemId(character) != characterId)
			character = null;
		ChimeraWorld world = GetGame().GetWorld();
		float elapsed = Math.Max(0, world.GetServerTimestamp().DiffSeconds(sentAt));
		seconds = Math.Max(0, seconds - elapsed);
		if (outsideSeconds >= 0)
			outsideSeconds = Math.Max(0, outsideSeconds - elapsed);
		GC_ApplySetupState(character, seconds, outsideSeconds, safeStart, frozen, displayText);
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_ApplySetupState(IEntity character, float seconds, float outsideSeconds, bool safeStart, bool frozen, string displayText)
	{
		m_GC_SetupCharacter = character;
		m_fGC_SetupUpdated = GetGame().GetWorld().GetWorldTime();
		m_fGC_SetupDeadline = m_fGC_SetupUpdated + seconds * 1000;
		m_fGC_SetupOutsideDeadline = 0;
		if (outsideSeconds >= 0)
			m_fGC_SetupOutsideDeadline = m_fGC_SetupUpdated + outsideSeconds * 1000;
		m_bGC_SetupSafeStart = safeStart;
		m_bGC_SetupFrozen = frozen;
		if (GetGame().GetPlayerController() != this)
			return;
		if (!GC_SetupActive())
		{
			GC_CloseSetupHud();
			return;
		}
		if (!m_GC_SetupHud)
		{
			m_GC_SetupHud = GetGame().GetWorkspace().CreateWidgets("{79C8E2442DF142AB}UI/layouts/Mission/Lobby_StagingCountdown.layout");
			if (!m_GC_SetupHud)
				return;
			m_GC_SetupTimer = TextWidget.Cast(m_GC_SetupHud.FindAnyWidget("StagingCountdownTime"));

			GetGame().GetCallqueue().CallLater(GC_RefreshSetupHud, 100, true);
		}
		m_iGC_SetupDisplayed = -1;
		TextWidget title = TextWidget.Cast(m_GC_SetupHud.FindAnyWidget("StagingCountdownTitle"));
		if (title)
			title.SetText(displayText);
		if (outsideSeconds >= 0)
		{
			if (!m_GC_SetupWarning)
				m_GC_SetupWarning = new GC_AOWarning();
			m_GC_SetupWarning.Update(character, outsideSeconds, true, "RETURN TO SETUP");
		}
		else if (m_GC_SetupWarning)
			m_GC_SetupWarning.Close();
		GC_RefreshSetupHud();
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_RefreshSetupHud()
	{
		if (!GC_SetupActive() || !m_GC_SetupTimer)
		{
			GC_CloseSetupHud();
			return;
		}
		float now = GetGame().GetWorld().GetWorldTime();
		int remaining = Math.Ceil((m_fGC_SetupDeadline - now) * 0.001);
		if (remaining == m_iGC_SetupDisplayed)
			return;
		m_iGC_SetupDisplayed = remaining;
		int seconds = remaining % 60;
		string secondText = seconds.ToString();
		if (seconds < 10)
			secondText = "0" + secondText;
		m_GC_SetupTimer.SetText((remaining / 60).ToString() + ":" + secondText);
		GC_CountdownHud.Fit(m_GC_SetupHud, m_GC_SetupTimer);
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_CloseSetupHud()
	{
		GetGame().GetCallqueue().Remove(GC_RefreshSetupHud);
		if (m_GC_SetupWarning)
			m_GC_SetupWarning.Close();
		if (m_GC_SetupHud)
			m_GC_SetupHud.RemoveFromHierarchy();
		m_GC_SetupHud = null;
		m_GC_SetupTimer = null;
	}

	//------------------------------------------------------------------------------------------------
	//! Possession readiness requests current owner state once; no countdown heartbeat.
	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		super.OnControlledEntityChanged(from, to);
		GC_RequestBoundaryPresentation();
	}

	//------------------------------------------------------------------------------------------------
	void GC_RequestBoundaryPresentation()
	{
		if (GetGame().GetPlayerController() != this || !GC_LobbyGameMode.GetGameMode())
			return;
		GC_CloseSetupHud();
		if (m_GC_AOWarning)
			m_GC_AOWarning.Close();
		if (Replication.IsServer())
			GC_RpcAskBoundaryPresentation();
		else
			Rpc(GC_RpcAskBoundaryPresentation);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcAskBoundaryPresentation()
	{
		if (!Replication.IsServer() || !GC_LobbyGameMode.GetGameMode())
			return;
		m_bGC_SetupSent = false;
		m_bGC_AOSent = false;
		m_bGC_AnnouncementSent = false;
	}

	//------------------------------------------------------------------------------------------------
	override void ShutdownLobbyPresentation()
	{
		GC_CloseSetupHud();
		m_GC_SetupCharacter = null;
		m_GC_SetupZone = null;
		super.ShutdownLobbyPresentation();
	}
}
