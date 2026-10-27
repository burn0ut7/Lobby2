#ifdef WORKBENCH
modded class GC_AdminLogsComponent
{
	//------------------------------------------------------------------------------------------------
	string GC_TestFilter(string query, int eventIndex)
	{
		if (!HasAccess() || eventIndex < 0 || eventIndex >= m_aTypes.Count())
			return "UNAVAILABLE access or filter";
		m_Search.SetText(query);
		m_Type.SetCurrentItem(eventIndex, false, false, false);
		OnType(m_Type, eventIndex);
		return "DISPATCHED filter through production change handler";
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	//! Native damage-manager fixture; production callbacks, relations and counters decide the result.
	string GC_TestLogDamage(int targetId, float amount)
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!Replication.IsServer() || !controller || !SCR_Global.IsAdmin(controller.GetPlayerId()) || amount <= 0 || amount > 1000)
			return "UNAVAILABLE authority/admin or damage bound";
		ChimeraCharacter target = ChimeraCharacter.Cast(GetGame().GetPlayerManager().GetPlayerControlledEntity(targetId));
		ChimeraCharacter actor = ChimeraCharacter.Cast(controller.GetControlledEntity());
		if (!target || !actor || target.FindComponent(GC_LobbySpectatorBodyComponent))
			return "UNAVAILABLE living target/actor";
		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(target.GetDamageManager());
		vector hit[3];
		hit[0] = target.GetOrigin();
		SCR_DamageContext context = new SCR_DamageContext(EDamageType.MELEE, amount, hit, target, damage.GetHeadHitZone(), Instigator.CreateInstigator(actor), null, -1, -1);
		BaseWeaponComponent weapon = actor.GetCharacterController().GetWeaponManagerComponent().GetCurrentWeapon();
		if (weapon)
			context.damageSource = weapon.GetOwner();
		context.damageEffect = new SCR_MeleeDamageEffect();
		damage.HandleDamage(context);
		return "DISPATCHED native melee damage; inspect individual events and health";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestLogFilter(string query, int eventIndex)
	{
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu || !menu.GetRootWidget())
			return "UNAVAILABLE menu";
		Widget root = menu.GetRootWidget().FindAnyWidget("LogFields");
		if (!root || !root.IsVisible())
			return "UNAVAILABLE logs panel";
		GC_AdminLogsComponent panel = GC_AdminLogsComponent.Cast(root.FindHandler(GC_AdminLogsComponent));
		return panel.GC_TestFilter(query, eventIndex);
	}
}
#endif
