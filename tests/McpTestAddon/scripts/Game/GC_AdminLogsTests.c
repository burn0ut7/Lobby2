#ifdef WORKBENCH

modded class GC_AdminSearchFilter
{
	//------------------------------------------------------------------------------------------------
	string GC_TestInteraction(string action)
	{
		if (action == "click")
			m_InputEvents.OnClick(m_Input, 0, 0, 0);
		else if (action == "back")
			OnMenuBack();
		Widget focus = m_Workspace.GetFocusedWidget();
		string name;
		if (focus)
			name = focus.GetName();
		return string.Format("open=%1 writing=%2 focused=%3 value=%4", m_bOpened, m_Input.IsInWriteMode(), name, GetFilterValue());
	}
}

modded class GC_AdminLogsComponent
{
	//------------------------------------------------------------------------------------------------
	string GC_TestFilter(string query, int eventIndex)
	{
		if (!HasAccess() || eventIndex < 0 || eventIndex >= m_aTypes.Count())
			return "UNAVAILABLE access or filter";
		EditBoxWidget input = EditBoxWidget.Cast(m_Players.GetRootWidget().FindAnyWidget("FilterInput"));
		input.SetText(query);
		SCR_EventHandlerComponent.Cast(input.FindHandler(SCR_EventHandlerComponent)).GetOnChange().Invoke(input);
		m_Type.OpenList();
		array<Widget> choices = {};
		m_Type.GetElementWidgets(choices);
		if (eventIndex >= choices.Count())
			return "UNAVAILABLE native choice";
		SCR_ButtonTextComponent choice = SCR_ButtonTextComponent.Cast(choices[eventIndex].FindHandler(SCR_ButtonTextComponent));
		choice.OnClick(choices[eventIndex], 0, 0, 0);
		return "DISPATCHED filter through production change handler";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestInteraction(string field, string action)
	{
		GC_AdminSearchFilter filter = m_Players;
		if (field == "events")
			filter = m_Type;
		if (action == "blur")
			GetGame().GetWorkspace().SetFocusedWidget(m_wRoot.FindAnyWidget("LogLive"));
		if (action == "roster")
			RequestPlayers();
		return filter.GC_TestInteraction(action) + " live=" + m_bLive.ToString()
			+ " checked=" + m_wRoot.FindAnyWidget("LogLive").FindAnyWidget("Tick").IsVisible().ToString();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestEditableFilter(string field, string query, int choice)
	{
		GC_AdminSearchFilter filter = m_Players;
		if (field == "events")
			filter = m_Type;
		EditBoxWidget input = EditBoxWidget.Cast(filter.GetRootWidget().FindAnyWidget("FilterInput"));
		input.SetText(query);
		SCR_EventHandlerComponent.Cast(input.FindHandler(SCR_EventHandlerComponent)).GetOnChange().Invoke(input);
		array<Widget> options = {};
		filter.GetElementWidgets(options);
		string result = "options=";
		foreach (int index, Widget option : options)
		{
			if (option.IsVisible())
				result += index.ToString() + ":" + filter.GetItemName(index) + ";";
		}
		if (choice >= 0 && choice < options.Count() && options[choice].IsVisible())
			SCR_ButtonTextComponent.Cast(options[choice].FindHandler(SCR_ButtonTextComponent)).OnClick(options[choice], 0, 0, 0);
		return result + " value=" + filter.GetFilterValue();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestRead()
	{
		string result = m_Page.GetText() + " | " + m_Status.GetText() + " | search=" + m_Players.GetFilterValue()
			+ " | previous=" + m_wRoot.FindAnyWidget("LogNewer").IsEnabled()
			+ " | next=" + m_wRoot.FindAnyWidget("LogOlder").IsEnabled();
		foreach (string value : m_aDisplayed)
			result += "\n" + value;
		return result;
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestLogInteraction(string field, string action)
	{
		Widget root = GetGame().GetMenuManager().GetTopMenu().GetRootWidget().FindAnyWidget("LogFields");
		return GC_AdminLogsComponent.Cast(root.FindHandler(GC_AdminLogsComponent)).GC_TestInteraction(field, action);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestLogEditable(string field, string query, int choice)
	{
		Widget root = GetGame().GetMenuManager().GetTopMenu().GetRootWidget().FindAnyWidget("LogFields");
		return GC_AdminLogsComponent.Cast(root.FindHandler(GC_AdminLogsComponent)).GC_TestEditableFilter(field, query, choice);
	}

	//------------------------------------------------------------------------------------------------
	//! Explicit pagination fixture records, through the authority's production recorder.
	string GC_TestLogPageSeed()
	{
		int playerId = GetGame().GetPlayerController().GetPlayerId();
		GC_SessionLogComponent logs = GC_SessionLogComponent.GetInstance();
		if (!Replication.IsServer() || !SCR_Global.IsAdmin(playerId) || !logs)
			return "UNAVAILABLE authority/admin";
		for (int i; i < 40; i++)
		{
			GC_SessionEvent entry = logs.CreateEvent("session", playerId, 0);
			entry.m_sDetail = string.Format("Pagination fixture %1", i + 1);
			logs.Record(entry);
		}
		return "RECORDED 40 explicit pagination fixtures";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestLogTimeFormat()
	{
		array<float> values = {0, 0.007, 62.125, 59.9996, 3600, 3661.125, 360000.125};
		array<string> expected = {"00:00:000", "00:00:007", "01:02:125", "01:00:000", "01:00:00:000", "01:01:01:125", "100:00:00:125"};
		foreach (int index, float value : values)
		{
			string actual = GC_SessionLogComponent.FormatEventTime(value);
			if (actual != expected[index])
				return string.Format("FAIL: %1 -> %2; expected %3", value, actual, expected[index]);
		}
		return "PASS: zero padding, millisecond precision, minute rollover and optional hours";
	}

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
