//! Optional peer/Workbench observations; no automatic execution or role grants.
modded class GC_LobbyPlayerController
{
	//------------------------------------------------------------------------------------------------
	//! Tests the production framing primitive; does not claim RPC authorization coverage.
	string GC_TestFollowFraming(IEntity target)
	{
		if (!CanUseAdminFollowCamera() || !target)
			return "UNAVAILABLE spectator camera/target";
		vector before = m_SpectatorCamera.GetOrigin();
		bool applied = m_SpectatorCamera.TryFollowPlayer(target);
		return string.Format("applied=%1 mode=%2 targetMatches=%3 before=%4 after=%5 target=%6",
			applied, m_SpectatorCamera.GetMode(), m_SpectatorCamera.GetTarget() == target,
			before, m_SpectatorCamera.GetOrigin(), target.GetOrigin());
	}

	//------------------------------------------------------------------------------------------------
	//! Exercise the ordinary production context action, not admin auto-framing.
	string GC_TestManualFollow(IEntity target)
	{
		if (!m_SpectatorCamera || !target || GetGame().GetPlayerController() != this)
			return "UNAVAILABLE local camera/target";
		m_SpectatorCamera.TrySetMode(GC_ESpectatorCameraMode.FREE);
		vector before[4];
		Math3D.AnglesToMatrix("67 12 0", before);
		before[3] = target.GetOrigin() + "7 4 -9";
		m_SpectatorCamera.SetTransform(before);
		GC_SpectatorCameraContextAction action = new GC_SpectatorCameraContextAction(
			m_SpectatorCamera, GC_ESpectatorCameraMode.FOLLOW, "Follow");
		GC_ContextMenuContext context = GC_ContextMenuContext.ForEntity(target);
		if (!action.CanExecute(context))
			return "UNAVAILABLE production context action";
		action.Execute(context);
		vector after[4];
		m_SpectatorCamera.GetWorldTransform(after);
		float basisError;
		for (int i; i < 3; i++)
			basisError = Math.Max(basisError, vector.Distance(before[i], after[i]));
		return string.Format("mode=%1 targetMatches=%2 positionError=%3 basisError=%4 position=%5",
			m_SpectatorCamera.GetMode(), m_SpectatorCamera.GetTarget() == target,
			vector.Distance(before[3], after[3]), basisError, after[3]);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestFollowSnapshot()
	{
		if (!m_SpectatorCamera)
			return "camera=none";
		int targetId = -1;
		if (m_SpectatorCamera.GetTarget())
			targetId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(m_SpectatorCamera.GetTarget());
		return string.Format("mode=%1 targetPlayer=%2 position=%3 eligible=%4 pendingReceipt=%5", m_SpectatorCamera.GetMode(), targetId, m_SpectatorCamera.GetOrigin(), CanUseAdminFollowCamera(), m_bAdminFollowPending);
	}
}

modded class GC_LobbyManager
{
	protected IEntity m_GC_TestFollowWall;

	//------------------------------------------------------------------------------------------------
	string GC_TestFollowWall(int slotId)
	{
		GC_Slot slot = GetSlot(slotId);
		if (!Replication.IsServer() || !slot || !slot.GetEntity() || m_GC_TestFollowWall)
			return "UNAVAILABLE authority/slot or wall already exists";
		Resource prefab = Resource.Load("{9A25B3AF92191E1F}Prefabs/Structures/Walls/Concrete/ConcreteWall_USSR_01/ConcreteWall_USSR_01_single.et");
		if (!prefab || !prefab.IsValid())
			return "UNAVAILABLE stock wall prefab";
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		slot.GetEntity().GetWorldTransform(params.Transform);
		params.Transform[3] = slot.GetEntity().GetOrigin() - params.Transform[2] * 2;
		m_GC_TestFollowWall = GetGame().SpawnEntityPrefab(prefab, GetGame().GetWorld(), params);
		if (!m_GC_TestFollowWall)
			return "FAILED wall spawn";
		vector boundsMin, boundsMax;
		m_GC_TestFollowWall.GetBounds(boundsMin, boundsMax);
		return string.Format("wall=%1 boundsMin=%2 boundsMax=%3 target=%4",
			m_GC_TestFollowWall.GetOrigin(), boundsMin, boundsMax, slot.GetEntity().GetOrigin());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestFollowFramingSlot(int slotId)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		GC_Slot slot = GetSlot(slotId);
		if (!controller || !slot || !slot.GetEntity())
			return "UNAVAILABLE controller/slot entity";
		return controller.GC_TestFollowFraming(slot.GetEntity());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestManualFollowSlot(int slotId)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		GC_Slot slot = GetSlot(slotId);
		if (!controller || !slot || !slot.GetEntity())
			return "UNAVAILABLE controller/slot entity";
		return controller.GC_TestManualFollow(slot.GetEntity());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminFollowPlayer(int targetId)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return "no-local-controller";
		controller.RequestAdminFollowPlayer(targetId, GetPlayerVoiceIdentity(targetId));
		return "dispatched; inspect snapshot and central audit";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminFollowSnapshot()
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return "no-local-controller";
		return controller.GC_TestFollowSnapshot();
	}
}
