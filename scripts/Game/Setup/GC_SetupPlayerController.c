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
	protected TextWidget m_GC_SetupGuidance;

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
		return now < m_fGC_SetupDeadline && now - m_fGC_SetupUpdated < 1500;
	}

	//------------------------------------------------------------------------------------------------
	bool GC_IsSetupFrozen()
	{
		return GC_SetupActive() && m_bGC_SetupFrozen;
	}

	//------------------------------------------------------------------------------------------------
	//! Shared by normal weapon inputs and animation-driven flare firing.
	bool GC_IsCombatRestricted()
	{
		if (!GC_LobbyGameMode.GetGameMode())
			return false;
		GC_LobbyFreezeComponent freeze = GC_LobbyFreezeComponent.GetInstance();
		bool adminFreeze = freeze && freeze.AffectsPlayer(GetPlayerId());
		return adminFreeze || (GC_SetupActive() && m_bGC_SetupSafeStart);
	}

	//------------------------------------------------------------------------------------------------
	void GC_FilterRestrictedActions(ActionManager actions)
	{
		if (!actions || !GC_IsCombatRestricted())
			return;
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
		m_GC_SetupZone = zone;
		GC_ApplySetupState(character, seconds, outsideSeconds, safeStart, frozen);
		RplId characterId = RplId.Invalid();
		if (character)
			characterId = Replication.FindItemId(character);
		Rpc(GC_RpcDoSetupState, characterId, seconds, outsideSeconds, safeStart, frozen);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDoSetupState(RplId characterId, float seconds, float outsideSeconds, bool safeStart, bool frozen)
	{
		if (GetGame().GetPlayerController() != this || !GC_LobbyGameMode.GetGameMode())
			return;
		IEntity character = GetControlledEntity();
		if (!character || Replication.FindItemId(character) != characterId)
			character = null;
		GC_ApplySetupState(character, seconds, outsideSeconds, safeStart, frozen);
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_ApplySetupState(IEntity character, float seconds, float outsideSeconds, bool safeStart, bool frozen)
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
			m_GC_SetupGuidance = TextWidget.Cast(m_GC_SetupHud.FindAnyWidget("StagingCountdownGuidance"));
			TextWidget title = TextWidget.Cast(m_GC_SetupHud.FindAnyWidget("StagingCountdownTitle"));
			if (title)
				title.SetText("SETUP");
			GetGame().GetCallqueue().CallLater(GC_RefreshSetupHud, 100, true);
		}
		GC_RefreshSetupHud();
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_RefreshSetupHud()
	{
		if (!GC_SetupActive() || !m_GC_SetupTimer || !m_GC_SetupGuidance)
		{
			GC_CloseSetupHud();
			return;
		}
		float now = GetGame().GetWorld().GetWorldTime();
		int remaining = Math.Ceil((m_fGC_SetupDeadline - now) * 0.001);
		int seconds = remaining % 60;
		string secondText = seconds.ToString();
		if (seconds < 10)
			secondText = "0" + secondText;
		m_GC_SetupTimer.SetText((remaining / 60).ToString() + ":" + secondText);
		string guidance = "Stay in your setup area";
		if (m_bGC_SetupFrozen)
			guidance = "Movement locked";
		GC_LobbyFreezeComponent adminFreeze = GC_LobbyFreezeComponent.GetInstance();
		if (m_bGC_SetupSafeStart || (adminFreeze && adminFreeze.AffectsPlayer(GetPlayerId())))
			guidance += " • Weapons disabled";
		if (m_fGC_SetupOutsideDeadline > 0)
			guidance = string.Format("Return to setup area • Death in %1 s", Math.Max(0, Math.Ceil((m_fGC_SetupOutsideDeadline - now) * 0.001)));
		m_GC_SetupGuidance.SetText(guidance);
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_CloseSetupHud()
	{
		GetGame().GetCallqueue().Remove(GC_RefreshSetupHud);
		if (m_GC_SetupHud)
			m_GC_SetupHud.RemoveFromHierarchy();
		m_GC_SetupHud = null;
		m_GC_SetupTimer = null;
		m_GC_SetupGuidance = null;
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
