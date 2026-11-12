//! Lobby2 integration for the base group type.
//!
//! This is intentionally a guarded modded extension rather than a replacement
//! group class. Every world keeps using the base SCR_AIGroup/prefab resources;
//! the Lobby2 behavior is active only while GC_LobbyGameMode is the active mode.
modded class SCR_AIGroup : ChimeraAIGroup
{
	[Attribute("", UIWidgets.EditBox, "Manual group call sign. Takes priority over faction call signs in GC Lobby.", category: "GC Lobby: Call sign")]
	protected string m_sGC_ManualCallsign;

	[Attribute("0", UIWidgets.EditBox, "Zero-based company index in this group's faction call sign configuration. -1 uses automatic assignment.", category: "GC Lobby: Call sign")]
	protected int m_iGC_CallsignCompany;

	[Attribute("0", UIWidgets.EditBox, "Zero-based platoon index. Used when Company is set.", category: "GC Lobby: Call sign")]
	protected int m_iGC_CallsignPlatoon;

	[Attribute("0", UIWidgets.EditBox, "Zero-based squad index. Used when Company is set.", category: "GC Lobby: Call sign")]
	protected int m_iGC_CallsignSquad;

	//------------------------------------------------------------------------------------------------
	string GC_GetManualCallsign()
	{
		if (!GC_LobbyGameMode.GetGameMode())
			return string.Empty;

		string name = m_sGC_ManualCallsign;
		name.TrimInPlace();
		return name;
	}

	//------------------------------------------------------------------------------------------------
	//! These are indices into the group's faction data, not faction IDs.
	bool GC_GetAuthoredCallsign(Faction faction, out int company, out int platoon, out int squad)
	{
		if (!GC_LobbyGameMode.GetGameMode() || m_iGC_CallsignCompany == -1)
			return false;

		SCR_Faction scrFaction = SCR_Faction.Cast(faction);
		SCR_FactionCallsignInfo info;
		if (scrFaction)
			info = scrFaction.GetCallsignInfo();
		if (info)
		{
			array<ref SCR_CallsignInfo> companies = {};
			array<ref SCR_CallsignInfo> platoons = {};
			array<ref SCR_CallsignInfo> squads = {};
			info.GetCompanyArray(companies);
			info.GetPlatoonArray(platoons);
			info.GetSquadArray(squads);
			if (companies.IsIndexValid(m_iGC_CallsignCompany) && platoons.IsIndexValid(m_iGC_CallsignPlatoon) && squads.IsIndexValid(m_iGC_CallsignSquad))
			{
				company = m_iGC_CallsignCompany;
				platoon = m_iGC_CallsignPlatoon;
				squad = m_iGC_CallsignSquad;
				return true;
			}
		}

		PrintFormat("GC | Call sign | invalid faction indices for %1: %2/%3/%4; using automatic assignment", GetName(), m_iGC_CallsignCompany, m_iGC_CallsignPlatoon, m_iGC_CallsignSquad, level: LogLevel.WARNING);
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override string GetCustomName()
	{
		string name = GC_GetManualCallsign();
		if (!name.IsEmpty())
			return name;
		return super.GetCustomName();
	}

	//------------------------------------------------------------------------------------------------
	override string GetCustomNameWithOriginal()
	{
		string name = GC_GetManualCallsign();
		if (!name.IsEmpty())
			return name;
		return super.GetCustomNameWithOriginal();
	}

#ifdef WORKBENCH
	protected static ref GC_PlayerFormationCatalog s_WorkbenchFormationCatalog;
	protected static bool s_bWorkbenchFormationFallbackLogged;
#endif

	//! Exposes the base group's authored member-placement policy to Lobby2's
	//! guarded spawn path without copying it into separate lobby state.
	bool GC_ShouldSnapMembersToTerrain()
	{
		return m_bSnapToTerrain;
	}

	//------------------------------------------------------------------------------------------------
	//! Native group IDs are snapshot-only. Publish assignment to existing proxies;
	//! proxies streamed later receive the same field through native RplLoad.
	void GC_PublishLobbyGroupID()
	{
		if (Replication.IsServer() && GC_LobbyManager.GetManager() && GetGroupID() >= 0)
			Rpc(GC_RpcDoLobbyGroupID, GetGroupID());
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void GC_RpcDoLobbyGroupID(int groupId)
	{
		if (Replication.IsServer() || !GC_LobbyManager.GetManager() || groupId < 0)
			return;
		SetGroupID(groupId);
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		if (!m_bPlayable)
			return;

		if (GC_LobbyGameMode.GetGameMode())
		{
			ChimeraAIWorld aiWorld = ChimeraAIWorld.Cast(GetGame().GetAIWorld());
			if (aiWorld)
				aiWorld.PurgeSpawnRequestsForGroup(this);

			if (Replication.IsServer())
			{
				array<AIAgent> agents = {};
				GetAgents(agents);
				foreach (AIAgent agent : agents)
					GC_DeactivatePlayableAgent(agent);
			}
		}

		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
			manager.RegisterGroup(this);
	}

	override void OnAgentAdded(AIAgent child)
	{
		super.OnAgentAdded(child);

		if (!Replication.IsServer() || !m_bPlayable || !GC_LobbyGameMode.GetGameMode())
			return;

		GC_DeactivatePlayableAgent(child);
	}

	protected void GC_DeactivatePlayableAgent(AIAgent agent)
	{
		if (!agent)
			return;

		IEntity entity = agent.GetControlledEntity();
		if (!entity)
			return;

		AIControlComponent aiControl = AIControlComponent.Cast(entity.FindComponent(AIControlComponent));
		if (aiControl)
			aiControl.DeactivateAI();
	}

	void ~SCR_AIGroup()
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
			manager.UnregisterGroup(this);
	}

	override void RequestSpawn(int slotsWanted = -1, float observerRange = 0)
	{
		if (GC_LobbyGameMode.GetGameMode() && m_bPlayable && !GetWorld().IsEditMode())
			return;

		super.RequestSpawn(slotsWanted, observerRange);
	}

	override bool ExpandOneMember()
	{
		// SCR_AIWorld drains queued startup requests through this method. Guard the
		// dequeue path as well as RequestSpawn so initialization order cannot let a
		// playable Lobby2 group materialize before its claimed slot is staged.
		if (GC_LobbyGameMode.GetGameMode() && m_bPlayable && !GetWorld().IsEditMode())
			return false;

		return super.ExpandOneMember();
	}

	override void OnEmpty()
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!manager || !manager.GetRegisteredGroups().Contains(this))
		{
			super.OnEmpty();
			return;
		}

		// Preserve the native empty event and dormant handling, but prevent its
		// delayed entity deletion while Lobby2 owns this playable respawn group.
		bool deleteWhenEmpty = m_bDeleteWhenEmpty;
		m_bDeleteWhenEmpty = false;
		super.OnEmpty();
		m_bDeleteWhenEmpty = deleteWhenEmpty;
		if (deleteWhenEmpty)
			PrintFormat("GC | Lifecycle | preserved empty playable AI group: group=%1", this);
	}

	override void SpawnUnits()
	{
		// Editor previews opt in through the world's mode too. The runtime lookup
		// deliberately excludes edit mode, so inspect the authored mode directly.
		if (GetWorld().IsEditMode() && m_bPlayable && GC_LobbyGameMode.Cast(GetGame().GetGameMode()))
		{
#ifdef WORKBENCH
			SpawnWorkbenchPreview();
#else
			super.SpawnUnits();
#endif
			return;
		}

		if (GC_LobbyGameMode.GetGameMode() && m_bPlayable && !GetWorld().IsEditMode())
			return;

		super.SpawnUnits();
	}

