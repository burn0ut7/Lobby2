enum GC_EAdminSlotAction
{
	LOCK,
	UNLOCK,
	//! Values 2 and 3 were retired assignment-pin actions.
	FREE = 4,
	REMOVE,
	RESPAWN_ORIGINAL,
	RESPAWN_CURRENT,
	FORCE_ENTRY,
	HEAL_OR_RESPAWN
}
