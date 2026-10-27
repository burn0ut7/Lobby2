#ifdef WORKBENCH
modded class GC_LobbyManager
{
	protected ChimeraCharacter m_GC_TestCombatAI;

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
