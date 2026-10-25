//! Local presentation only; this clock never causes gameplay death or teleportation.
class GC_AOWarning
{
	protected Widget m_Root;
	protected TextWidget m_Timer;
	protected Widget m_Card;
	protected bool m_bSetupWarning;
	protected IEntity m_Character;
	protected float m_fDeadline;
	protected float m_fLastUpdate;
	protected static const ResourceName LAYOUT = "{8F470B543A20D941}UI/layouts/Mission/Lobby_AOWarning.layout";

	//------------------------------------------------------------------------------------------------
	static string FormatTime(float seconds)
	{
		int totalMilliseconds = Math.Max(0, Math.Ceil(seconds * 1000));
		int minutes = totalMilliseconds / 60000;
		int wholeSeconds = (totalMilliseconds / 1000) % 60;
		int milliseconds = totalMilliseconds % 1000;
		string fraction = milliseconds.ToString();
		while (fraction.Length() < 3)
			fraction = "0" + fraction;
		string secondText = wholeSeconds.ToString();
		if (minutes > 0)
		{
			if (wholeSeconds < 10)
				secondText = "0" + secondText;
			return minutes.ToString() + ":" + secondText + "." + fraction;
		}
		return secondText + "." + fraction;
	}

	//------------------------------------------------------------------------------------------------
	void Update(IEntity character, float seconds, bool setupWarning = false, string warningText = "RETURN TO AO")
	{
		m_bSetupWarning = setupWarning;
		m_Character = character;
		m_fLastUpdate = GetGame().GetWorld().GetWorldTime();
		m_fDeadline = m_fLastUpdate + seconds * 1000;
		if (!m_Root)
		{
			m_Root = GetGame().GetWorkspace().CreateWidgets(LAYOUT);
			if (m_Root)
			{
				m_Timer = TextWidget.Cast(m_Root.FindAnyWidget("AOTimer"));
				m_Card = m_Root.FindAnyWidget("AOCard");
				if (m_bSetupWarning)
				{
					m_Root.SetName("LobbySetupWarning");

				}
			}
			if (!m_Timer || !m_Card)
			{
				Close();
				Print("GC | AO | warning layout unavailable", LogLevel.ERROR);
				return;
			}
			GetGame().GetCallqueue().CallLater(Refresh, 16, true);
		}
		TextWidget title = TextWidget.Cast(m_Root.FindAnyWidget("AOTitle"));
		if (title)
			title.SetText(warningText);
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void Refresh()
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		PlayerController controller = GetGame().GetPlayerController();
		GC_LobbyPlayerController lobbyPlayer = GC_LobbyPlayerController.Cast(controller);
		float now = GetGame().GetWorld().GetWorldTime();
		if (!mode || mode.GetState() != SCR_EGameModeState.GAME || !controller
			|| controller.GetControlledEntity() != m_Character || !m_Character
			|| !SCR_AIDamageHandling.IsAlive(m_Character) || now - m_fLastUpdate > 1500
			|| (m_bSetupWarning && (!lobbyPlayer || !lobbyPlayer.GC_HasSetupReturnWarning())))
		{
			Close();
			return;
		}
		// Reserve separate rows rather than relying on root creation order/Z order.
		float top = 70;
		if (lobbyPlayer && lobbyPlayer.GC_SetupActive())
		{
			top = 80;
			if (!m_bSetupWarning && lobbyPlayer.GC_HasSetupReturnWarning())
				top += 108;
		}
		FrameSlot.SetPos(m_Card, 0, top);
		m_Timer.SetText(FormatTime(Math.Max(0, (m_fDeadline - now) * 0.001)));
	}

	//------------------------------------------------------------------------------------------------
	void Close()
	{
		GetGame().GetCallqueue().Remove(Refresh);
		if (m_Root)
			m_Root.RemoveFromHierarchy();
		m_Root = null;
		m_Timer = null;
		m_Card = null;
		m_Character = null;
	}
}

//! This is the Lobby2-owned controller, not a shared base-game controller override.
modded class GC_LobbyPlayerController
{
	protected ref GC_AOWarning m_GC_AOWarning;

	//------------------------------------------------------------------------------------------------
	void GC_SendAOWarning(IEntity character, float seconds, string warningText = "RETURN TO AO")
	{
		if (!Replication.IsServer())
			return;
		RplId characterId = RplId.Invalid();
		if (character)
			characterId = Replication.FindItemId(character);
		Rpc(GC_RpcDoAOWarning, characterId, seconds, warningText);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDoAOWarning(RplId characterId, float seconds, string warningText)
	{
		if (GetGame().GetPlayerController() != this || !GC_LobbyGameMode.GetGameMode())
			return;
		IEntity character = GetControlledEntity();
		if (seconds < 0 || !character || Replication.FindItemId(character) != characterId)
		{
			if (m_GC_AOWarning)
				m_GC_AOWarning.Close();
			return;
		}
		if (!m_GC_AOWarning)
			m_GC_AOWarning = new GC_AOWarning();
		m_GC_AOWarning.Update(character, seconds, false, warningText);
	}

	//------------------------------------------------------------------------------------------------
	override void ShutdownLobbyPresentation()
	{
		if (m_GC_AOWarning)
			m_GC_AOWarning.Close();
		super.ShutdownLobbyPresentation();
	}
}
