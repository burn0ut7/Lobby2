#ifdef WORKBENCH
//! Explicit native-world playback fixtures; production collectors own all records.
modded class GC_LobbyManager
{
	protected IEntity m_GC_PlaybackTarget;

	//------------------------------------------------------------------------------------------------
	string GC_TestPlaybackMapInfo()
	{
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity)
			return "unavailable-map";
		return string.Format("world=%1 size=%2 offset=%3", GetGame().GetWorldFile(), mapEntity.Size(), mapEntity.Offset());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestPlaybackTerrain(string action, vector location)
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!Replication.IsServer() || GetManager() != this || !player || !AuthorizeAdminAction(player.GetPlayerId(), "playback-terrain-test", action))
			return "denied";
		ChimeraCharacter character = ChimeraCharacter.Cast(player.GetControlledEntity());
		if (!character || character.FindComponent(GC_LobbySpectatorBodyComponent))
			return "unavailable-playing-character";
		if (action == "place")
		{
			location[1] = GetGame().GetWorld().GetSurfaceY(location[0], location[2]) + 0.2;
			character.SetOrigin(location);
			character.GetCharacterController().SetHeadingAngle(0, true);
		}
		else if (action == "target")
		{
			if (m_GC_PlaybackTarget)
				return "target-exists";
			EntitySpawnParams parameters = new EntitySpawnParams();
			parameters.TransformMode = ETransformMode.WORLD;
			location = character.GetOrigin() + Vector(8, 0, 8);
			location[1] = GetGame().GetWorld().GetSurfaceY(location[0], location[2]) + 0.2;
			parameters.Transform[3] = location;
			m_GC_PlaybackTarget = GetGame().SpawnEntityPrefab(Resource.Load("{BF643BE4ADBDFDD3}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_LAT.et"), GetGame().GetWorld(), parameters);
			if (!m_GC_PlaybackTarget)
				return "spawn-failed";
		}
		else if (action == "crouch")
			character.GetCharacterController().SetStanceChange(ECharacterStanceChange.STANCECHANGE_TOCROUCH);
		else if (action == "prone")
			character.GetCharacterController().SetStanceChange(ECharacterStanceChange.STANCECHANGE_TOPRONE);
		else if (action == "stand")
			character.GetCharacterController().SetStanceChange(ECharacterStanceChange.STANCECHANGE_TOERECTED);
		else if (action == "walk")
			SCR_CharacterControllerComponent.Cast(character.GetCharacterController()).GC_TestAOWalk(10);
		else if (action == "status")
		{
			return string.Format("position=%1 stance=%2 target=%3", character.GetOrigin(), character.GetCharacterController().GetStance(), m_GC_PlaybackTarget != null);
		}
		else
			return "unsupported-action";
		GC_AdminActionLogger.Log(player.GetPlayerId(), "playback-terrain-test", action, "dispatched");
		return string.Format("dispatched=%1 position=%2", action, character.GetOrigin());
	}
}
#endif
