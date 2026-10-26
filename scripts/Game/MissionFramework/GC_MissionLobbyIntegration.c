//! The existing replicated lobby component is the transport and slot boundary.
//! No client-callable request RPC can execute mission rules.
modded class GC_LobbyManager
{
	protected ref map<string, bool> m_mGC_ClientMissionFlags = new map<string, bool>();
	protected ref map<string, int> m_mGC_MissionTickets = new map<string, int>();

	//------------------------------------------------------------------------------------------------
	bool GC_IsMissionEndPreview()
	{
		return m_bPreviewMissionEnd;
	}

	//------------------------------------------------------------------------------------------------
	GC_MissionRuntime GC_GetMissionRuntime()
	{
		if (GetManager() != this || !m_MissionFramework)
			return null;
		return m_MissionFramework.GetRuntime();
	}

	//------------------------------------------------------------------------------------------------
	bool GC_IsMissionFlagSet(string name)
	{
		if (GetManager() != this)
			return false;
		if (!Replication.IsServer())
			return m_mGC_ClientMissionFlags.Get(name);
		GC_MissionRuntime runtime = GC_GetMissionRuntime();
		return runtime && runtime.IsFlagSet(name);
	}

	//------------------------------------------------------------------------------------------------
	void GC_PublishMissionFlag(string name, bool value)
	{
		if (Replication.IsServer() && GetManager() == this)
			Rpc(GC_RpcDo_MissionFlag, name, value);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void GC_RpcDo_MissionFlag(string name, bool value)
	{
		if (!Replication.IsServer() && GetManager() == this)
			m_mGC_ClientMissionFlags.Set(name, value);
	}

	//------------------------------------------------------------------------------------------------
	override protected bool RplSave(ScriptBitWriter writer)
	{
		if (!super.RplSave(writer))
			return false;
		array<string> names = {};
		array<bool> values = {};
		GC_MissionRuntime runtime = GC_GetMissionRuntime();
		if (runtime)
			runtime.CopyFlagSnapshot(names, values);
		writer.WriteInt(names.Count());
		foreach (int index, string name : names)
		{
			writer.WriteString(name);
			writer.WriteBool(values[index]);
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override protected bool RplLoad(ScriptBitReader reader)
	{
		if (!super.RplLoad(reader))
			return false;
		int count;
		if (!reader.ReadInt(count) || count < 0 || count > 4096)
			return false;
		m_mGC_ClientMissionFlags.Clear();
		for (int i; i < count; i++)
		{
			string name;
			bool value;
			if (!reader.ReadString(name) || !reader.ReadBool(value))
				return false;
			m_mGC_ClientMissionFlags.Set(name, value);
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool GC_SendMissionMessage(string title, string body, float seconds, array<string> factions, bool adminsOnly, bool chat)
	{
		if (!Replication.IsServer() || GetManager() != this || GetLobbyPhase() == SCR_EGameModeState.POSTGAME)
			return false;
		Rpc(GC_RpcDo_MissionMessage, title, body, seconds, factions, adminsOnly, chat);
		if (RplSession.Mode() != RplMode.Dedicated)
			GC_ShowMissionMessage(title, body, seconds, factions, adminsOnly, chat);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void GC_RpcDo_MissionMessage(string title, string body, float seconds, array<string> factions, bool adminsOnly, bool chat)
	{
		if (!Replication.IsServer() && GetManager() == this)
			GC_ShowMissionMessage(title, body, seconds, factions, adminsOnly, chat);
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_ShowMissionMessage(string title, string body, float seconds, array<string> factions, bool adminsOnly, bool chat)
	{
		if (adminsOnly && !SCR_Global.IsAdmin())
			return;
		PlayerController player = GetGame().GetPlayerController();
		if (!player)
			return;
		if (factions && !factions.IsEmpty())
		{
			SCR_FactionManager manager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			Faction faction = manager.GetLocalPlayerFaction();
			if (!faction || !factions.Contains(faction.GetFactionKey()))
				return;
		}
		if (chat)
		{
			SCR_ChatComponent messages = SCR_ChatComponent.Cast(player.FindComponent(SCR_ChatComponent));
			if (messages)
				messages.ShowMessage(body);
			return;
		}
		SCR_HintManagerComponent hints = SCR_HintManagerComponent.GetInstance();
		if (hints)
			hints.Show(SCR_HintUIInfo.CreateInfo(body, title, seconds, EHint.UNDEFINED, EFieldManualEntryId.NONE, false), false, false);
	}

	//------------------------------------------------------------------------------------------------
	bool GC_DeleteEmptyMissionSlots(array<string> factions, GC_LobbyMissionContext context)
	{
		if (!Replication.IsServer() || GetManager() != this || !context || context.GetManager() != this
			|| !context.IsActive() || GetLobbyPhase() == SCR_EGameModeState.POSTGAME)
			return false;
		for (int i = GetSlots().Count() - 1; i >= 0; i--)
		{
			GC_Slot slot = GetSlots()[i];
			SCR_AIGroup group = slot.GetGroup();
			if (slot.HasClaim() || !group || !group.GetFaction()
				|| (factions && !factions.IsEmpty() && !factions.Contains(group.GetFaction().GetFactionKey())))
				continue;
			IEntity character = slot.GetEntity();
			if (character && !context.CanDelete(character))
				continue;
			RemoveRegisteredSlot(slot);
			Rpc(RpcDo_RemoveAdminSlot, slot.GetID());
			if (character)
				SCR_EntityHelper.DeleteEntityAndChildren(character);
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Mission budget belongs here; native slot spawning/possession remains canonical.
	bool GC_ChangeMissionTickets(string faction, GC_EMissionNumberOperation operation, int amount, bool respawn)
	{
		if (!Replication.IsServer() || GetManager() != this || GetLobbyPhase() == SCR_EGameModeState.POSTGAME
			|| amount < 0 || amount > 100000 || !GetGame().GetFactionManager().GetFactionByKey(faction))
			return false;
		int tickets = m_mGC_MissionTickets.Get(faction);
		// Reject overflow before changing the budget. Authored actions can repeat.
		if ((operation == GC_EMissionNumberOperation.ADD && tickets > 100000 - amount)
			|| (operation == GC_EMissionNumberOperation.MULTIPLY && amount > 0 && tickets > 100000 / amount))
		{
			PrintFormat("GC | Mission Framework | respawn tickets rejected: faction=%1 budget exceeds 100000", faction, level: LogLevel.ERROR);
			return false;
		}
		switch (operation)
		{
			case GC_EMissionNumberOperation.SET: tickets = amount; break;
			case GC_EMissionNumberOperation.ADD: tickets += amount; break;
			case GC_EMissionNumberOperation.SUBTRACT: tickets -= amount; break;
			case GC_EMissionNumberOperation.MULTIPLY: tickets *= amount; break;
			case GC_EMissionNumberOperation.DIVIDE:
				if (amount == 0)
					return false;
				tickets /= amount;
				break;
			default: return false;
		}
		tickets = Math.Max(0, tickets);
		m_mGC_MissionTickets.Set(faction, tickets);
		if (respawn && GetLobbyPhase() == SCR_EGameModeState.GAME)
		{
			foreach (GC_Slot slot : GetSlots())
			{
				if (tickets <= 0)
					break;
				int playerId = slot.GetPlayerId();
				if (!slot.IsKIA() || playerId <= 0 || !GetGame().GetPlayerManager().IsPlayerConnected(playerId))
					continue;
				Faction nativeFaction = SCR_FactionManager.Cast(GetGame().GetFactionManager()).GetPlayerFaction(playerId);
				if (!nativeFaction || nativeFaction.GetFactionKey() != faction)
					continue;
				if (RespawnAdminSlot(slot, false) == "applied")
					tickets--;
			}
			m_mGC_MissionTickets.Set(faction, tickets);
		}
		PrintFormat("GC | Mission Framework | respawn tickets: faction=%1 remaining=%2", faction, tickets);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool GC_RunMissionScript(string entityName, bool clients)
	{
		if (!Replication.IsServer() || GetManager() != this || GetLobbyPhase() == SCR_EGameModeState.POSTGAME)
			return false;
		IEntity entity = GetGame().GetWorld().FindEntityByName(entityName);
		if (!entity || !entity.FindComponent(GC_MissionScriptComponent))
			return false;
		if (!clients)
			return GC_MissionScriptComponent.Cast(entity.FindComponent(GC_MissionScriptComponent)).Run();
		Rpc(GC_RpcDo_RunMissionScript, entityName);
		if (RplSession.Mode() != RplMode.Dedicated)
			GC_RunLocalMissionScript(entityName);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void GC_RpcDo_RunMissionScript(string name)
	{
		if (!Replication.IsServer() && GetManager() == this)
			GC_RunLocalMissionScript(name);
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_RunLocalMissionScript(string name)
	{
		IEntity entity = GetGame().GetWorld().FindEntityByName(name);
		GC_MissionScriptComponent script;
		if (entity)
			script = GC_MissionScriptComponent.Cast(entity.FindComponent(GC_MissionScriptComponent));
		bool applied = script && script.Run();
		PrintFormat("GC | Mission Framework | client script: entity=%1 applied=%2", name, applied);
	}
}

[ComponentEditorProps(category: "Game/Mission Framework", description: "Custom mission script extension. Derive a component and override Run; attach it to a uniquely named entity.")]
class GC_MissionScriptComponentClass : ScriptComponentClass
{
}

class GC_MissionScriptComponent : ScriptComponent
{
	//------------------------------------------------------------------------------------------------
	bool Run()
	{
		Print("GC | Mission Framework | custom script has no Run implementation", LogLevel.ERROR);
		return false;
	}
}
