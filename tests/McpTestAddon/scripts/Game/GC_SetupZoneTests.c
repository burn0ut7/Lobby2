//! Explicit runtime fixtures; no automatic tests or authority bypasses.
modded class GC_LobbyManager
{
	protected GC_SetupZoneEntity m_GC_SetupFixture;
	protected vector m_vGC_SetupOrigin;

	//------------------------------------------------------------------------------------------------
	string GC_TestSetupSpawn(int behavior, float seconds, float graceSeconds, float halfExtent, bool safeStart, string factionKey, bool outside)
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!Replication.IsServer() || !player || !player.GetControlledEntity())
			return "UNAVAILABLE authority/player";
		if (m_GC_SetupFixture)
			SCR_EntityHelper.DeleteEntityAndChildren(m_GC_SetupFixture);
		m_vGC_SetupOrigin = player.GetControlledEntity().GetOrigin();
		vector center = m_vGC_SetupOrigin;
		if (outside)
			center += Vector(halfExtent * 3, 0, 0);
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = center;
		m_GC_SetupFixture = GC_SetupZoneEntity.Cast(GetGame().SpawnEntityPrefab(
			Resource.Load("{6A4D831C2E5079B3}Prefabs/Logic/Setup/SetupZone.et"), GetGame().GetWorld(), params));
		if (!m_GC_SetupFixture)
			return "FAILED spawn";
		m_GC_SetupFixture.SetName("SetupFixture");
		array<vector> points = {center + Vector(-halfExtent, 0, -halfExtent), center + Vector(halfExtent, 0, -halfExtent),
			center + Vector(halfExtent, 0, halfExtent), center + Vector(-halfExtent, 0, halfExtent)};
		array<string> factions = {};
		if (!factionKey.IsEmpty())
			factions.Insert(factionKey);
		bool configured = m_GC_SetupFixture.Configure(points, factions, GC_LobbyGameMode.GetGameMode().GetElapsedTime() + seconds, behavior, graceSeconds, safeStart);
		return string.Format("configured=%1 behavior=%2 remaining=%3 origin=%4", configured, behavior, seconds, m_vGC_SetupOrigin);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestSetupSnapshot()
	{
		GC_LobbyPlayerController player = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!player || !player.GetControlledEntity())
			return "UNAVAILABLE player";
		ChimeraCharacter character = ChimeraCharacter.Cast(player.GetControlledEntity());
		if (!character)
			return "UNAVAILABLE character";
		Faction faction = GetPlayerFaction(player.GetPlayerId());
		string key;
		if (faction)
			key = faction.GetFactionKey();
		Widget root = GetGame().GetWorkspace().FindAnyWidget("LobbyStagingCountdown");
		string timer;
		string guidance;
		if (root)
		{
			TextWidget text = TextWidget.Cast(root.FindAnyWidget("StagingCountdownTime"));
			if (text)
				timer = text.GetText();
			text = TextWidget.Cast(root.FindAnyWidget("StagingCountdownGuidance"));
			if (text)
				guidance = text.GetText();
		}
		bool active = player.GC_SetupActive();
		bool frozen = player.GC_IsSetupFrozen();
		bool alive = SCR_AIDamageHandling.IsAlive(character);
		bool disabled = character.GetCharacterController().GetDisableMovementControls();
		vector offset = character.GetOrigin() - m_vGC_SetupOrigin;
		return string.Format("active=%1 frozen=%2 alive=%3 offset=%4 faction=%5 timer=%6 guidance=%7 movementDisabled=%8",
			active, frozen, alive, offset, key, timer, guidance, disabled);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestSetupMove(vector offset)
	{
		IEntity character = GetGame().GetPlayerController().GetControlledEntity();
		IEntity mover = CompartmentAccessComponent.GetVehicleIn(character);
		if (!mover)
			mover = character;
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(mover.FindComponent(SCR_EditableEntityComponent));
		if (!editable)
			return "UNAVAILABLE editable mover";
		vector transform[4];
		mover.GetWorldTransform(transform);
		transform[3] = m_vGC_SetupOrigin + offset;
		return string.Format("accepted=%1", editable.SetTransform(transform));
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestSetupRemove()
	{
		if (m_GC_SetupFixture)
			SCR_EntityHelper.DeleteEntityAndChildren(m_GC_SetupFixture);
		m_GC_SetupFixture = null;
		return "removed";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestSetupAdminFreeze(string action, int seconds, int targetId, bool stale)
	{
		GC_LobbyPlayerController player = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		GC_LobbyFreezeComponent freeze = GC_LobbyFreezeComponent.GetInstance();
		if (!player || !freeze)
			return "UNAVAILABLE player/freeze";
		array<int> targets = {targetId};
		array<string> identities = {GetPlayerVoiceIdentity(targetId)};
		if (stale)
			identities[0] = "stale-fixture-identity";
		player.RequestAdminFreeze(freeze.GetRevision(), action, seconds, targets, identities);
		return "DISPATCHED selected-player request; inspect audit and subsequent state";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestSetupFreezeDenied()
	{
		GC_LobbyFreezeComponent freeze = GC_LobbyFreezeComponent.GetInstance();
		if (!freeze)
			return "UNAVAILABLE freeze";
		int before = freeze.GetRevision();
		string result = freeze.ExecuteAdminAction(-1, before, "start", 30);
		return string.Format("result=%1 revisionBefore=%2 revisionAfter=%3", result, before, freeze.GetRevision());
	}
}
