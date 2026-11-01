#ifdef WORKBENCH
//! Explicit local development session. Never enabled by loading the addon or opening Tab.
class GC_WorkbenchItemBrowser
{
	protected static IEntity s_Owner;
	protected static SCR_BaseGameMode s_Mode;
	protected static SCR_PlayerController s_Controller;

	//------------------------------------------------------------------------------------------------
	static bool Open(bool showInventory = true)
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
		GC_GearSession.Close();
		ResourceName editing = GC_PrefabAuthoringSession.GetEditingPrefab();
		Disable();
		s_Owner = owner;
		s_Mode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		s_Controller = controller;
		GC_PrefabAuthoringSession.SetEditingPrefab(editing);
		s_Controller.m_OnControlledEntityChanged.Insert(OnOwnerChanged);
		s_Mode.GetOnGameEnd().Insert(Disable);
		if (showInventory)
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
	}

	//------------------------------------------------------------------------------------------------
	static ResourceName GetEditingPrefab()
	{
		return GC_PrefabAuthoringSession.GetEditingPrefab();
	}

	//------------------------------------------------------------------------------------------------
	static void SetEditingPrefab(ResourceName prefab)
	{
		if (IsActive(s_Owner))
			GC_PrefabAuthoringSession.SetEditingPrefab(prefab);
	}

	//------------------------------------------------------------------------------------------------
	static bool LoadPrefab(ResourceName prefab)
	{
		if (!IsActive(s_Owner))
			return false;
		SCR_PlayerController controller = s_Controller;
		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return false;
		IEntitySource source = resource.GetResource().ToEntitySource();
		if (!source || !source.GetClassName().ToType().IsInherited(ChimeraCharacter))
			return false;
		EntitySpawnParams spawn = new EntitySpawnParams();
		spawn.TransformMode = ETransformMode.WORLD;
		IEntity previous = controller.GetControlledEntity();
		previous.GetWorldTransform(spawn.Transform);
		IEntity character = GetGame().SpawnEntityPrefabLocal(resource, GetGame().GetWorld(), spawn);
		if (!character)
			return false;
		RplComponent replication = RplComponent.Cast(character.FindComponent(RplComponent));
		if (!replication || !replication.Id().IsValid() || !character.FindComponent(SCR_InventoryStorageManagerComponent))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(character);
			return false;
		}
		controller.SetControlledEntity(character);
		if (controller.GetControlledEntity() != character)
		{
			controller.SetControlledEntity(previous);
			SCR_EntityHelper.DeleteEntityAndChildren(character);
			return false;
		}
		if (!Open(false))
		{
			controller.SetControlledEntity(previous);
			SCR_EntityHelper.DeleteEntityAndChildren(character);
			return false;
		}
		SCR_EntityHelper.DeleteEntityAndChildren(previous);
		SetEditingPrefab(prefab);
		// Let the old character's inventory and the file dialog finish closing.
		GetGame().GetCallqueue().CallLater(ReopenInventory, 1, false, character, 0);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static void ReopenInventory(IEntity owner, int attempt)
	{
		if (!IsActive(owner))
			return;
		MenuManager menus = GetGame().GetMenuManager();
		if (menus.FindMenuByPreset(ChimeraMenuPreset.Inventory20Menu) || menus.FindMenuByPreset(ChimeraMenuPreset.GC_LoadoutFilesMenu))
		{
			if (attempt < 40)
				GetGame().GetCallqueue().CallLater(ReopenInventory, 25, false, owner, attempt + 1);
			return;
		}
		Open();
	}
	//------------------------------------------------------------------------------------------------
	protected static void OnOwnerChanged(IEntity from, IEntity to)
	{
		if (to != s_Owner)
			Disable();
	}
}
#endif
