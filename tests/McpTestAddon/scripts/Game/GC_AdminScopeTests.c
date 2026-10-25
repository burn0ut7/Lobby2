//! Opt-in observations and production requests for separate-client range tests.
//! No automatic execution, admin grants, or replacement gameplay handlers.
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestAdminScopeUnslot()
	{
		GC_LobbyPlayerController local = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!local || !GetSlotByPlayerId(local.GetPlayerId()))
			return "UNAVAILABLE occupied local slot";
		local.RequestLobbySlot(GetSlotByPlayerId(local.GetPlayerId()).GetID());
		return "DISPATCHED normal own-slot release request";
	}

	//------------------------------------------------------------------------------------------------
	//! Disposable range fixture: move only this client's character onto terrain.
	string GC_TestAdminScopeMove(vector position)
	{
		PlayerController local = GetGame().GetPlayerController();
		if (GetManager() != this || !local || !GC_GetAdminToolCharacter(local.GetPlayerId()))
			return "UNAVAILABLE living local character";
		position[1] = GetGame().GetWorld().GetSurfaceY(position[0], position[2]) + 0.2;
		bool moved = SCR_Global.TeleportPlayer(local.GetPlayerId(), position);
		return string.Format("fixtureMove=%1 %2", moved, GC_TestAdminScopeSnapshot(local.GetPlayerId()));
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestAdminScopePicker(int targetId)
	{
		GC_AdminToolsComponent.Open();
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu || !menu.GetRootWidget())
			return "UNAVAILABLE tools menu";
		Widget field = menu.GetRootWidget().FindAnyWidget("HealPlayers");
		if (!field)
			return "UNAVAILABLE picker";
		GC_AdminPlayerPicker picker = GC_AdminPlayerPicker.Cast(field.FindHandler(GC_AdminPlayerPicker));
		return picker.GC_TestAdminPickerTarget(targetId);
	}

	//------------------------------------------------------------------------------------------------
	//! Calls the rendered button's normal handler; does not emulate hardware input.
	string GC_TestAdminScopeUi(string operation, string value)
	{
		if (operation == "open")
			GC_AdminToolsComponent.Open();
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu || !menu.GetRootWidget())
			return "UNAVAILABLE menu";
		Widget root = menu.GetRootWidget();
		if (operation == "click")
		{
			Widget target = root.FindAnyWidget(value);
			Widget ancestor = target;
			while (ancestor)
			{
				if (!ancestor.IsVisible() || !ancestor.IsEnabled())
					return "UNAVAILABLE hidden or disabled control";
				ancestor = ancestor.GetParent();
			}
			if (!target)
				return "UNAVAILABLE control";
			SCR_ButtonBaseComponent button = SCR_ButtonBaseComponent.Cast(target.FindHandler(SCR_ButtonBaseComponent));
			if (!button)
				return "UNAVAILABLE button handler";
			button.OnClick(target, 0, 0, 0);
		}
		if (operation == "moderation")
		{
			Widget panel = root.FindAnyWidget("ModerationFields");
			if (!panel)
				return "UNAVAILABLE moderation panel";
			GC_AdminModerationComponent moderation = GC_AdminModerationComponent.Cast(panel.FindHandler(GC_AdminModerationComponent));
			return moderation.GC_TestScopeTarget(value.ToInt());
		}
		string result;
		array<string> labels = {"ToolTitle", "ToolSummary", "ToolStatus", "ModerationStatus"};
		foreach (string name : labels)
		{
			TextWidget text = TextWidget.Cast(root.FindAnyWidget(name));
			if (text)
				result += name + "=" + text.GetText() + "; ";
		}
		result += string.Format("ReservationPanel=%1 ReservationsSelectedBar=%2 MapPlanPanel=%3 MapPlansSelectedBar=%4",
			root.FindAnyWidget("ReservationPanel").IsVisible(), root.FindAnyWidget("ReservationsSelectedBar").IsVisible(),
			root.FindAnyWidget("MapPlanPanel").IsVisible(), root.FindAnyWidget("MapPlansSelectedBar").IsVisible());
		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! Call on both authority and the remote admin before mutations. Distance alone is not proof.
	string GC_TestAdminScopeSnapshot(int targetId)
	{
		PlayerManager players = GetGame().GetPlayerManager();
		array<int> connected = {};
		players.GetPlayers(connected);
		IEntity character = players.GetPlayerControlledEntity(targetId);
		GC_Slot slot = GetSlotByPlayerId(targetId);
		bool slotEntity;
		bool replicationItem;
		if (slot)
		{
			slotEntity = slot.GetEntity() != null;
			replicationItem = Replication.FindItem(slot.GetEntityRplId()) != null;
		}
		Faction faction = SCR_FactionManager.SGetPlayerFaction(targetId);
		string key;
		if (faction)
			key = faction.GetFactionKey();
		string result = string.Format("server=%1 connected=%2 listed=%3 identityReady=%4 character=%5 slotEntity=%6 replicationItem=%7 faction=%8",
			Replication.IsServer(), connected.Count(), connected.Contains(targetId), !GetPlayerVoiceIdentity(targetId).IsEmpty(),
			character != null, slotEntity, replicationItem, key);
		if (character)
		{
			result += " position=" + character.GetOrigin().ToString();
			SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(character.FindComponent(SCR_CharacterDamageManagerComponent));
			if (damage)
				result += " health=" + damage.GetHealthScaled().ToString();
		}
		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! Uses empty client selections for server-wide scopes, just like the production form.
	string GC_TestAdminScopeRequest(string action, string factionKey, int targetId)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!controller || GetManager() != this)
			return "UNAVAILABLE local controller";
		array<int> selected = {};
		array<string> identities = {};
		int requestId = controller.ReserveAdminPlayerToolRequest();
		if (action == "heal-everyone")
			controller.RequestAdminHeal(GC_EAdminHealScope.EVERYONE, selected, identities, requestId);
		else if (action == "heal-faction")
			controller.RequestAdminHeal(GC_EAdminHealScope.FACTION, selected, identities, requestId, GC_DEFAULT_HEAL_RADIUS, factionKey);
		else if (action == "heal-selected" || action == "heal-stale")
		{
			selected.Insert(targetId);
			string identity = GetPlayerVoiceIdentity(targetId);
			if (action == "heal-stale")
				identity = "stale-test-identity";
			identities.Insert(identity);
			controller.RequestAdminHeal(GC_EAdminHealScope.SELECTED, selected, identities, requestId);
		}
		else if (action == "teleport-here")
			controller.GC_RequestAdminTeleport(targetId, GetPlayerVoiceIdentity(targetId), controller.GetPlayerId(), GetPlayerVoiceIdentity(controller.GetPlayerId()), requestId);
		else if (action == "teleport-faction")
			controller.GC_RequestFactionTeleport(factionKey, targetId, GetPlayerVoiceIdentity(targetId), requestId);
		else if (action == "message-faction")
			controller.RequestAdminMessage(GC_EAdminMessageAudience.FACTION, "Admin scope regression", identities, requestId, factionKey);
		else if (action == "moderation")
			controller.RequestModerationSession(requestId);
		else
			return "UNAVAILABLE test action";
		return string.Format("DISPATCHED request=%1; inspect authoritative state, owner result and matching audit", requestId);
	}

	//------------------------------------------------------------------------------------------------
	//! Fixture only: damage one real spawned character, then invoke production healing separately.
	string GC_TestAdminScopeDamage(int targetId)
	{
		if (!Replication.IsServer() || GetManager() != this)
			return "UNAVAILABLE authority";
		ChimeraCharacter character = GC_GetAdminToolCharacter(targetId);
		if (!character)
			return "UNAVAILABLE living character";
		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(character.FindComponent(SCR_CharacterDamageManagerComponent));
		damage.SetHealthScaled(0.5);
		return GC_TestAdminScopeSnapshot(targetId);
	}

	//------------------------------------------------------------------------------------------------
	//! Assert a native faction member is included even with no Lobby2 slot.
	string GC_TestAdminScopeUnslotted(int targetId)
	{
		Faction faction = SCR_FactionManager.SGetPlayerFaction(targetId);
		if (!Replication.IsServer() || !GetGame().GetPlayerManager().IsPlayerConnected(targetId)
			|| !faction || GetSlotByPlayerId(targetId))
			return "UNAVAILABLE connected native faction member without a slot";
		array<int> audience;
		array<string> identities;
		if (!GC_GetFactionPlayers(faction.GetFactionKey(), audience, identities) || !audience.Contains(targetId))
			return "FAIL unslotted faction member omitted";
		return "PASS unslotted native faction member included";
	}

	//------------------------------------------------------------------------------------------------
	//! Destructive only to disposable test-session presentation history; does not change players or bans.
	string GC_TestAdminModerationRecovery(int targetId)
	{
		PlayerController local = GetGame().GetPlayerController();
		PlayerManager players = GetGame().GetPlayerManager();
		if (!Replication.IsServer() || !local || !SCR_Global.IsAdmin(local.GetPlayerId()) || !players.IsPlayerConnected(targetId))
			return "UNAVAILABLE authenticated authority and connected target";
		string identity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(targetId);
		if (identity.IsEmpty())
			return "UNAVAILABLE audited target identity";
		for (int index = m_aModerationHistory.Count() - 1; index >= 0; index--)
		{
			if (m_aModerationHistory[index].m_sIdentity == identity)
				m_aModerationHistory.RemoveOrdered(index);
		}
		SendModerationSession(local.GetPlayerId(), 9200);
		int matches;
		foreach (GC_AdminSessionPlayer record : m_aModerationHistory)
		{
			if (record.m_sIdentity == identity && record.m_iPlayerId == targetId)
				matches++;
		}
		if (matches != 1)
			return "FAIL live player missing or duplicated after roster refresh";
		return "PASS live player recovered from native roster without a join event";
	}
}

