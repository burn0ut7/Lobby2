//! Explicit local pose-reconstruction probe; never runs automatically.
class GC_KilledFromPoseProbe : GC_SpectatorKilledFromVisualizer
{
	protected IEntity m_Source;
	protected int m_iPoseMode;

	//------------------------------------------------------------------------------------------------
	string Arrange(ResourceName prefab, int stance)
	{
		GetGame().GetCallqueue().Remove(AdvanceProbe);
		GetGame().GetCallqueue().Remove(DriveSource);
		DestroyPreview();
		if (m_Source)
			delete m_Source;
		m_World = GetGame().GetWorld();
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.AnglesToMatrix("180 0 0", params.Transform);
		params.Transform[3] = "1194 39 2730";
		m_Source = GetGame().SpawnEntityPrefabLocal(Resource.Load(prefab), m_World, params);
		ChimeraCharacter character = ChimeraCharacter.Cast(m_Source);
		if (!character)
			return "FAIL source spawn";
		AIControlComponent ai = AIControlComponent.Cast(character.FindComponent(AIControlComponent));
		if (ai)
			ai.DeactivateAI();
		ECharacterStanceChange change = ECharacterStanceChange.STANCECHANGE_TOERECTED;
		if (stance == 1)
			change = ECharacterStanceChange.STANCECHANGE_TOCROUCH;
		else if (stance == 2)
			change = ECharacterStanceChange.STANCECHANGE_TOPRONE;
		m_iPoseMode = stance;
		character.GetCharacterController().SetStanceChange(change);
		character.GetCharacterController().SetWeaponRaised(true);
		character.GetCharacterController().GetInputContext().SetWeaponADS(true);
		GetGame().GetCallqueue().CallLater(DriveSource, 0, true);
		return "ARRANGED local AI source; capture after stance settles";
	}

	//------------------------------------------------------------------------------------------------
	protected void DriveSource()
	{
		if (!m_Source)
			return;
		CharacterControllerComponent controller = ChimeraCharacter.Cast(m_Source).GetCharacterController();
		controller.SetWeaponRaised(true);
		controller.GetInputContext().SetWeaponADS(true);
		if (m_iPoseMode == 3)
			controller.GetInputContext().SetLean(0.8);
		if (m_iPoseMode == 4)
			controller.SetDynamicStance(0.5);
	}

	//------------------------------------------------------------------------------------------------
	string CaptureProbe()
	{
		if (!m_Source)
			return "UNAVAILABLE source";
		GetGame().GetCallqueue().Remove(AdvanceProbe);
		GetGame().GetCallqueue().Remove(DriveSource);
		DestroyPreview();
		m_Snapshot = new GC_KilledFromSnapshot();
		m_Snapshot.CaptureDeathFallback(m_Source, m_Source);
		m_Snapshot.GC_TestOffsetPreviews();
		CreatePreview();
		CharacterAnimationComponent.Cast(m_Source.FindComponent(CharacterAnimationComponent)).Deactivate(m_Source);
		GetGame().GetCallqueue().CallLater(AdvanceProbe, 0, true);
		return ReadProbe();
	}

	//------------------------------------------------------------------------------------------------
	protected void AdvanceProbe()
	{
		UpdatePose();
		if (m_bPoseFrozen)
			GetGame().GetCallqueue().Remove(AdvanceProbe);
	}

	//------------------------------------------------------------------------------------------------
	string ViewProbe()
	{
		if (!m_Source || !m_KillerPreviewEntity || !m_VictimPreviewEntity)
			return "UNAVAILABLE actors";
		GetGame().GetMenuManager().CloseAllMenus();
		return string.Format("LEFT bones=%1 CENTER source=%2 RIGHT victim=%3; ", m_KillerPreviewEntity.GetOrigin(), m_Source.GetOrigin(), m_VictimPreviewEntity.GetOrigin()) + ReadProbe();
	}

	//------------------------------------------------------------------------------------------------
	string RemoveHelmetProbe()
	{
		if (!m_Source)
			return "UNAVAILABLE source";
		InventoryStorageManagerComponent inventory = InventoryStorageManagerComponent.Cast(m_Source.FindComponent(InventoryStorageManagerComponent));
		array<IEntity> hierarchy = {m_Source};
		SCR_EntityHelper.GetHierarchyEntityList(m_Source, hierarchy);
		foreach (IEntity item : hierarchy)
		{
			EntityPrefabData prefab = item.GetPrefabData();
			if (!prefab)
				continue;
			string path = prefab.GetPrefabName();
			path.ToLower();
			if (path.Contains("helmet"))
				return string.Format("removed=%1 prefab=%2", inventory.TryDeleteItem(item), path);
		}
		return "UNAVAILABLE helmet";
	}

