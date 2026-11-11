modded class GC_LobbyPlayerController
{
	protected float m_fGC_LastJipPress = -1000;
	protected float m_fGC_LastJipRequest = -10000;
	protected string m_sGC_JipMessage;
	protected RplId m_GC_JipOfferCharacter = RplId.Invalid();
	protected RplId m_GC_JipMovingCharacter = RplId.Invalid();
	protected vector m_vGC_JipDestination;
	protected int m_iGC_JipChecks;
	protected InputManager m_GC_JipInput;
	protected bool m_bGC_JipAuto;
	protected int m_iGC_JipPresentationChecks;

	//------------------------------------------------------------------------------------------------
	override protected void OnInit(IEntity owner)
	{
		super.OnInit(owner);
		if (!GetGame().InPlayMode() || RplSession.Mode() == RplMode.Dedicated || !GC_LobbyGameMode.GetGameMode())
			return;
		m_GC_JipInput = GetGame().GetInputManager();
		if (m_GC_JipInput)
			m_GC_JipInput.AddActionListener("GC_Jip", EActionTrigger.DOWN, GC_OnJipPressed);
	}

	//------------------------------------------------------------------------------------------------
	void GC_OnJipPressed()
	{
		if (GetGame().GetPlayerController() != this || !GC_LobbyGameMode.GetGameMode()
			|| !GetControlledEntity() || GetGame().GetMenuManager().IsAnyMenuOpen())
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now - m_fGC_LastJipPress <= 400)
		{
			m_fGC_LastJipPress = -1000;
			GC_RequestJip();
		}
		else
			m_fGC_LastJipPress = now;
	}

	//------------------------------------------------------------------------------------------------
	void GC_RequestJip()
	{
		if (GetGame().GetPlayerController() != this)
			return;
		if (Replication.IsServer())
			GC_RpcAskJip();
		else
			Rpc(GC_RpcAskJip);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcAskJip()
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!Replication.IsServer() || !mode || !mode.GetJip())
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now - m_fGC_LastJipRequest < 1000)
			return;
		m_fGC_LastJipRequest = now;
		mode.GetJip().Request(this);
	}

	//------------------------------------------------------------------------------------------------
	void GC_SendJipMessage(string message, RplId character = RplId.Invalid())
	{
		if (Replication.IsServer())
			Rpc(GC_RpcDoJipMessage, message, character, false);
	}

	//------------------------------------------------------------------------------------------------
	void GC_SendJipOffer(RplId character, bool automatic)
	{
		if (Replication.IsServer())
			Rpc(GC_RpcDoJipMessage, "#GC-Jip_Offer", character, automatic);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDoJipMessage(string message, RplId character, bool automatic)
	{
		if (GetGame().GetPlayerController() != this || !GC_LobbyGameMode.GetGameMode())
			return;
		m_sGC_JipMessage = message;
		m_GC_JipOfferCharacter = character;
		m_bGC_JipAuto = automatic;
		m_iGC_JipPresentationChecks = 0;
		GetGame().GetCallqueue().Remove(GC_PresentJipMessage);
		GC_PresentJipMessage();
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_PresentJipMessage()
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!mode || mode.GetState() == SCR_EGameModeState.POSTGAME || GetGame().GetPlayerController() != this)
			return;
		IEntity character = GetControlledEntity();
		if (++m_iGC_JipPresentationChecks > 120)
			return;
		if (mode.GetState() != SCR_EGameModeState.GAME || ArmaReforgerLoadingAnim.IsOpen()
			|| !character || !SCR_PopUpNotification.GetInstance()
			|| (m_GC_JipOfferCharacter.IsValid() && Replication.FindItemId(character) != m_GC_JipOfferCharacter))
		{
			GetGame().GetCallqueue().CallLater(GC_PresentJipMessage, 500);
			return;
		}
		// Possession can replicate after the offer. Auto uses the same server
		// request only once the owning client actually has its bound character.
		if (m_bGC_JipAuto)
		{
			m_bGC_JipAuto = false;
			GC_RequestJip();
			return;
		}
		string text = "<color rgba=\"235,235,235,255\">" + WidgetManager.Translate(m_sGC_JipMessage) + "</color>";
		SCR_PopUpNotification.GetInstance().PopupMsg(text, 12, "#GC-Jip_Title");
	}

	//------------------------------------------------------------------------------------------------
	void GC_DispatchJip(RplId character, vector position, ECharacterStance stance)
	{
		if (!Replication.IsServer() || !GC_LobbyGameMode.GetGameMode())
			return;
		m_GC_JipMovingCharacter = character;
		m_vGC_JipDestination = position;
		m_iGC_JipChecks = 0;
		Rpc(GC_RpcDoJipTeleport, character, position, stance);
		GetGame().GetCallqueue().CallLater(GC_ObserveJip, 500);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDoJipTeleport(RplId expectedCharacter, vector position, ECharacterStance stance)
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		ChimeraCharacter character = ChimeraCharacter.Cast(GetControlledEntity());
		if (!mode || mode.GetState() != SCR_EGameModeState.GAME || GetGame().GetPlayerController() != this
			|| !character || Replication.FindItemId(character) != expectedCharacter
			|| character.GetRootParent() != character || CompartmentAccessComponent.GetVehicleIn(character))
			return;
		CharacterControllerComponent control = character.GetCharacterController();
		if (!control || control.IsDead() || control.IsUnconscious())
			return;
		SCR_Global.TeleportPlayer(GetPlayerId(), position);
		control.SetStanceChange(SCR_AIStanceHandling.ConvertStanceToStanceChange(stance));
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_ObserveJip()
	{
		if (!Replication.IsServer() || !GC_LobbyGameMode.GetGameMode())
			return;
		IEntity character = GetControlledEntity();
		if (character && Replication.FindItemId(character) == m_GC_JipMovingCharacter)
		{
			if (vector.DistanceSq(character.GetOrigin(), m_vGC_JipDestination) <= 9)
			{
				GC_LobbyGameMode.GetGameMode().GetJip().CompleteMove(this, m_GC_JipMovingCharacter, true);
				GC_SendJipMessage("#GC-Jip_Arrived");
				return;
			}
			if (++m_iGC_JipChecks < 10)
			{
				GetGame().GetCallqueue().CallLater(GC_ObserveJip, 500);
				return;
			}
		}
		GC_SendJipMessage("#GC-Jip_Unverified");
	}

	//------------------------------------------------------------------------------------------------
	override void ShutdownLobbyPresentation()
	{
		GC_StopJipPresentation();
		super.ShutdownLobbyPresentation();
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_StopJipPresentation()
	{
		if (GetGame())
		{
			GetGame().GetCallqueue().Remove(GC_PresentJipMessage);
			GetGame().GetCallqueue().Remove(GC_ObserveJip);
			if (m_GC_JipInput)
				m_GC_JipInput.RemoveActionListener("GC_Jip", EActionTrigger.DOWN, GC_OnJipPressed);
		}
		m_GC_JipInput = null;
		m_sGC_JipMessage = string.Empty;
		m_GC_JipOfferCharacter = RplId.Invalid();
		m_GC_JipMovingCharacter = RplId.Invalid();
		m_bGC_JipAuto = false;
		m_fGC_LastJipPress = -1000;
	}

	//------------------------------------------------------------------------------------------------
	void ~GC_LobbyPlayerController()
	{
		GC_StopJipPresentation();
	}
}