modded class GC_AdminPlayerPicker
{
	//------------------------------------------------------------------------------------------------
	string GC_TestAdminPickerTarget(int targetId)
	{
		array<int> players;
		array<string> names;
		ReadRoster(players, names);
		if (!m_Menu.IsVisible())
			m_Field.OnClick(m_Field.GetRootWidget(), 0, 0, 0);
		SCR_ButtonBaseComponent target;
		foreach (SCR_ButtonBaseComponent button, int playerId : m_mRows)
		{
			if (playerId == targetId)
				target = button;
		}
		if (!target)
			return "FAIL rendered player row missing";
		target.OnClick(target.GetRootWidget(), 0, 0, 0);
		array<int> selected;
		array<string> identities;
		bool valid = GetSelection(selected, identities);
		return string.Format("listed=%1 selectable=%2 character=%3", players.Contains(targetId),
			valid && selected.Contains(targetId), GetGame().GetPlayerManager().GetPlayerControlledEntity(targetId) != null);
	}
}

modded class GC_AdminModerationComponent
{
	//------------------------------------------------------------------------------------------------
	string GC_TestScopeTarget(int targetId)
	{
		foreach (SCR_ButtonBaseComponent button, int index : m_mRows)
		{
			if (m_aRows[index].ToInt() != targetId)
				continue;
			button.OnClick(button.GetRootWidget(), 0, 0, 0);
			return string.Format("rows=%1 selected=%2 identityReady=%3 kickEnabled=%4 banEnabled=%5", m_aRows.Count() / 7,
				m_iSelected == index, !m_Identity.GetText().IsEmpty(), m_wRoot.FindAnyWidget("ModerationKick").IsEnabled(),
				m_wRoot.FindAnyWidget("ModerationBan").IsEnabled());
		}
		return "FAIL connected target missing from moderation rows";
	}
}
