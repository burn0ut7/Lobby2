modded class SCR_VONController
{
	//------------------------------------------------------------------------------------------------
	override void AddEntry(SCR_VONEntry entry)
	{
		//! The lobby transport is not a selectable gameplay radio. Its fixed
		//! admin receiver and spectator channels are owned by the lobby controller.
		SCR_VONEntryRadio lobbyEntry = SCR_VONEntryRadio.Cast(entry);
		if (GC_LobbyGameMode.GetGameMode() && lobbyEntry && lobbyEntry.GetTransceiver())
		{
			BaseRadioComponent lobbyRadio = lobbyEntry.GetTransceiver().GetRadio();
			GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetOwner());
			if (controller && lobbyRadio && lobbyRadio.GetOwner() && lobbyRadio.GetOwner().GetPrefabData() && lobbyRadio.GetOwner().GetPrefabData().GetPrefabName() == GC_LobbyRadio.PREFAB)
			{
				controller.ApplyLobbyRadio();
				return;
			}
		}
		super.AddEntry(entry);
	}
}