#ifdef WORKBENCH
	protected GC_PlayerFormationCatalog GetWorkbenchFormationCatalog()
	{
		if (!s_WorkbenchFormationCatalog)
		{
			s_WorkbenchFormationCatalog = new GC_PlayerFormationCatalog();
			s_WorkbenchFormationCatalog.Load(GC_PlayerFormationCatalog.GetDefaultSource());
		}

		return s_WorkbenchFormationCatalog;
	}

	protected string ResolveWorkbenchFormationName()
	{
		string formationName = GC_PlayerFormationCatalog.GetDefaultFormationName();
		string authoredFormationName;
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		WorldEditorAPI worldEditorAPI;
		IEntitySource entitySource;
		if (worldEditor)
			worldEditorAPI = worldEditor.GetApi();
		if (worldEditorAPI)
			entitySource = worldEditorAPI.EntityToSource(this);
		if (entitySource)
		{
			for (int componentIndex = 0; componentIndex < entitySource.GetComponentCount(); componentIndex++)
			{
				IEntityComponentSource componentSource = entitySource.GetComponent(componentIndex);
				if (!componentSource || componentSource.GetClassName() != "AIFormationComponent")
					continue;

				if (componentSource.Get("DefaultFormation", authoredFormationName) && authoredFormationName != string.Empty)
					return authoredFormationName;
			}
		}

		AIFormationComponent formationComponent = AIFormationComponent.Cast(FindComponent(AIFormationComponent));
		BaseContainer formationSource;
		if (formationComponent)
			formationSource = formationComponent.GetComponentSource(this);
		if (formationSource && formationSource.Get("DefaultFormation", authoredFormationName) && authoredFormationName != string.Empty)
			return authoredFormationName;

		AIFormationDefinition formationDefinition;
		if (formationComponent)
			formationDefinition = formationComponent.GetFormation();
		if (formationDefinition && formationDefinition.GetName() != string.Empty)
			return formationDefinition.GetName();

		return formationName;
	}

	protected vector ResolveWorkbenchFormationOffset(string formationName, int index)
	{
		GC_PlayerFormationCatalog catalog = GetWorkbenchFormationCatalog();
		vector offset;
		if (catalog && catalog.TryGetOffset(formationName, index, offset))
			return offset;
		if (formationName != GC_PlayerFormationCatalog.GetDefaultFormationName() && catalog && catalog.TryGetOffset(GC_PlayerFormationCatalog.GetDefaultFormationName(), index, offset))
			return offset;

		if (!s_bWorkbenchFormationFallbackLogged)
		{
			s_bWorkbenchFormationFallbackLogged = true;
			PrintFormat("GC | Formation | Workbench linear fallback: formation=%1 slot=%2", formationName, index, level: LogLevel.WARNING);
		}

		return Vector(index * 2, 0, 0);
	}

	protected void SpawnWorkbenchPreview()
	{
		DestroyEntities(m_aSceneGroupUnitInstances);
		if (!m_aUnitPrefabSlots || m_aUnitPrefabSlots.IsEmpty())
			return;

		string formationName = ResolveWorkbenchFormationName();
		m_aSceneGroupUnitInstances = new array<IEntity>();
		for (int index = 0; index < m_aUnitPrefabSlots.Count(); index++)
		{
			EntitySpawnParams spawnParams = new EntitySpawnParams();
			spawnParams.TransformMode = ETransformMode.WORLD;
			GetWorldTransform(spawnParams.Transform);
			vector memberPosition = CoordToParent(ResolveWorkbenchFormationOffset(formationName, index));
			if (GC_ShouldSnapMembersToTerrain())
				memberPosition[1] = GetWorld().GetSurfaceY(memberPosition[0], memberPosition[2]);
			spawnParams.Transform[3] = memberPosition;

			IEntity member = GetGame().SpawnEntityPrefabEx(m_aUnitPrefabSlots[index], true, GetWorld(), spawnParams);
			if (!member)
				continue;

			m_aSceneGroupUnitInstances.Insert(member);
			Faction faction = GetFaction();
			if (faction)
			{
				FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(member.FindComponent(FactionAffiliationComponent));
				if (affiliation)
					affiliation.SetAffiliatedFactionByKey(faction.GetFactionKey());
			}

			AIControlComponent aiControl = AIControlComponent.Cast(member.FindComponent(AIControlComponent));
			if (aiControl)
				aiControl.DeactivateAI();

		}
	}

	override bool _WB_OnKeyChanged(BaseContainer src, string key, BaseContainerList ownerContainers, IEntity parent)
	{
		if (!GC_LobbyGameMode.Cast(GetGame().GetGameMode()))
			return super._WB_OnKeyChanged(src, key, ownerContainers, parent);

		if (key == "coords" || key == "angles" || key == "m_aUnitPrefabSlots" || key == "m_bSnapToTerrain" || key == "m_bPlayable" || key == "DefaultFormation")
		{
			if (m_bPlayable)
				SpawnWorkbenchPreview();
			else
				DestroyEntities(m_aSceneGroupUnitInstances);

			return false;
		}

		return super._WB_OnKeyChanged(src, key, ownerContainers, parent);
	}
#endif
}