	//------------------------------------------------------------------------------------------------
	string DamageProbe(int playerId, float amount = 1000)
	{
		PlayerController localPlayer = GetGame().GetPlayerController();
		if (!Replication.IsServer() || !localPlayer || !SCR_Global.IsAdmin(localPlayer.GetPlayerId()) || !m_Source)
			return "UNAVAILABLE authority/admin/source";
		ChimeraCharacter target = ChimeraCharacter.Cast(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId));
		if (!target || target.FindComponent(GC_LobbySpectatorBodyComponent))
			return "UNAVAILABLE playable target";
		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(target.GetDamageManager());
		vector hit[3];
		hit[0] = target.GetOrigin() + "0 1.5 0";
		SCR_DamageContext context = new SCR_DamageContext(EDamageType.MELEE, amount, hit, target, damage.GetHeadHitZone(), Instigator.CreateInstigator(m_Source), null, -1, -1);
		context.damageEffect = new SCR_MeleeDamageEffect();
		damage.HandleDamage(context);
		return "DISPATCHED native damage from local AI; source is intentionally absent on remote owner";
	}

	//------------------------------------------------------------------------------------------------
	string UnarmProbe()
	{
		if (!m_Source)
			return "UNAVAILABLE source";
		GetGame().GetCallqueue().Remove(DriveSource);
		CharacterControllerComponent controller = ChimeraCharacter.Cast(m_Source).GetCharacterController();
		return "requested=" + controller.TryEquipRightHandItem(null, EEquipItemType.EEquipTypeUnarmedDeliberate, true);
	}

	//------------------------------------------------------------------------------------------------
	string PublishProbe(int playerId)
	{
		PlayerController localPlayer = GetGame().GetPlayerController();
		if (!Replication.IsServer() || !localPlayer || !SCR_Global.IsAdmin(localPlayer.GetPlayerId()) || !m_Snapshot)
			return "UNAVAILABLE authority/admin/snapshot";
		GC_LobbyPlayerController recipient = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!recipient)
			return "UNAVAILABLE recipient";
		recipient.SendKilledFromSnapshot(m_Snapshot);
		return "DISPATCHED production snapshot publication; inspect owner receipt";
	}

	//------------------------------------------------------------------------------------------------
	string RoundTripProbe()
	{
		if (!m_Snapshot)
			return "UNAVAILABLE snapshot";
		string json = GC_KilledFromSnapshot.Serialize(m_Snapshot);
		GC_KilledFromSnapshot decoded = GC_KilledFromSnapshot.Deserialize(json);
		if (!decoded)
			return "FAIL roundtrip bytes=" + json.Length();
		DestroyPreview();
		m_Snapshot = decoded;
		CreatePreview();
		GetGame().GetCallqueue().CallLater(AdvanceProbe, 0, true);
		return string.Format("bytes=%1 decoded=%2; reconstruction scheduled; read after native frame", json.Length(), decoded.GetDebugReport());
	}

	//------------------------------------------------------------------------------------------------
	string ReadProbe()
	{
		if (!m_Snapshot)
			return "UNAVAILABLE snapshot";
		return "killer " + ComparePose(m_KillerPreviewEntity, m_Snapshot.GetKillerBonePose())
			+ " victim " + ComparePose(m_VictimPreviewEntity, m_Snapshot.GetVictimBonePose())
			+ " weapon " + CompareWeapon(m_KillerPreviewEntity, m_Snapshot.GetKillerWeapon())
			+ string.Format(" gearKiller=%1 gearVictim=%2", CompareGear(m_KillerPreviewEntity, m_Snapshot.GetKillerLoadout()), CompareGear(m_VictimPreviewEntity, m_Snapshot.GetVictimLoadout()));
	}

	//------------------------------------------------------------------------------------------------
	protected bool CompareGear(IEntity actor, string expected)
	{
		if (!actor)
			return false;
		JsonSaveContext context = new JsonSaveContext();
		if (!SCR_PlayerArsenalLoadout.ReadLoadoutString(actor, context))
			return false;
		return GC_KilledFromGearProbe.Signature(context.SaveToString()) == GC_KilledFromGearProbe.Signature(expected);
	}

	//------------------------------------------------------------------------------------------------
	protected string CompareWeapon(IEntity actor, GC_KilledFromWeaponState state)
	{
		if (!actor || !state)
			return "missing";
		BaseWeaponManagerComponent manager = BaseWeaponManagerComponent.Cast(actor.FindComponent(BaseWeaponManagerComponent));
		if (manager && !state.IsValid())
		{
			if (manager.GetCurrentWeapon())
				return "FAIL unexpected held weapon";
			return "unarmed-match";
		}
		if (!manager || !manager.GetCurrentWeapon())
			return "unequipped";
		vector root[4];
		vector actual[4];
		vector expected[4];
		vector relative[4];
		actor.GetWorldTransform(root);
		manager.GetCurrentWeapon().GetOwner().GetWorldTransform(actual);
		Math3D.MatrixInvMultiply4(root, actual, relative);
		state.GetActorRelativeTransform(expected);
		float basisError;
		for (int axis; axis < 3; axis++)
			basisError = Math.Max(basisError, vector.Distance(relative[axis], expected[axis]));
		array<IEntity> weapons = {};
		manager.GetWeaponsList(weapons);
		return string.Format("positionErrorM=%1 basisError=%2 index=%3 expectedIndex=%4", vector.Distance(relative[3], expected[3]), basisError, weapons.Find(manager.GetCurrentWeapon().GetOwner()), state.GetManagedWeaponIndex());
	}

	//------------------------------------------------------------------------------------------------
	protected string ComparePose(IEntity actor, array<ref GC_KilledFromBonePose> pose)
	{
		if (!actor || !pose || !actor.GetAnimation())
			return "missing actor/pose";
		float maximumPositionError;
		float maximumBasisError;
		float maximumLocalError;
		int count;
		foreach (GC_KilledFromBonePose bone : pose)
		{
			vector matrix[4];
			if (!actor.GetAnimation().GetBoneMatrix(actor.GetAnimation().GetBoneIndex(bone.m_sBoneName), matrix))
				continue;
			vector expected[4];
			if (!m_Source.GetAnimation().GetBoneMatrix(m_Source.GetAnimation().GetBoneIndex(bone.m_sBoneName), expected))
				continue;
			count++;
			vector localMatrix[4];
			actor.GetAnimation().GetBoneLocalMatrix(actor.GetAnimation().GetBoneIndex(bone.m_sBoneName), localMatrix);
			maximumLocalError = Math.Max(maximumLocalError, vector.Distance(localMatrix[3], bone.m_aLocalTransform[3]));
			maximumPositionError = Math.Max(maximumPositionError, vector.Distance(matrix[3], expected[3]));
			for (int axis; axis < 3; axis++)
				maximumBasisError = Math.Max(maximumBasisError, vector.Distance(matrix[axis], expected[axis]));
		}
		return string.Format("bones=%1 maxPositionM=%2 maxBasis=%3 maxLocalM=%4", count, maximumPositionError, maximumBasisError, maximumLocalError);
	}

	//------------------------------------------------------------------------------------------------
	void Cleanup()
	{
		GetGame().GetCallqueue().Remove(AdvanceProbe);
		GetGame().GetCallqueue().Remove(DriveSource);
		DestroyPreview();
		if (m_Source)
			delete m_Source;
		m_Source = null;
		m_Snapshot = null;
	}
}

