//! Explicit fixtures arrange real characters/items/physics; production callbacks collect results.
modded class GC_StatsCharacterTracker
{
	//------------------------------------------------------------------------------------------------
	string GC_TestDamageDuplicates()
	{
		BaseDamageContext context = new BaseDamageContext();
		context.damageType = EDamageType.KINETIC;
		context.instigator = Instigator.CreateInstigator(null);
		context.instigator.SetInstigatorByPlayerID(m_iPlayerId);
		m_iDamageInstigator = m_iPlayerId;
		m_sDamageWeapon = "known-fixture-weapon";
		m_fDamageImpactTime = GetGame().GetWorld().GetWorldTime();
		OnDamage(context);
		if (m_sDamageWeapon != "known-fixture-weapon")
			return "FAIL: paired notification erased weapon";
		m_fDamageImpactTime -= 1;
		OnDamage(context);
		if (!m_sDamageWeapon.IsEmpty())
			return "FAIL: later unknown hit retained stale weapon";
		m_sDamageWeapon = "known-fixture-weapon";
		context.hitPosition = "1 2 3";
		OnDamage(context);
		if (!m_sDamageWeapon.IsEmpty())
			return "FAIL: distinct same-frame impact retained stale weapon";
		IEntity victim = GetGame().GetPlayerController().GetControlledEntity();
		m_DamageCharacterId = victim.GetID();
		m_sDamageWeapon = "known-fixture-weapon";
		Detach();
		if (GetDamageWeapon(m_iPlayerId, victim) != "known-fixture-weapon")
			return "FAIL: possession handoff erased victim impact";
		if (!GetDamageWeapon(m_iPlayerId, GetGame().GetGameMode()).IsEmpty())
			return "FAIL: different entity inherited impact";
		return "PASS: duplicate, later unknown, distinct impact, detach and victim identity";
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnDamage(notnull BaseDamageContext context)
	{
		super.OnDamage(context);
		PrintFormat("GC | Stats test | damage player=%1 type=%2 source=%3 instigator=%4 weapon=%5 hit=%6 direction=%7 velocity=%8", m_iPlayerId, context.damageType, context.damageSource, m_iDamageInstigator, m_sDamageWeapon, context.hitPosition, context.hitDirection, context.impactVelocity);
	}

	//------------------------------------------------------------------------------------------------
	override void Detach()
	{
		if (m_Character && !m_sDamageWeapon.IsEmpty())
			PrintFormat("GC | Stats test | detach player=%1 character=%2 weapon=%3", m_iPlayerId, m_Character, m_sDamageWeapon);
		super.Detach();
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	//! Arrange a wounded target; only subsequent native projectile damage may produce the tested death.
	string GC_TestStatsWoundedTarget(int playerId)
	{
		if (!Replication.IsServer() || GetManager() != this)
			return "unavailable: authority";
		ChimeraCharacter target = ChimeraCharacter.Cast(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId));
		if (!target || target.FindComponent(GC_LobbySpectatorBodyComponent))
			return "unavailable: living target";
		target.GetDamageManager().GetDefaultHitZone().SetHealthScaled(0.05);
		return GC_TestAdminHealthSnapshot(playerId);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestStatsDamageDuplicates()
	{
		GC_MissionStatsComponent stats = GC_MissionStatsComponent.GetInstance();
		if (!stats || !stats.IsCollecting())
			return "unavailable: collecting authority";
		GC_StatsCharacterTracker tracker = new GC_StatsCharacterTracker();
		tracker.Start(stats, GetGame().GetPlayerController().GetPlayerId(), new GC_StatsPlayer());
		return tracker.GC_TestDamageDuplicates();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestStatsMedical(string action, int targetId)
	{
		if (!Replication.IsServer() || GetManager() != this)
			return "unavailable: active authority";
		ChimeraCharacter medic = ChimeraCharacter.Cast(GetGame().GetPlayerController().GetControlledEntity());
		ChimeraCharacter target = ChimeraCharacter.Cast(GetGame().GetPlayerManager().GetPlayerControlledEntity(targetId));
		if (!medic || !target || medic == target)
			return "unavailable: distinct controlled characters";
		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(target.GetDamageManager());
		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(medic.GetCharacterController());
		if (!damage || !controller)
			return "unavailable: character components";
		if (action == "prepare")
		{
			SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(medic.FindComponent(SCR_InventoryStorageManagerComponent));
			SCR_CharacterInventoryStorageComponent storage = SCR_CharacterInventoryStorageComponent.Cast(medic.FindComponent(SCR_CharacterInventoryStorageComponent));
			if (!inventory || !storage || !inventory.GetBandageItem())
				return "unavailable: inventory bandage";
			damage.AddParticularBleeding("Chest");
			controller.SetWeaponRaised(false);
			storage.UseItem(inventory.GetBandageItem(), ESlotFunction.TYPE_GADGET);
			return "dispatched native bleeding and bandage equip; inspect before use";
		}
		if (action != "use")
			return "unavailable: expected prepare or use";
		ActionsManagerComponent actions = ActionsManagerComponent.Cast(target.FindComponent(ActionsManagerComponent));
		if (!actions)
			return "unavailable: target actions";
		array<BaseUserAction> available = {};
		actions.GetActionsList(available);
		foreach (BaseUserAction baseAction : available)
		{
			SCR_BandageUserAction bandage = SCR_BandageUserAction.Cast(baseAction);
			if (bandage && bandage.CanBeShownScript(medic) && bandage.CanBePerformedScript(medic))
			{
				bandage.PerformAction(target, medic);
				return "dispatched stock bandage action; inspect completion and statistics";
			}
		}
		return "unavailable: eligible bandage action";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestStatsAim(float pitchRadians)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(GetGame().GetPlayerController().GetControlledEntity());
		if (!character || Math.AbsFloat(pitchRadians) > 0.5)
			return "unavailable: character or pitch";
		CharacterControllerComponent controller = character.GetCharacterController();
		controller.SetHeadingAngle(0, true);
		CharacterAimingComponent aiming = controller.GetAimingComponent();
		if (!aiming)
			return "unavailable: aiming component";
		aiming.SetAimingRotationWanted(Vector(0, pitchRadians, 0));
		aiming.SetAimingRotation(Vector(0, pitchRadians, 0));
		return string.Format("position=%1 aimWorld=%2", character.GetOrigin(), aiming.GetAimingDirectionWorld());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestStatsVehicleDrift(float speed)
	{
		if (!Replication.IsServer() || speed < 0 || speed > 10)
			return "unavailable: authority or bounded speed";
		IEntity character = GetGame().GetPlayerController().GetControlledEntity();
		IEntity vehicle = CompartmentAccessComponent.GetVehicleIn(character);
		if (!vehicle || !vehicle.GetPhysics())
			return "unavailable: occupied physical vehicle";
		vehicle.GetPhysics().SetVelocity(Vector(0, 0, speed));
		return string.Format("fixture velocity=%1 position=%2", speed, vehicle.GetOrigin());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestStatsLayout(string action)
	{
		GC_Lobby_DebriefMenu menu = GC_Lobby_DebriefMenu.Cast(GetGame().GetMenuManager().GetTopMenu());
		if (!menu)
			return "unavailable: debrief";
		return menu.GC_TestStatsLayout(action);
	}
}

modded class GC_Lobby_DebriefMenu
{
	//------------------------------------------------------------------------------------------------
	string GC_TestStatsLayout(string action)
	{
		ScrollLayoutWidget scroll = ScrollLayoutWidget.Cast(GetRootWidget().FindAnyWidget("AmmoScroll"));
		if (action == "bottom")
		{
			scroll.SetSliderPos(0, 1);
			float x;
			float y;
			scroll.GetSliderPos(x, y);
			return string.Format("scroll=%1", y);
		}
		if (action != "long")
			return "unavailable: expected long or bottom";
		GC_StatsAssignment assignment = new GC_StatsAssignment();
		assignment.m_iShots = 123456;
		assignment.m_fPlaySeconds = 72000;
		for (int i = 0; i < 40; i++)
		{
			GC_StatsAmmo ammo = new GC_StatsAmmo();
			ammo.m_sProjectile = string.Format("fixture/Ammo_%1_Long_ammunition_type_with_several_words_for_wrapping_and_a_deliberately_extended_label.et", i);
			ammo.m_iRounds = 123456;
			assignment.m_aAmmo.Insert(ammo);
		}
		GC_StatsDebrief report = new GC_StatsDebrief();
		report.AddAssignment(assignment, "fixture", true);
		report.Seal(false);
		OnSummary(m_sFaction, report.GetSummary(), report.GetAmmunition(), false);
		TextWidget.Cast(GetRootWidget().FindAnyWidget("DebriefPlayer")).SetText("Layout test · synthetic values");
		return string.Format("synthetic ammo characters=%1", report.GetAmmunition().Length());
	}
}
