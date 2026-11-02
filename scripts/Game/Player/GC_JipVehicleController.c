modded class GC_LobbyPlayerController
{
	protected ref GC_JipVehicleMove m_GC_JipVehicleMove;
	protected int m_iGC_JipVehicleSequence;
	protected int m_iGC_JipSeatSequence;
	protected RplId m_GC_JipSeatCharacter;
	protected RplId m_GC_JipSeatOwner;
	protected int m_iGC_JipSeatId;
	protected int m_iGC_JipSeatChecks;
	protected bool m_bGC_JipSeatExecuted;

	//------------------------------------------------------------------------------------------------
	void GC_BeginJipVehicle(ChimeraCharacter character, IEntity target, IEntity vehicle)
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!Replication.IsServer() || !mode || !mode.GetJip()
			|| !mode.GetJip().ValidateVehicleMove(this, character, target, vehicle))
			return;
		if (m_GC_JipVehicleMove)
			m_GC_JipVehicleMove.Stop();
		m_GC_JipVehicleMove = new GC_JipVehicleMove();
		string result = m_GC_JipVehicleMove.Start(this, character, target, vehicle, ++m_iGC_JipVehicleSequence);
		if (!result.IsEmpty())
			GC_SendJipMessage(result);
	}

	//------------------------------------------------------------------------------------------------
	void GC_PrepareJipSeat(int sequence, RplId character, RplId owner, int seatId)
	{
		if (Replication.IsServer())
			Rpc(GC_RpcPrepareJipSeat, sequence, character, owner, seatId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcPrepareJipSeat(int sequence, RplId character, RplId owner, int seatId)
	{
		if (GetGame().GetPlayerController() != this || !GC_LobbyGameMode.GetGameMode())
			return;
		m_iGC_JipSeatSequence = sequence;
		m_GC_JipSeatCharacter = character;
		m_GC_JipSeatOwner = owner;
		m_iGC_JipSeatId = seatId;
		m_iGC_JipSeatChecks = 0;
		m_bGC_JipSeatExecuted = false;
		GetGame().GetCallqueue().Remove(GC_WaitForJipSeat);
		GC_WaitForJipSeat();
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_WaitForJipSeat()
	{
		if (m_iGC_JipSeatSequence == 0 || !GC_LobbyGameMode.GetGameMode())
			return;
		RplComponent owner = RplComponent.Cast(Replication.FindItem(m_GC_JipSeatOwner));
		if (owner && owner.GetEntity())
		{
			Rpc(GC_RpcJipSeatReady, m_iGC_JipSeatSequence);
			return;
		}
		if (++m_iGC_JipSeatChecks < 40)
			GetGame().GetCallqueue().CallLater(GC_WaitForJipSeat, 200);
		else
			Rpc(GC_RpcJipSeatRejected, m_iGC_JipSeatSequence);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcJipSeatReady(int sequence)
	{
		if (Replication.IsServer() && m_GC_JipVehicleMove)
			m_GC_JipVehicleMove.Ready(sequence);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcJipSeatRejected(int sequence)
	{
		if (Replication.IsServer() && m_GC_JipVehicleMove)
			m_GC_JipVehicleMove.Rejected(sequence);
	}

	//------------------------------------------------------------------------------------------------
	void GC_ExecuteJipSeat(int sequence)
	{
		if (Replication.IsServer())
			Rpc(GC_RpcExecuteJipSeat, sequence);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcExecuteJipSeat(int sequence)
	{
		if (sequence != m_iGC_JipSeatSequence || m_bGC_JipSeatExecuted || GetGame().GetPlayerController() != this)
			return;
		m_bGC_JipSeatExecuted = true;
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		ChimeraCharacter character = ChimeraCharacter.Cast(GetControlledEntity());
		RplComponent owner = RplComponent.Cast(Replication.FindItem(m_GC_JipSeatOwner));
		BaseCompartmentManagerComponent manager;
		if (owner && owner.GetEntity())
			manager = BaseCompartmentManagerComponent.Cast(owner.GetEntity().FindComponent(BaseCompartmentManagerComponent));
		BaseCompartmentSlot seat;
		if (manager)
			seat = manager.FindCompartment(m_iGC_JipSeatId);
		if (!mode || mode.GetState() != SCR_EGameModeState.GAME || !character
			|| Replication.FindItemId(character) != m_GC_JipSeatCharacter || !seat
			|| seat.IsOccupied() || !seat.IsCompartmentAccessible() || seat.IsGetInLockedFor(character)
			|| character.GetCharacterController().IsDead() || character.GetCharacterController().IsUnconscious()
			|| character.GetRootParent() != character || CompartmentAccessComponent.GetVehicleIn(character))
		{
			Rpc(GC_RpcJipSeatRejected, sequence);
			return;
		}
		if (!character.GetCompartmentAccessComponent().GetInVehicle(owner.GetEntity(), seat, true, -1, ECloseDoorAfterActions.INVALID, false))
			Rpc(GC_RpcJipSeatRejected, sequence);
	}

	//------------------------------------------------------------------------------------------------
	void GC_CancelJipSeat(int sequence)
	{
		if (Replication.IsServer())
			Rpc(GC_RpcCancelJipSeat, sequence);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcCancelJipSeat(int sequence)
	{
		if (sequence != m_iGC_JipSeatSequence)
			return;
		m_iGC_JipSeatSequence = 0;
		GetGame().GetCallqueue().Remove(GC_WaitForJipSeat);
	}

	//------------------------------------------------------------------------------------------------
	void ~GC_LobbyPlayerController()
	{
		if (!GetGame())
			return;
		GetGame().GetCallqueue().Remove(GC_WaitForJipSeat);
		if (m_GC_JipVehicleMove)
			m_GC_JipVehicleMove.Stop();
	}
}
