//! Adapt TilW's instruction to Lobby2's authority-owned budget and slot respawn.
modded class TILW_EditRespawnTicketsInstruction
{
	//------------------------------------------------------------------------------------------------
	override void Execute()
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!manager)
		{
			super.Execute();
			return;
		}
		if (!Replication.IsServer() || m_amount < 0)
			return;

		int tickets = manager.GC_GetFactionRespawnTickets(m_factionKey);
		switch (m_operation)
		{
			case TILW_EVariableOperation.SET:
				tickets = m_amount;
				break;
			case TILW_EVariableOperation.ADD:
				if (tickets > int.MAX - m_amount)
					return;
				tickets += m_amount;
				break;
			case TILW_EVariableOperation.SUB:
				tickets = Math.Max(0, tickets - m_amount);
				break;
			case TILW_EVariableOperation.MUL:
				if (m_amount > 0 && tickets > int.MAX / m_amount)
					return;
				tickets *= m_amount;
				break;
			case TILW_EVariableOperation.DIV:
				if (m_amount == 0)
					return;
				tickets /= m_amount;
				break;
			default:
				return;
		}

		if (manager.GC_SetFactionRespawnTickets(m_factionKey, tickets) && m_allowSpecRespawn)
			manager.GC_RespawnFactionSlots(m_factionKey);
	}
}
