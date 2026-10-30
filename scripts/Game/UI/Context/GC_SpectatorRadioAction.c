//! Reuses the existing player context menu; radio choices never select a transmitter.
class GC_SpectatorRadioAction : GC_ContextMenuAction
{
	protected int m_iOperation;
	protected RplId m_RadioId;
	protected string m_sLabel;

	void GC_SpectatorRadioAction(int operation, RplId radioId, string label)
	{
		m_iOperation = operation;
		m_RadioId = radioId;
		m_sLabel = label;
	}

	override string GetGroupLabel(GC_ContextMenuContext context)
	{
		return "RADIOS - LISTEN ONLY";
	}

	override string GetLabel(GC_ContextMenuContext context)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (m_iOperation != 0 && controller && controller.GC_IsListeningToRadio(context.GetPlayerId(), m_RadioId, m_iOperation == 1))
			return "[x] " + m_sLabel;
		return m_sLabel;
	}

	override bool CanExecute(GC_ContextMenuContext context)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		return context && manager && manager.GetLobbyPhase() == SCR_EGameModeState.GAME && controller && controller.GC_CanJumpSpectatorCamera();
	}

	override void Execute(GC_ContextMenuContext context)
	{
		if (!CanExecute(context))
			return;
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		controller.GC_RequestRadioListening(context.GetPlayerId(), context.GetPlayerIdentity(), m_iOperation, m_RadioId);
	}

	static void Collect(GC_ContextMenuContext context, array<ref GC_ContextMenuAction> actions)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!context || context.GetPlayerId() <= 0 || !manager || manager.GetLobbyPhase() != SCR_EGameModeState.GAME || !controller || !controller.GC_CanJumpSpectatorCamera())
			return;
		array<BaseRadioComponent> radios;
		GC_SpectatorRadio.GetRadios(context.GetEntity(), radios);
		// All uses authoritative inventory lookup, including outside local replication range.
		actions.Insert(new GC_SpectatorRadioAction(1, RplId.Invalid(), "All radios"));
		for (int index; index < radios.Count(); index++)
		{
			BaseRadioComponent radio = radios[index];
			RplId id = GC_SpectatorRadio.GetId(radio);
			if (!id.IsValid())
				continue;
			string label = "Radio " + (index + 1).ToString();
			for (int channel; channel < radio.TransceiversCount(); channel++)
			{
				if (channel == 0)
					label += " - ";
				else
					label += " / ";
				label += (radio.GetTransceiver(channel).GetFrequency() * 0.001).ToString(-1, 3) + " MHz";
			}
			if (!radio.IsPowered())
				label += " (off)";
			actions.Insert(new GC_SpectatorRadioAction(2, id, label));
		}
		if (controller.GC_IsListeningToRadios())
			actions.Insert(new GC_SpectatorRadioAction(0, RplId.Invalid(), "Stop listening"));
	}
}
