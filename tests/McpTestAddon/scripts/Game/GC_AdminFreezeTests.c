//! Bounded native-action fixture. It does not call a weapon/muzzle Fire method.
modded class SCR_CharacterControllerComponent
{
	protected float m_fGC_TestFireRemaining;
	protected bool m_bGC_TestFireRelease;
	protected string m_sGC_TestCombatAction = "CharacterFire";
	protected float m_fGC_TestCombatObserve;
	protected int m_iGC_TestThrowFrames;
	protected int m_iGC_TestMeleeFrames;
	protected int m_iGC_TestThrowInputFrames;

	//------------------------------------------------------------------------------------------------
	void GC_TestFireInput(float seconds)
	{
		m_sGC_TestCombatAction = "CharacterFire";
		m_fGC_TestFireRemaining = Math.Clamp(seconds, 0, 2);
		m_bGC_TestFireRelease = true;
	}

	//------------------------------------------------------------------------------------------------
	//! Inject only native input; observe the engine command state on later frames.
	bool GC_TestCombatInput(string action)
	{
		if (action != "CharacterThrowGrenade" && action != "CharacterMelee" && action != "CharacterFire")
			return false;
		if (m_fGC_TestCombatObserve > 0 || m_fGC_TestFireRemaining > 0)
			return false;
		m_sGC_TestCombatAction = action;
		m_fGC_TestFireRemaining = 0.5;
		m_bGC_TestFireRelease = true;
		m_fGC_TestCombatObserve = 3;
		m_iGC_TestThrowFrames = 0;
		m_iGC_TestMeleeFrames = 0;
		m_iGC_TestThrowInputFrames = 0;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestCombatSnapshot()
	{
		BaseWeaponManagerComponent weapons = GetWeaponManagerComponent();
		InventoryStorageManagerComponent inventory = InventoryStorageManagerComponent.Cast(GetOwner().FindComponent(InventoryStorageManagerComponent));
		int grenadeCount = -1;
		if (inventory)
			grenadeCount = inventory.GetGrenadesCount();
		string result = string.Format("action=%1 observing=%2 throwingFrames=%3 meleeFrames=%4 grenadeSelected=%5 inventoryGrenades=%6",
			m_sGC_TestCombatAction, m_fGC_TestCombatObserve > 0, m_iGC_TestThrowFrames,
			m_iGC_TestMeleeFrames, weapons && weapons.GetCurrentGrenade(), grenadeCount);
		return result + string.Format(" nativeThrowInputFrames=%1 raised=%2 canFire=%3",
			m_iGC_TestThrowInputFrames, IsWeaponRaised(), CanFire());
	}

	//------------------------------------------------------------------------------------------------
	override void OnPrepareControls(IEntity owner, ActionManager am, float dt, bool player)
	{
		if ((GC_LobbyManager.GetManager() || owner.GetName() == "IsolationCharacter") && player && GetGame().GetPlayerController() && GetGame().GetPlayerController().GetControlledEntity() == owner)
		{
			if (m_fGC_TestFireRemaining > 0)
			{
				am.SetActionValue(m_sGC_TestCombatAction, 1);
				m_fGC_TestFireRemaining -= dt;
			}
			else if (m_bGC_TestFireRelease)
			{
				am.SetActionValue(m_sGC_TestCombatAction, 0);
				m_bGC_TestFireRelease = false;
			}
			if (m_fGC_TestCombatObserve > 0 && GetAnimationComponent())
			{
				CharacterInputContext input = GetInputContext();
				if (input && input.GetThrow())
					m_iGC_TestThrowInputFrames++;
				CharacterCommandHandlerComponent handler = GetAnimationComponent().GetCommandHandler();
				if (handler)
				{
					if (handler.IsThrowingAction())
						m_iGC_TestThrowFrames++;
					CharacterCommandMelee melee = handler.GetCommandModifier_Melee();
					if (melee && melee.IsMeleeAttackTag())
						m_iGC_TestMeleeFrames++;
				}
				m_fGC_TestCombatObserve -= dt;
			}
		}
		super.OnPrepareControls(owner, am, dt, player);
	}
}

modded class GC_LobbyFreezeComponent
{
	//------------------------------------------------------------------------------------------------
	string GC_TestFreezeServerCombat(int playerId)
	{
		if (!Replication.IsServer() || GetInstance() != this)
			return "UNAVAILABLE active authority";
		IEntity character = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!character)
			return "UNAVAILABLE controlled character";
		InventoryStorageManagerComponent inventory = InventoryStorageManagerComponent.Cast(character.FindComponent(InventoryStorageManagerComponent));
		if (!inventory)
			return "UNAVAILABLE character inventory";
		return string.Format("player=%1 freeze=%2 inventoryGrenades=%3 position=%4", playerId,
			IsFreezeActive(), inventory.GetGrenadesCount(), character.GetOrigin());
	}

	//------------------------------------------------------------------------------------------------
	//! Equip an existing inventory grenade as fixture setup; never throw it here.
	string GC_TestEquipGrenade()
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!player || !player.GetControlledEntity() || GetInstance() != this)
			return "UNAVAILABLE active local character";
		IEntity character = player.GetControlledEntity();
		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(character.FindComponent(SCR_CharacterControllerComponent));
		InventoryStorageManagerComponent inventory = InventoryStorageManagerComponent.Cast(character.FindComponent(InventoryStorageManagerComponent));
		if (!controller || !inventory)
			return "UNAVAILABLE controller/inventory";
		array<IEntity> items = {};
		inventory.GetItems(items);
		foreach (IEntity item : items)
		{
			if (!item || !item.FindComponent(GrenadeMoveComponent))
				continue;
			bool equipped = controller.TryEquipRightHandItem(item, EEquipItemType.EEquipTypeWeapon, true);
			return string.Format("fixture-equip=%1 inventoryGrenades=%2 prefab=%3", equipped,
				inventory.GetGrenadesCount(), item.GetPrefabData().GetPrefabName());
		}
		return string.Format("UNAVAILABLE inventory grenade; items=%1 grenadeCount=%2", items.Count(), inventory.GetGrenadesCount());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestFreezeCombat(string action)
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!player || !player.GetControlledEntity() || GetInstance() != this)
			return "UNAVAILABLE active local character";
		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(player.GetControlledEntity().FindComponent(SCR_CharacterControllerComponent));
		if (!controller)
			return "UNAVAILABLE character controller";
		if (action == "raise")
			controller.SetWeaponRaised(true);
		else if (action != "snapshot" && !controller.GC_TestCombatInput(action))
			return "REJECTED unsupported action or observation already running";
		return string.Format("freeze=%1 %2", IsFreezeActive(), controller.GC_TestCombatSnapshot());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestFreezeHud()
	{
		if (!m_wCountdown)
			return string.Format("visible=0 active=%1 remaining=%2", IsFreezeActive(), GetRemainingSeconds());
		float width, height;
		Widget card = m_wCountdown.FindAnyWidget("StagingCountdownCard");
		if (!card)
			return "FAIL countdown card missing";
		card.GetScreenSize(width, height);
		return string.Format("visible=%1 active=%2 remaining=%3 text=%4 width=%5 height=%6", m_wCountdown.IsVisible(), IsFreezeActive(), GetRemainingSeconds(), m_wCountdownTime.GetText(), width, height);
	}

	//------------------------------------------------------------------------------------------------
	//! Server fixture movement only; production boundary handling runs normally.
	string GC_TestFreezeMovePlayer(int playerId, vector position)
	{
		if (!Replication.IsServer() || GetInstance() != this || !GetGame().GetPlayerManager().IsPlayerConnected(playerId))
			return "UNAVAILABLE connected player on authority";
		return string.Format("fixture-move=%1 player=%2", SCR_Global.TeleportPlayer(playerId, position), playerId);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestFreezeServerPlayer(int playerId)
	{
		if (!Replication.IsServer())
			return "UNAVAILABLE authority";
		ChimeraCharacter character = ChimeraCharacter.Cast(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId));
		if (!character)
			return "UNAVAILABLE character";
		BaseWeaponManagerComponent weapons = character.GetCharacterController().GetWeaponManagerComponent();
		if (!weapons || !weapons.GetCurrentWeapon() || !weapons.GetCurrentWeapon().GetCurrentMagazine())
			return "UNAVAILABLE weapon/magazine";
		return string.Format("server-player=%1 ammo=%2 position=%3 freeze=%4", playerId, weapons.GetCurrentWeapon().GetCurrentMagazine().GetAmmoCount(), character.GetOrigin(), IsFreezeActive());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestFreezeWeapon(string action)
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!player)
			return "UNAVAILABLE player";
		ChimeraCharacter character = ChimeraCharacter.Cast(player.GetControlledEntity());
		if (!character)
			return "UNAVAILABLE character";
		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(character.GetCharacterController());
		if (!controller)
			return "UNAVAILABLE character controller";
		BaseWeaponManagerComponent weapons = controller.GetWeaponManagerComponent();
		if (!weapons)
			return "UNAVAILABLE weapon manager";
		BaseWeaponComponent weapon = weapons.GetCurrentWeapon();
		if (!weapon || !weapon.GetCurrentMagazine())
			return "UNAVAILABLE current weapon/magazine";
		if (action == "raise")
			controller.SetWeaponRaised(true);
		else if (action == "fire")
			controller.GC_TestFireInput(1);
		else if (action != "snapshot")
			return "REJECTED unsupported action";
		return string.Format("ammo=%1 raised=%2 canFire=%3 freeze=%4 position=%5", weapon.GetCurrentMagazine().GetAmmoCount(), controller.IsWeaponRaised(), controller.CanFire(), IsFreezeActive(), character.GetOrigin());
	}

	//------------------------------------------------------------------------------------------------
	//! Fixture movement through the stock teleport helper; boundary enforcement
	//! must be observed on a later frame, never called directly by this test.
	string GC_TestFreezeMove(vector position)
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller || !controller.GetControlledEntity())
			return "UNAVAILABLE local controlled character";
		bool moved = SCR_Global.TeleportPlayer(controller.GetPlayerId(), position);
		return string.Format("fixture-move=%1 position=%2", moved, controller.GetControlledEntity().GetOrigin());
	}

	//------------------------------------------------------------------------------------------------
	//! Optional fixture setup only. Does not change phase, roles, or countdown.
	string GC_TestStagingArea(vector center, float radius)
	{
		if (!Replication.IsServer() || GetInstance() != this || IsFreezeActive() || m_iConfiguredSeconds != 0)
			return "UNAVAILABLE inactive, unconfigured authority required";
		GC_LobbyStagingArea area = new GC_LobbyStagingArea();
		area.m_vCenter = center;
		area.m_fRadius = radius;
		if (!area.IsValid())
			return "REJECTED invalid fixture area";
		m_aAreas = {};
		m_aAreas.Insert(area);
		return "fixture-area-installed; no countdown or authorization changed";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestFreezeSnapshot()
	{
		return string.Format("configured=%1 remaining=%2 revision=%3 active=%4 areas=%5", GetConfiguredSeconds(), GetRemainingSeconds(), GetRevision(), IsFreezeActive(), HasStagingAreas());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestFreezeRequest(string action, int seconds)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return "UNAVAILABLE local controller";
		controller.RequestAdminFreeze(GetRevision(), action, seconds);
		return "DISPATCHED production controller request; inspect subsequent snapshot and audit";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestFreezeAdvance()
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!controller || !mode)
			return "UNAVAILABLE local controller or mode";
		controller.RequestLobbyPhaseAdvance(GC_LobbyPhase.GetNext(mode.GetState()));
		return "DISPATCHED production phase request";
	}
}
