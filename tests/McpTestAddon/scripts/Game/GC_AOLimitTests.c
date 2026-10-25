//! Explicit optional fixtures; no automatic execution, permission grants, or duplicate AO logic.
modded class GC_LobbyManager
{
	protected ref array<GC_AOLimitEntity> m_aGC_AOFixtures = {};
	protected vector m_vGC_AOOrigin;

	//------------------------------------------------------------------------------------------------
	string GC_TestAOSpawn()
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!Replication.IsServer() || !player || !player.GetControlledEntity())
			return "UNAVAILABLE authority and controlled entity";
		if (m_aGC_AOFixtures.IsEmpty())
			m_vGC_AOOrigin = player.GetControlledEntity().GetOrigin();
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = m_vGC_AOOrigin;
		GC_AOLimitEntity limit = GC_AOLimitEntity.Cast(GetGame().SpawnEntityPrefab(
			Resource.Load("{8F470B543A20D944}Prefabs/Logic/AO/AOLimit.et"), GetGame().GetWorld(), params));
		if (!limit)
			return "FAILED spawn";
		limit.SetName("AOFixture" + m_aGC_AOFixtures.Count());
		m_aGC_AOFixtures.Insert(limit);
		return string.Format("spawned index=%1 origin=%2; configure after native initialization", m_aGC_AOFixtures.Count() - 1, m_vGC_AOOrigin);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAOConfigure(int index, int behavior, bool inverted, float seconds, int priority, float halfExtent, vector offset)
	{
		if (!m_aGC_AOFixtures.IsIndexValid(index) || !m_aGC_AOFixtures[index])
			return "UNAVAILABLE fixture";
		GC_AOLimitEntity limit = m_aGC_AOFixtures[index];
		vector center = m_vGC_AOOrigin + offset;
		array<vector> points = {};
		points.Insert(center + Vector(-halfExtent, 0, -halfExtent));
		points.Insert(center + Vector(halfExtent, 0, -halfExtent));
		points.Insert(center + Vector(halfExtent, 0, halfExtent));
		points.Insert(center + Vector(-halfExtent, 0, halfExtent));
		bool behaviorSet = limit.SetBehavior(behavior, seconds, priority, index);
		bool polygonSet = limit.SetPolygon(points, inverted, "");
		return string.Format("behavior=%1 polygon=%2 enabled=%3 permitsOrigin=%4", behaviorSet, polygonSet, limit.SetEnabled(true), limit.Permits(m_vGC_AOOrigin));
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAOFactions(int index, string factionKeys)
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!m_aGC_AOFixtures.IsIndexValid(index) || !m_aGC_AOFixtures[index] || !player || !player.GetControlledEntity())
			return "UNAVAILABLE fixture/player";
		array<string> factions = {};
		if (!factionKeys.IsEmpty())
			factionKeys.Split(",", factions, true);
		GC_AOLimitEntity limit = m_aGC_AOFixtures[index];
		bool updated = limit.SetEligibility(factions, GC_EAOVehicleState.ANY, GC_EAOListMode.IGNORE, {}, GC_EAOItemCondition.IGNORE, {});
		Faction faction = GetPlayerFaction(player.GetPlayerId());
		string key;
		if (faction)
			key = faction.GetFactionKey();
		return string.Format("updated=%1 faction=%2 applies=%3", updated, key,
			limit.AppliesTo(player.GetPlayerId(), player.GetControlledEntity(), CompartmentAccessComponent.GetVehicleIn(player.GetControlledEntity())));
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAOFilter(int index, int vehicleState, int itemCondition, bool existingItem, int vehicleMode, bool currentFactionOnly)
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!m_aGC_AOFixtures.IsIndexValid(index) || !player || !player.GetControlledEntity())
			return "UNAVAILABLE fixture/player";
		array<ResourceName> items = {"{4A71F755A4513227}Prefabs/Vehicles/Wheeled/M998/M1025.et"};
		if (existingItem)
		{
			InventoryStorageManagerComponent inventory = InventoryStorageManagerComponent.Cast(player.GetControlledEntity().FindComponent(InventoryStorageManagerComponent));
			array<IEntity> inventoryItems = {};
			if (inventory)
				inventory.GetItems(inventoryItems);
			if (inventoryItems.IsEmpty())
				return "UNAVAILABLE existing inventory item";
			items[0] = inventoryItems[0].GetPrefabData().GetPrefabName();
		}
		array<string> factions = {};
		Faction faction = GetPlayerFaction(player.GetPlayerId());
		if (currentFactionOnly && faction)
			factions.Insert(faction.GetFactionKey());
		array<ResourceName> vehicles = {"{4A71F755A4513227}Prefabs/Vehicles/Wheeled/M998/M1025.et"};
		bool changed = m_aGC_AOFixtures[index].SetEligibility(factions, vehicleState, vehicleMode, vehicles, itemCondition, items);
		return string.Format("changed=%1 item=%2 applies=%3", changed, items[0], m_aGC_AOFixtures[index].AppliesTo(player.GetPlayerId(), player.GetControlledEntity(), CompartmentAccessComponent.GetVehicleIn(player.GetControlledEntity())));
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAOToggle(int index, bool enabled, bool remove)
	{
		if (!m_aGC_AOFixtures.IsIndexValid(index) || !m_aGC_AOFixtures[index])
			return "UNAVAILABLE fixture";
		if (remove)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(m_aGC_AOFixtures[index]);
			m_aGC_AOFixtures[index] = null;
			return "removed";
		}
		return m_aGC_AOFixtures[index].SetEnabled(enabled).ToString();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAOMove(vector offset)
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!player)
			return "UNAVAILABLE local player";
		return GC_TestAOMovePlayer(player.GetPlayerId(), offset);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAOMovePlayer(int playerId, vector offset)
	{
		PlayerController player = GetGame().GetPlayerManager().GetPlayerController(playerId);
		if (!player || !player.GetControlledEntity())
			return "UNAVAILABLE player";
		IEntity mover = CompartmentAccessComponent.GetVehicleIn(player.GetControlledEntity());
		if (!mover)
			mover = player.GetControlledEntity();
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(mover.FindComponent(SCR_EditableEntityComponent));
		if (!editable)
			return "UNAVAILABLE native editable mover";
		vector transform[4];
		mover.GetWorldTransform(transform);
		transform[3] = m_vGC_AOOrigin + offset;
		return string.Format("arranged=%1 destination=%2", editable.SetTransform(transform), transform[3]);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAOSnapshot()
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!player)
			return "UNAVAILABLE player";
		IEntity character = player.GetControlledEntity();
		string result = string.Format("phase=%1 player=%2 controlled=%3", GetLobbyPhase(), player.GetPlayerId(), character);
		if (character)
		{
			IEntity vehicle = CompartmentAccessComponent.GetVehicleIn(character);
			result += string.Format(" alive=%1 position=%2 offset=%3 vehicle=%4", SCR_AIDamageHandling.IsAlive(character), character.GetOrigin(), character.GetOrigin() - m_vGC_AOOrigin, vehicle);
			if (vehicle)
				result += string.Format(" vehiclePosition=%1", vehicle.GetOrigin());
		}
		TextWidget timer = TextWidget.Cast(GetGame().GetWorkspace().FindAnyWidget("AOTimer"));
		if (timer)
			result += " HUD=" + timer.GetText();
		else
			result += " HUD=absent";
		GC_AOLimitSystem limits = GC_LobbyGameMode.GetGameMode().GetAOLimits();
		if (limits)
			result += limits.GC_TestSnapshot(player.GetPlayerId());
		return result;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAOServerPlayer(int playerId)
	{
		IEntity character = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!character)
			return "UNAVAILABLE controlled entity";
		string result = string.Format("player=%1 alive=%2 position=%3 vehicle=%4 ", playerId, SCR_AIDamageHandling.IsAlive(character),
			character.GetOrigin(), CompartmentAccessComponent.GetVehicleIn(character));
		result += GC_LobbyGameMode.GetGameMode().GetAOLimits().GC_TestSnapshot(playerId);
		return result;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAOTimeFormat()
	{
		array<float> inputs = {20, 30, 65.25, 59.999, 60, 0, -1, 0.001};
		array<string> expected = {"20.000", "30.000", "1:05.250", "59.999", "1:00.000", "0.000", "0.000", "0.001"};
		string result;
		for (int i; i < inputs.Count(); i++)
		{
			string actual = GC_AOWarning.FormatTime(inputs[i]);
			result += string.Format(" [%1 actual=%2 pass=%3]", inputs[i], actual, actual == expected[i]);
		}
		return result;
	}
}

modded class GC_AOLimitSystem
{
	//------------------------------------------------------------------------------------------------
	string GC_TestSnapshot(int playerId)
	{
		GC_AOPlayerState state;
		if (!m_mPlayers.Find(playerId, state))
			return " AO=not-evaluated";
		string result = string.Format(" limits=%1 clocks=%2", m_aLimits.Count(), state.m_mDeadlines.Count());
		if (state.m_Selected)
			result += string.Format(" selected=%1 behavior=%2 priority=%3 remaining=%4", state.m_Selected.GetName(), state.m_Selected.GetBehavior(), state.m_Selected.GetPriority(), state.m_fDeadline - m_Mode.GetWorld().GetWorldTime() * 0.001);
		return result;
	}
}
