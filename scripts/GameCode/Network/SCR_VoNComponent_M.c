modded class SCR_VoNComponent : VoNComponent
{
	protected static const string CONFIG = "{49CE1A7D3B74F5F3}Sounds/VON/GC_VAR_VON.conf";
	protected static const string VAR_NAME   = "GC_SPEC";

	override protected event void OnReceive(int playerId, bool isSenderEditor, BaseTransceiver receiver, int frequency, float quality)
	{
		super.OnReceive(playerId, isSenderEditor, receiver, frequency, quality);
		

		//PrintFormat("GC | Source:%1",AudioSystem.GetVariableIDByName(VAR_NAME, CONFIG));
		
		if(AudioSystem.GetVariableIDByName(VAR_NAME, CONFIG) != -1)
		{
			float current = AudioSystem.GetVariableValue(VAR_NAME, CONFIG);
			PrintFormat("GC | OnReceive Current:%1",current);

			AudioSystem.SetVariableByName(VAR_NAME, 1, CONFIG);
		}
		

		GC_LobbyManager lobbyManager = GC_LobbyManager.GetManager();
		if (!lobbyManager)
			return;

		GC_VoiceChannelAssignment assignment = lobbyManager.GetPlayerVoiceAssignment(playerId);
		if (!assignment)
			return;

		string channelName;
		switch (assignment.GetChannelType())
		{
			case GC_VoiceChannelType.GLOBAL:
				channelName = "GLOBAL";
				break;
			case GC_VoiceChannelType.FACTION:
				channelName = "FACTION";
				break;
			case GC_VoiceChannelType.GROUP:
				channelName = "GROUP";
				break;
			case GC_VoiceChannelType.PERSONAL:
				channelName = "PERSONAL";
				break;
		}
		
		PrintFormat(
			"GC | Voice receive | talker=%1 channel=%2 faction=%3 group=%4 personalOwner=%5",
			playerId,
			channelName,
			assignment.GetFactionKey(),
			assignment.GetGroupRplId(),
			assignment.GetPersonalOwnerIdentity()
		);
	}
}