modded class GC_KilledFromSnapshot
{
	//------------------------------------------------------------------------------------------------
	void GC_TestOffsetPreviews()
	{
		m_aKillerRootTransform[3] = m_aKillerRootTransform[3] + "-2 0 0";
		m_aVictimRootTransform[3] = m_aVictimRootTransform[3] + "2 0 0";
	}
}

modded class GC_LobbyManager
{
	protected ref GC_KilledFromPoseProbe m_GC_KilledFromProbe;

	//------------------------------------------------------------------------------------------------
	string GC_TestKilledFromPose(string action, int stance)
	{
		if (GetManager() != this || !GetGame().GetPlayerController())
			return "UNAVAILABLE local Lobby2";
		if (!m_GC_KilledFromProbe)
			m_GC_KilledFromProbe = new GC_KilledFromPoseProbe();
		if (action == "arrange" || action == "arrange-lat")
		{
			GC_Slot slot = m_Roster.GetSlot(1);
			if (action == "arrange-lat")
				slot = m_Roster.GetSlot(2);
			if (!slot || stance < 0 || stance > 4)
				return "UNAVAILABLE slot or stance";
			return m_GC_KilledFromProbe.Arrange(slot.GetResolvedPrefab(), stance);
		}
		if (action == "capture")
			return m_GC_KilledFromProbe.CaptureProbe();
		if (action == "gear")
			return m_GC_KilledFromProbe.RemoveHelmetProbe();
		if (action == "damage")
			return m_GC_KilledFromProbe.DamageProbe(stance);
		if (action == "publish")
			return m_GC_KilledFromProbe.PublishProbe(stance);
		if (action == "roundtrip")
			return m_GC_KilledFromProbe.RoundTripProbe();
		if (action == "unarmed")
			return m_GC_KilledFromProbe.UnarmProbe();
		if (action == "wound")
			return m_GC_KilledFromProbe.DamageProbe(stance, 1);
		if (action == "read")
			return m_GC_KilledFromProbe.ReadProbe();
		if (action == "view")
			return m_GC_KilledFromProbe.ViewProbe();
		if (action == "cleanup")
		{
			m_GC_KilledFromProbe.Cleanup();
			return "CLEANED";
		}
		return "REJECTED action";
	}
}


