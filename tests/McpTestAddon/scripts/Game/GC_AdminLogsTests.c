#ifdef WORKBENCH
modded class GC_AdminLogsComponent
{
	//------------------------------------------------------------------------------------------------
	string GC_TestFilter(string query, int eventIndex)
	{
		if (!HasAccess() || eventIndex < 0 || eventIndex >= m_aTypes.Count())
			return "UNAVAILABLE access or filter";
		m_Search.SetText(query);
		OnSearch(m_Search);
		m_Type.SetCurrentItem(eventIndex, false, false, false);
		OnType(m_Type, eventIndex);
		return "DISPATCHED filter through production change handler";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestRead()
	{
		string result = m_Status.GetText() + " | search=" + m_Search.GetText();
		foreach (string value : m_aDisplayed)
			result += "\n" + value;
		return result;
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	//! Arrange low blood above the fatal threshold; the stock bleeding effect must cause death.
	string GC_TestLogBleed(int targetId)
	{
		if (!Replication.IsServer() || !SCR_Global.IsAdmin(GetGame().GetPlayerController().GetPlayerId()))
			return "UNAVAILABLE authority/admin";
		ChimeraCharacter target = ChimeraCharacter.Cast(GetGame().GetPlayerManager().GetPlayerControlledEntity(targetId));
		return GC_TestApplyBleed(target);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestLogBleedSlot(int slotId)
	{
		if (!Replication.IsServer() || !SCR_Global.IsAdmin(GetGame().GetPlayerController().GetPlayerId()))
			return "UNAVAILABLE authority/admin";
		GC_Slot slot = GetSlot(slotId);
		if (!slot)
			return "UNAVAILABLE slot";
		return GC_TestApplyBleed(ChimeraCharacter.Cast(slot.GetEntity()));
	}

	//------------------------------------------------------------------------------------------------
	protected string GC_TestApplyBleed(ChimeraCharacter target)
	{
		if (!target || target.FindComponent(GC_LobbySpectatorBodyComponent) || target.GetCharacterController().IsDead())
			return "UNAVAILABLE living character";
		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(target.GetDamageManager());
		HitZone blood = damage.GetBloodHitZone();
		blood.SetHealthScaled(blood.GetDamageStateThreshold(ECharacterBloodState.DESTROYED) + 0.005);
		damage.AddParticularBleeding("Chest");
		return string.Format("DISPATCHED stock bleeding; blood=%1 state=%2", blood.GetHealthScaled(), blood.GetDamageState());
	}

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
		if (eventIndex == -1)
			return panel.GC_TestRead();
		return panel.GC_TestFilter(query, eventIndex);
	}
}
#endif
