#ifdef WORKBENCH
modded class GC_LobbyManager
{
	protected ChimeraCharacter m_GC_TestCombatAI;

	//! Native death actions; production callbacks alone award credit and increment counters.
	string GC_TestDelayedDeath(string action)
	{
		if (!Replication.IsServer() || !GC_MissionStatsComponent.GetInstance().IsCollecting())
			return "UNAVAILABLE active authority";
		ChimeraCharacter target = m_GC_TestCombatAI;
		if (action.Contains("player"))
			target = ChimeraCharacter.Cast(GetGame().GetPlayerController().GetControlledEntity());
		if (!target || target.FindComponent(GC_LobbySpectatorBodyComponent) || target.GetCharacterController().IsDead())
			return "UNAVAILABLE living target";
		if (action == "bleed-ai" || action == "bleed-player")
			return GC_TestApplyBleed(target);
		if (action == "respawn-player")
		{
			SCR_RespawnComponent respawn = SCR_RespawnComponent.Cast(GetGame().GetPlayerController().FindComponent(SCR_RespawnComponent));
			respawn.RequestPlayerSuicide();
			return "DISPATCHED owner respawn";
		}
		if (action != "suicide-ai" && action != "suicide-player")
			return "UNAVAILABLE action";
		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(target.GetDamageManager());
		vector hit[3];
		hit[0] = target.GetOrigin();
		SCR_DamageContext context = new SCR_DamageContext(EDamageType.MELEE, 1000, hit, target, damage.GetHeadHitZone(), Instigator.CreateInstigator(target), null, -1, -1);
		context.damageEffect = new SCR_MeleeDamageEffect();
		damage.HandleDamage(context);
		return "DISPATCHED native self damage";
	}

	//------------------------------------------------------------------------------------------------
	//! Native characters/damage only; production game-mode callbacks own all statistics.
	string GC_TestCombatAI(string action)
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!Replication.IsServer() || !controller || !SCR_Global.IsAdmin(controller.GetPlayerId()))
			return "UNAVAILABLE authority/admin";
		ChimeraCharacter player = ChimeraCharacter.Cast(controller.GetControlledEntity());
		if (!player)
			return "UNAVAILABLE living player";
		if (action == "spawn")
		{
			if (m_GC_TestCombatAI && !m_GC_TestCombatAI.GetCharacterController().IsDead())
				return "UNAVAILABLE existing living AI";
			EntitySpawnParams parameters = new EntitySpawnParams();
			parameters.TransformMode = ETransformMode.WORLD;
			parameters.Transform[3] = player.GetOrigin() + Vector(3, 0, 0);
			Resource prefab = Resource.Load("{BF643BE4ADBDFDD3}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_LAT.et");
			m_GC_TestCombatAI = ChimeraCharacter.Cast(GetGame().SpawnEntityPrefab(prefab, GetGame().GetWorld(), parameters));
			if (!m_GC_TestCombatAI)
				return "FAIL native spawn";
			return "SPAWNED " + m_GC_TestCombatAI.GetID().ToString();
		}
		if (action == "hurt-authored")
		{
			m_GC_TestCombatAI = ChimeraCharacter.Cast(GC_MissionStatsComponent.GetInstance().GC_TestFindAuthoredAI());
			action = "hurt-ai";
		}
		if (!m_GC_TestCombatAI || m_GC_TestCombatAI.GetCharacterController().IsDead())
			return "UNAVAILABLE living AI";
		if (action == "faction")
		{
			FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(m_GC_TestCombatAI.FindComponent(FactionAffiliationComponent));
			if (!affiliation)
				return "NO affiliation";
			return string.Format("faction=%1", affiliation.GetAffiliatedFaction());
		}
		if (action == "weapon")
		{
			CharacterControllerComponent current = m_GC_TestCombatAI.GetCharacterController();
			BaseWeaponComponent equipped = current.GetWeaponManagerComponent().GetCurrentWeapon();
			int ammo = -1;
			if (equipped && equipped.GetCurrentMagazine())
				ammo = equipped.GetCurrentMagazine().GetAmmoCount();
			return string.Format("weapon=%1 ammo=%2 canFire=%3 raised=%4", equipped, ammo, current.CanFire(), current.IsWeaponRaised());
		}
		if (action == "fire" || action == "prepare-fire")
		{
			CharacterControllerComponent aiController = m_GC_TestCombatAI.GetCharacterController();
			AIControlComponent ai = AIControlComponent.Cast(m_GC_TestCombatAI.FindComponent(AIControlComponent));
			if (ai)
				ai.DeactivateAI();
			array<BaseWeaponComponent> weapons = {};
			aiController.GetWeaponManagerComponent().GetWeapons(weapons);
			if (action == "prepare-fire")
			{
				foreach (BaseWeaponComponent candidate : weapons)
				{
					if (candidate.GetCurrentMagazine() && candidate.GetCurrentMagazine().GetAmmoCount() > 0)
					{
						aiController.SelectWeapon(candidate);
						break;
					}
				}
			}
			aiController.SetWeaponRaised(true);
			if (action == "prepare-fire")
				return "PREPARED native AI weapon";
			aiController.SetFireWeaponWanted(true);
			GetGame().GetCallqueue().CallLater(aiController.SetFireWeaponWanted, 1000, false, false);
			return "DISPATCHED native AI fire; inspect ammunition delta";
		}
		if (action == "delete-ai")
		{
			SCR_EntityHelper.DeleteEntityAndChildren(m_GC_TestCombatAI);
			return "DISPATCHED native deletion";
		}
		ChimeraCharacter target = player;
		ChimeraCharacter actor = m_GC_TestCombatAI;
		float amount = 100;
		if (action == "kill-ai" || action == "hurt-ai" || action == "hurt-ai-twice")
		{
			target = m_GC_TestCombatAI;
			actor = player;
			if (action != "kill-ai")
				amount = 1;
		}
		else if (action == "kill-ai-by-ai")
		{
			EntitySpawnParams targetParameters = new EntitySpawnParams();
			targetParameters.TransformMode = ETransformMode.WORLD;
			targetParameters.Transform[3] = player.GetOrigin() + Vector(6, 0, 0);
			target = ChimeraCharacter.Cast(GetGame().SpawnEntityPrefab(Resource.Load("{BF643BE4ADBDFDD3}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_LAT.et"), GetGame().GetWorld(), targetParameters));
			if (!target)
				return "FAIL native target spawn";
		}
		else if (action == "hurt-player")
			amount = 2;
		else if (action != "kill-player")
			return "UNAVAILABLE action";
		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(target.GetDamageManager());
		vector hit[3];
		hit[0] = target.GetOrigin();
		SCR_DamageContext context = new SCR_DamageContext(EDamageType.MELEE, amount, hit, target, damage.GetHeadHitZone(), Instigator.CreateInstigator(actor), null, -1, -1);
		BaseWeaponComponent weapon = actor.GetCharacterController().GetWeaponManagerComponent().GetCurrentWeapon();
		if (weapon)
			context.damageSource = weapon.GetOwner();
		context.damageEffect = new SCR_MeleeDamageEffect();
		damage.HandleDamage(context);
		if (action == "hurt-ai-twice")
			damage.HandleDamage(context);
		return "DISPATCHED native damage; inspect counters and events";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestStatsScroll(float position)
	{
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu || !menu.GetRootWidget())
			return "UNAVAILABLE menu";
		ScrollLayoutWidget scroll = ScrollLayoutWidget.Cast(menu.GetRootWidget().FindAnyWidget("StatsScroll"));
		if (!scroll)
			return "UNAVAILABLE stats scroll";
		scroll.SetSliderPos(0, Math.Clamp(position, 0, 1));
		return "DISPATCHED native scroll";
	}
}

modded class GC_StatsAIDamageTracker
{
	//------------------------------------------------------------------------------------------------
	IEntity GC_TestCharacter()
	{
		return m_Character;
	}
}

modded class GC_MissionStatsComponent
{
	//------------------------------------------------------------------------------------------------
	IEntity GC_TestFindAuthoredAI()
	{
		foreach (EntityID entityId, GC_StatsAIDamageTracker observer : m_mDamageObservers)
		{
			IEntity character = observer.GC_TestCharacter();
			if (character && character.GetPrefabData()
				&& character.GetPrefabData().GetPrefabName() == "{1CA3D30464EE4674}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Spotter.et")
				return character;
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestDamageObservers()
	{
		return string.Format("collecting=%1 observers=%2", IsCollecting(), m_mDamageObservers.Count());
	}
}
#endif
