//! A briefing acknowledgement, not a second source of faction or command ownership.
class GC_FactionReadiness
{
	int m_iSlotId;
	int m_iPlayerId;
	string m_sIdentity;

	//------------------------------------------------------------------------------------------------
	void GC_FactionReadiness(int slotId, int playerId, string identity)
	{
		m_iSlotId = slotId;
		m_iPlayerId = playerId;
		m_sIdentity = identity;
	}
}
