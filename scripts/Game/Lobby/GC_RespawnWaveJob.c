//! A queued request targets the original claim, never a replacement occupant.
class GC_RespawnWaveCandidate
{
	int m_iSlot;
	int m_iPlayer;
	UUID m_sIdentity;

	void GC_RespawnWaveCandidate(GC_Slot slot)
	{
		m_iSlot = slot.GetID();
		m_iPlayer = slot.GetPlayerId();
		m_sIdentity = slot.GetUUID();
	}
}

//! Transient scheduling state. All eligibility, ownership and tickets are
//! revalidated by the manager immediately before each production respawn.
class GC_RespawnWaveJob
{
	ref array<string> m_aFactions = {};
	ref array<ref GC_RespawnWaveCandidate> m_aCandidates = {};
	ref array<int> m_aAttemptedPlayers = {};
	ref map<GC_RespawnLocation, int> m_mLocationIndices = new map<GC_RespawnLocation, int>();
	ref GC_RespawnLocation m_Location;
	ref GC_RespawnLocation m_OriginalLocation = new GC_RespawnLocation();
	ref map<string, ref GC_RespawnLocation> m_mDefaultLocations = new map<string, ref GC_RespawnLocation>();
	GC_MissionRuleEngine m_Completion;
	int m_iCursor;
	int m_iMaximum;
	int m_iRespawned;
	bool m_bUseTickets;
}
