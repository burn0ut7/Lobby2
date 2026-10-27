//! Placed configuration only. The lobby-owned runtime remains the execution owner.
class GC_MissionAuthoringEntityClass : GenericEntityClass
{
}

class GC_MissionAuthoringEntity : GenericEntity
{
	[Attribute("0", UIWidgets.EditBox, "Execution order, lowest first. Give sibling actions different orders.", category: "Mission Framework")]
	int m_iOrder;

	//! Weak entity references: the registry must not keep a world alive.
	protected static ref array<GC_MissionAuthoringEntity> s_aNodes = {};

	//------------------------------------------------------------------------------------------------
	static bool Collect(BaseWorld world, array<ref GC_MissionEvent> events, array<ref GC_MissionFlag> flags, out string error)
	{
		array<GC_MissionEventEntity> roots = {};
		array<GC_MissionFlagEntity> flagRoots = {};
		foreach (GC_MissionAuthoringEntity node : s_aNodes)
		{
			if (!node || node.GetWorld() != world)
				continue;
			GC_MissionFlagEntity flagEntity = GC_MissionFlagEntity.Cast(node);
			if (flagEntity)
			{
				if (GC_MissionAuthoringEntity.Cast(node.GetParent()))
				{
					error = "Mission Flag cannot be nested under another mission node: " + node.GetName();
					return false;
				}
				int flagIndex;
				while (flagIndex < flagRoots.Count() && flagRoots[flagIndex].m_iOrder < node.m_iOrder)
					flagIndex++;
				if (flagIndex < flagRoots.Count() && flagRoots[flagIndex].m_iOrder == node.m_iOrder)
				{
					error = "Mission Flags need distinct Order values: " + node.GetName();
					return false;
				}
				flagRoots.InsertAt(flagEntity, flagIndex);
				continue;
			}
			GC_MissionEventEntity eventEntity = GC_MissionEventEntity.Cast(node);
			if (eventEntity)
			{
				if (GC_MissionAuthoringEntity.Cast(node.GetParent()))
				{
					error = "Mission Event cannot be nested: " + node.GetName();
					return false;
				}
				int insertAt;
				while (insertAt < roots.Count() && roots[insertAt].m_iOrder < node.m_iOrder)
					insertAt++;
				if (insertAt < roots.Count() && roots[insertAt].m_iOrder == node.m_iOrder)
				{
					error = "Mission Events need distinct Order values: " + node.GetName();
					return false;
				}
				roots.InsertAt(eventEntity, insertAt);
				continue;
			}
			IEntity parent = node.GetParent();
			int depth;
			while (parent && GC_MissionAuthoringEntity.Cast(parent) && !GC_MissionEventEntity.Cast(parent)
				&& !GC_MissionFlagEntity.Cast(parent) && depth < 32)
			{
				parent = parent.GetParent();
				depth++;
			}
			if ((!GC_MissionEventEntity.Cast(parent) && !GC_MissionFlagEntity.Cast(parent)) || depth >= 32)
			{
				error = "Place this node under a Mission Event or an automatic Mission Flag (maximum depth 32): " + node.GetName();
				return false;
			}
		}
		foreach (GC_MissionFlagEntity root : flagRoots)
		{
			GC_MissionFlag definition = root.BuildFlag(error);
			if (!definition)
				return false;
			flags.Insert(definition);
		}
		foreach (GC_MissionEventEntity root : roots)
		{
			GC_MissionEvent definition = root.BuildEvent(error);
			if (!definition)
				return false;
			events.Insert(definition);
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool ReadChildren(array<ref GC_MissionCondition> conditions, array<ref GC_MissionAction> actions, out string error)
	{
		array<GC_MissionActionEntity> orderedActions = {};
		IEntity child = GetChildren();
		while (child)
		{
			GC_MissionConditionEntity condition = GC_MissionConditionEntity.Cast(child);
			GC_MissionActionEntity action = GC_MissionActionEntity.Cast(child);
			if (condition && conditions)
			{
				GC_MissionCondition definition = condition.BuildCondition(error);
				if (!definition)
					return false;
				conditions.Insert(definition);
			}
			else if (action && actions)
			{
				int insertAt;
				while (insertAt < orderedActions.Count() && orderedActions[insertAt].m_iOrder < action.m_iOrder)
					insertAt++;
				if (insertAt < orderedActions.Count() && orderedActions[insertAt].m_iOrder == action.m_iOrder)
				{
					error = "Sibling actions need distinct Order values: " + GetName();
					return false;
				}
				orderedActions.InsertAt(action, insertAt);
			}
			else
			{
				error = "Unsupported child under mission entity: " + GetName() + " / " + child.GetName();
				return false;
			}
			child = child.GetSibling();
		}
		foreach (GC_MissionActionEntity action : orderedActions)
		{
			GC_MissionAction definition = action.BuildAction(error);
			if (!definition)
				return false;
			actions.Insert(definition);
		}
		return true;
	}

#ifdef WORKBENCH
	//------------------------------------------------------------------------------------------------
	override array<ref ParamEnum> _WB_GetUserEnums(string varName, IEntitySource src)
	{
		if (varName != "m_sFlag" && varName != "m_sFaction" && varName != "m_sWinningFaction")
			return super._WB_GetUserEnums(varName, src);
		WorldEditorAPI api = _WB_GetEditorAPI();
		if (!api)
			return null;
		array<string> values;
		if (varName == "m_sFlag")
		{
			GC_MissionFlagChoices provider = new GC_MissionFlagChoices();
			values = provider.GetChoices(api);
		}
		else if (GC_PlayerCasualtiesConditionEntity.Cast(this))
		{
			GC_PlayableFactionChoices provider = new GC_PlayableFactionChoices();
			values = provider.GetChoices(api);
		}
		else
		{
			GC_MissionFactionChoices provider = new GC_MissionFactionChoices();
			values = provider.GetChoices(api);
		}
		values.Sort();
		array<ref ParamEnum> choices = {new ParamEnum("", "", "")};
		string previous;
		foreach (string value : values)
		{
			if (value.IsEmpty() || value == previous)
				continue;
			choices.Insert(new ParamEnum(value, value, ""));
			previous = value;
		}
		return choices;
	}
#endif
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		if (!SCR_Global.IsEditMode())
			s_aNodes.Insert(this);
	}

	//------------------------------------------------------------------------------------------------
	void GC_MissionAuthoringEntity(IEntitySource src, IEntity parent)
	{
		SetEventMask(EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	void ~GC_MissionAuthoringEntity()
	{
		s_aNodes.RemoveItem(this);
	}
}

class GC_MissionConditionEntityClass : GC_MissionAuthoringEntityClass
{
}

class GC_MissionConditionEntity : GC_MissionAuthoringEntity
{
	//------------------------------------------------------------------------------------------------
	GC_MissionCondition BuildCondition(out string error)
	{
		error = "Choose a condition entity type: " + GetName();
		return null;
	}
}

class GC_MissionActionEntityClass : GC_MissionAuthoringEntityClass
{
}

class GC_MissionActionEntity : GC_MissionAuthoringEntity
{
	//------------------------------------------------------------------------------------------------
	GC_MissionAction BuildAction(out string error)
	{
		error = "Choose an action entity type: " + GetName();
		return null;
	}
}
