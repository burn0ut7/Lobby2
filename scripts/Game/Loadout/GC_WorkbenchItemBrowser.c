#ifdef WORKBENCH
//! Explicit local development session. Never enabled by loading the addon or opening Tab.
class GC_WorkbenchItemBrowser
{
	protected static IEntity s_Owner;
	protected static SCR_BaseGameMode s_Mode;
	protected static SCR_PlayerController s_Controller;
	protected static ResourceName s_sEditingPrefab;

	//------------------------------------------------------------------------------------------------
	static bool Open()
	{
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (RplSession.Mode() != RplMode.None || !GC_LobbyGameMode.GetGameMode() || !controller)
			return false;
		IEntity owner = controller.GetControlledEntity();
		if (!owner)
			return false;
		SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!manager)
			return false;
		ResourceName editing = SCR_ResourceNameUtils.GetPrefabName(owner);
		if (s_Owner == owner && IsActive(owner) && !s_sEditingPrefab.IsEmpty())
			editing = s_sEditingPrefab;
		Disable();
		s_Owner = owner;
		s_Mode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		s_Controller = controller;
		s_sEditingPrefab = editing;
		s_Controller.m_OnControlledEntityChanged.Insert(OnOwnerChanged);
		s_Mode.GetOnGameEnd().Insert(Disable);
		manager.OpenInventory();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsActive(IEntity owner)
	{
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!s_Owner || !s_Mode || RplSession.Mode() != RplMode.None || !GC_LobbyGameMode.GetGameMode() || GetGame().GetGameMode() != s_Mode || !controller || controller.GetControlledEntity() != s_Owner)
		{
			Disable();
			return false;
		}
		return owner == s_Owner;
	}

	//------------------------------------------------------------------------------------------------
	static void Disable()
	{
		if (s_Controller)
			s_Controller.m_OnControlledEntityChanged.Remove(OnOwnerChanged);
		if (s_Mode)
			s_Mode.GetOnGameEnd().Remove(Disable);
		s_Controller = null;
		s_Owner = null;
		s_Mode = null;
		s_sEditingPrefab = string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	static ResourceName GetEditingPrefab()
	{
		return s_sEditingPrefab;
	}

	//------------------------------------------------------------------------------------------------
	static void SetEditingPrefab(ResourceName prefab)
	{
		if (IsActive(s_Owner))
			s_sEditingPrefab = prefab;
	}

	//------------------------------------------------------------------------------------------------
	static bool LoadPrefab(ResourceName prefab)
	{
		if (!IsActive(s_Owner))
			return false;
		return GC_LoadoutRefresh.Request(prefab, s_Owner);
	}

	//------------------------------------------------------------------------------------------------
	//! Called after the Workbench preview has restarted with a fresh resource database.
	static bool ResumePrefab(ResourceName prefab, vector position, vector angles)
	{
		if (RplSession.Mode() != RplMode.None || !GC_LobbyGameMode.GetGameMode())
			return false;
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!controller || !controller.GetControlledEntity())
			return false;
		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return false;
		IEntitySource source = resource.GetResource().ToEntitySource();
		if (!source || !source.GetClassName().ToType().IsInherited(ChimeraCharacter))
			return false;
		EntitySpawnParams spawn = new EntitySpawnParams();
		spawn.TransformMode = ETransformMode.WORLD;
		Math3D.AnglesToMatrix(angles, spawn.Transform);
		spawn.Transform[3] = position;
		IEntity previous = controller.GetControlledEntity();
		IEntity character = GetGame().SpawnEntityPrefabLocal(resource, GetGame().GetWorld(), spawn);
		if (!character)
			return false;
		controller.SetControlledEntity(character);
		if (controller.GetControlledEntity() != character)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(character);
			return false;
		}
		SCR_EntityHelper.DeleteEntityAndChildren(previous);
		if (!Open())
			return false;
		SetEditingPrefab(prefab);
		return true;
	}
	//------------------------------------------------------------------------------------------------
	protected static void OnOwnerChanged(IEntity from, IEntity to)
	{
		if (to != s_Owner)
			Disable();
	}
}
#endif
