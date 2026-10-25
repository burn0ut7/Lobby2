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
	protected float m_fTop = -1;
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
					m_Root.SetName("LobbySetupWarning");
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
		// Keep both warning rows aligned with the setup and announcement cards.
		FrameSlot.SetSizeX(m_Card, GC_CountdownHud.WIDTH);
		if (title)
		{
			title.SetDesiredFontSize(21);
			title.SetMinFontSize(8);
		}
		m_Timer.SetDesiredFontSize(30);
		m_Timer.SetMinFontSize(18);
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
			|| !SCR_AIDamageHandling.IsAlive(m_Character) || now >= m_fDeadline
			|| (m_bSetupWarning && (!lobbyPlayer || !lobbyPlayer.GC_HasSetupReturnWarning())))
		{
			Close();
			return;
		}
		// Reserve separate rows rather than relying on root creation order/Z order.
		float top = 28;
		GC_LobbyFreezeComponent freeze = GC_LobbyFreezeComponent.GetInstance();
		bool staging = (lobbyPlayer && lobbyPlayer.GC_SetupActive()) || (freeze && freeze.AffectsPlayer(controller.GetPlayerId()));
		if (staging)
			top += 38;
		if (lobbyPlayer && !m_bSetupWarning && lobbyPlayer.GC_HasSetupReturnWarning())
			top += 74;
		if (lobbyPlayer && lobbyPlayer.GC_AnnouncementActive())
			top += 38;
		if (top != m_fTop)
		{
			FrameSlot.SetPos(m_Card, 0, top);
			m_fTop = top;
		}
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
		m_fTop = -1;
		m_Character = null;
	}
}

//! This is the Lobby2-owned controller, not a shared base-game controller override.
modded class GC_LobbyPlayerController
{
	protected ref GC_AOWarning m_GC_AOWarning;
	protected bool m_bGC_AOSent;
	protected IEntity m_GC_AOSentCharacter;
	protected float m_fGC_AOSentDeadline;
	protected string m_sGC_AOSentText;

	//------------------------------------------------------------------------------------------------
	void GC_SendAOWarning(IEntity character, float seconds, string warningText = "RETURN TO AO")
	{
		if (!Replication.IsServer())
			return;
		float deadline = -1;
		if (seconds >= 0)
			deadline = GetGame().GetWorld().GetWorldTime() * 0.001 + seconds;
		if (m_bGC_AOSent && m_GC_AOSentCharacter == character && Math.AbsFloat(deadline - m_fGC_AOSentDeadline) < 0.05 && warningText == m_sGC_AOSentText)
			return;
		m_bGC_AOSent = true;
		m_GC_AOSentCharacter = character;
		m_fGC_AOSentDeadline = deadline;
		m_sGC_AOSentText = warningText;
		RplId characterId = RplId.Invalid();
		if (character)
			characterId = Replication.FindItemId(character);
		ChimeraWorld world = GetGame().GetWorld();
		Rpc(GC_RpcDoAOWarning, characterId, seconds, warningText, world.GetServerTimestamp());
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDoAOWarning(RplId characterId, float seconds, string warningText, WorldTimestamp sentAt)
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
		ChimeraWorld world = GetGame().GetWorld();
		seconds = Math.Max(0, seconds - Math.Max(0, world.GetServerTimestamp().DiffSeconds(sentAt)));
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