modded class GC_LobbyPlayerController
{
	//------------------------------------------------------------------------------------------------
	string GC_TestKilledFromOwner(string action)
	{
		if (!GC_LobbyGameMode.GetGameMode() || GetGame().GetPlayerController() != this)
			return "UNAVAILABLE owning Lobby2 player";
		if (action == "on")
			SetSpectatorKilledFromEnabled(true);
		else if (action == "off")
			SetSpectatorKilledFromEnabled(false);
		else if (action == "gear" && m_SpectatorMarkerOverlay)
			return m_SpectatorMarkerOverlay.GC_TestKilledFromGear();
		if (!m_KilledFromSnapshot)
			return string.Format("snapshot=none sequence=%1 parts=%2/%3", m_iKilledFromReceiveSequence, m_iKilledFromNextPart, m_iKilledFromPartCount);
		return string.Format("enabled=%1 sequence=%2 parts=%3/%4 snapshot={%5}", m_bSpectatorKilledFromEnabled, m_iKilledFromReceiveSequence, m_iKilledFromNextPart, m_iKilledFromPartCount, m_KilledFromSnapshot.GetDebugReport());
	}
}

modded class GC_SpectatorMarkerOverlay
{
	//------------------------------------------------------------------------------------------------
	string GC_TestKilledFromGear()
	{
		if (!m_KilledFromVisualizer)
			return "UNAVAILABLE visualizer";
		return m_KilledFromVisualizer.GC_TestGear();
	}
}

modded class GC_SpectatorKilledFromVisualizer
{
	//------------------------------------------------------------------------------------------------
	string GC_TestGear()
	{
		if (!m_KillerPreviewEntity || !m_Snapshot)
			return "UNAVAILABLE preview";
		JsonSaveContext context = new JsonSaveContext();
		SCR_PlayerArsenalLoadout.ReadLoadoutString(m_KillerPreviewEntity, context);
		string actual = context.SaveToString();
		string report = string.Format("gearEqual=%1 actualBytes=%2 expectedBytes=%3", GC_KilledFromGearProbe.Signature(actual) == GC_KilledFromGearProbe.Signature(m_Snapshot.GetKillerLoadout()), actual.Length(), m_Snapshot.GetKillerLoadout().Length());
		string expected = m_Snapshot.GetKillerLoadout();
		for (int i; i < Math.Min(actual.Length(), expected.Length()); i++)
		{
			if (actual.Substring(i, 1) == expected.Substring(i, 1))
				continue;
			int start = Math.Max(0, i - 30);
			report += string.Format(" firstDifference=%1 actual=%2 expected=%3", i, actual.Substring(start, Math.Min(100, actual.Length() - start)), expected.Substring(start, Math.Min(100, expected.Length() - start)));
			break;
		}
		array<IEntity> hierarchy = {};
		SCR_EntityHelper.GetHierarchyEntityList(m_KillerPreviewEntity, hierarchy);
		foreach (IEntity item : hierarchy)
		{
			string path = SCR_ResourceNameUtils.GetPrefabName(item);
			path.ToLower();
			if (path.Contains("helmet"))
				report += " helmet=" + path;
		}
		return report;
	}
}

//! Compare the recursive equipment tree independent of native storage ordering.
class GC_KilledFromGearProbe : SCR_PlayerArsenalLoadout
{
	//------------------------------------------------------------------------------------------------
	static string Signature(string data)
	{
		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromString(data) || !context.StartObject(ARSENALLOADOUT_KEY))
			return "INVALID " + data;
		array<string> entries = {};
		if (!Collect(context, "character", entries))
			return "INVALID " + data;
		entries.Sort();
		string signature;
		foreach (string entry : entries)
			signature += entry + "\n";
		return signature;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool Collect(LoadContext context, string path, array<string> entries)
	{
		ResourceName prefab;
		if (!context.Read(prefab))
			return false;
		entries.Insert(path + "=" + prefab);
		int count;
		if (!context.StartArray("storages", count))
			return true;
		for (int i; i < count; i++)
		{
			if (!context.StartObject())
				return false;
			string id;
			if (!context.Read(id))
				return false;
			int slots;
			if (!context.StartMap("slots", slots))
				return false;
			for (int j; j < slots; j++)
			{
				string key;
				if (!context.ReadMapKey(j, key) || !context.StartObject(key))
					return false;
				if (!Collect(context, path + "/" + id + "[" + key + "]", entries) || !context.EndObject())
					return false;
			}
			if (!context.EndMap() || !context.EndObject())
				return false;
		}
		return context.EndArray();
	}
}
