//! Explicit geometry/transport fixture; AI is not a production group-selection substitute.
modded class GC_LobbyManager
{
	protected ChimeraCharacter m_GC_JipBehindTarget;

	string GC_TestJipBehind(string action, int yaw)
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!mode || mode.GetState() != SCR_EGameModeState.GAME || !controller
			|| !Replication.IsServer() || !SCR_Global.IsAdmin(controller.GetPlayerId()))
			return "UNAVAILABLE authenticated local admin in GAME";
		ChimeraCharacter character = ChimeraCharacter.Cast(controller.GetControlledEntity());
		if (!character)
			return "UNAVAILABLE character";
		if (action == "setup")
		{
			if (m_GC_JipBehindTarget)
				return "UNAVAILABLE existing fixture";
			EntitySpawnParams params = new EntitySpawnParams();
			params.TransformMode = ETransformMode.WORLD;
			params.Transform[3] = character.GetOrigin() + "10 0 0";
			m_GC_JipBehindTarget = ChimeraCharacter.Cast(GetGame().SpawnEntityPrefab(Resource.Load("{26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et"), GetGame().GetWorld(), params));
			if (!m_GC_JipBehindTarget)
				return "FAIL spawn";
			AIControlComponent.Cast(m_GC_JipBehindTarget.FindComponent(AIControlComponent)).DeactivateAI();
		}
		if (!m_GC_JipBehindTarget)
			return "UNAVAILABLE fixture";
		CharacterControllerComponent targetControl = m_GC_JipBehindTarget.GetCharacterController();
		if (action == "stand")
			targetControl.SetStanceChange(ECharacterStanceChange.STANCECHANGE_TOERECTED);
		else if (action == "crouch")
			targetControl.SetStanceChange(ECharacterStanceChange.STANCECHANGE_TOCROUCH);
		else if (action == "prone")
			targetControl.SetStanceChange(ECharacterStanceChange.STANCECHANGE_TOPRONE);
		else if (action == "face")
			m_GC_JipBehindTarget.SetYawPitchRoll(Vector(yaw, 0, 0));
		vector destination;
		bool found = GC_JipSystem.FindBehindDestination(character, m_GC_JipBehindTarget, destination);
		if (action == "move" && found)
			controller.GC_DispatchJip(Replication.FindItemId(character), destination, targetControl.GetStance());
		vector transform[4];
		m_GC_JipBehindTarget.GetWorldTransform(transform);
		vector offset = character.GetOrigin() - m_GC_JipBehindTarget.GetOrigin();
		vector planned = destination - m_GC_JipBehindTarget.GetOrigin();
		return string.Format("found=%1 plannedForward=%2 plannedSide=%3 actualForward=%4 actualSide=%5 targetStance=%6 playerStance=%7",
			found, vector.Dot(planned, transform[2]), vector.Dot(planned, transform[0]), vector.Dot(offset, transform[2]),
			vector.Dot(offset, transform[0]), targetControl.GetStance(), character.GetCharacterController().GetStance());
	}
}
