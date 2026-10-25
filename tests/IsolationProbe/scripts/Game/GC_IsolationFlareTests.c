//! The same vanilla flare callback and observation run with and without Lobby2.
#ifdef WORKBENCH
modded class SCR_FlareAnimationComponent
{
	//------------------------------------------------------------------------------------------------
	void GC_TestFlareInput()
	{
		// Exercise the input listener, never a muzzle or trigger bypass.
		FireFlare();
	}
}

modded class SCR_CharacterControllerComponent
{
	protected IEntity m_GC_TestFlare;

	//------------------------------------------------------------------------------------------------
	string GC_TestFlare(string action)
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!player || player.GetControlledEntity() != GetOwner())
			return "UNAVAILABLE local controlled character";
		InventoryStorageManagerComponent inventory = GetInventoryStorageManager();
		if (!inventory)
			return "UNAVAILABLE inventory";
		if (action == "spawn")
		{
			ResourceName prefab = Workbench.GetResourceName("Prefabs/Weapons/Flares/Flare_RSP30_white.et");
			if (prefab.IsEmpty())
				return "UNAVAILABLE flare resource";
			return string.Format("spawn=%1 prefab=%2", inventory.TrySpawnPrefabToStorage(prefab), prefab);
		}
		if (action == "equip")
		{
			array<IEntity> items = {};
			inventory.GetItems(items);
			foreach (IEntity item : items)
			{
				if (!item.FindComponent(SCR_FlareAnimationComponent))
					continue;
				BaseWeaponComponent weapon = BaseWeaponComponent.Cast(item.FindComponent(BaseWeaponComponent));
				if (!weapon || !weapon.GetCurrentMagazine() || weapon.GetCurrentMagazine().GetAmmoCount() == 0)
					continue;
				m_GC_TestFlare = item;
				return string.Format("equip=%1", TryEquipRightHandItem(item, EEquipItemType.EEquipTypeWeapon, true));
			}
			return "UNAVAILABLE loaded flare";
		}
		if (!m_GC_TestFlare)
			return "UNAVAILABLE fixture flare";
		if (action == "raise")
			SetWeaponRaised(true);
		else if (action == "fire")
			SCR_FlareAnimationComponent.Cast(m_GC_TestFlare.FindComponent(SCR_FlareAnimationComponent)).GC_TestFlareInput();
		else if (action != "snapshot")
			return "REJECTED action";
		BaseWeaponComponent flare = BaseWeaponComponent.Cast(m_GC_TestFlare.FindComponent(BaseWeaponComponent));
		int ammo = -1;
		if (flare && flare.GetCurrentMagazine())
			ammo = flare.GetCurrentMagazine().GetAmmoCount();
		return string.Format("ammo=%1 raised=%2 usingItem=%3 canFireWeapon=%4 currentFlare=%5", ammo,
			IsWeaponRaised(), IsUsingItem(), GetCanFireWeapon(), GetWeaponManagerComponent().GetCurrentWeapon() == flare);
	}
}

modded class GC_IsolationProbeComponent
{
	//------------------------------------------------------------------------------------------------
	string FlareInput(string action)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(GetGame().GetPlayerController().GetControlledEntity());
		if (!character || character.GetName() != "IsolationCharacter")
			return "UNAVAILABLE fixture character";
		return SCR_CharacterControllerComponent.Cast(character.GetCharacterController()).GC_TestFlare(action);
	}
}
#endif
