enum PlayerState
{
    SPECTATOR,
    PLAYING
}

modded class SCR_PlayerController : PlayerController
{
	protected PlayerState m_ePlayerState;

	void SetSpectator(IEntity entity)
	{
		SetPlayerState(PlayerState.SPECTATOR);
		SetPossessedEntity(entity);
		Rpc(RplDo_SpectatorBrodcast);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RplDo_SpectatorBrodcast()
	{
		SetPlayerState(PlayerState.SPECTATOR);

		RplComponent rpl = RplComponent.Cast(FindComponent(RplComponent));
		if (rpl && rpl.IsOwner())
		{
			//Do camera shit
		}
	}

	void SetPlayerState(PlayerState state)
	{
		if(m_ePlayerState == state)
			return;
		
		m_ePlayerState = state;

		//OnPlayerStateChanged
	}
}
