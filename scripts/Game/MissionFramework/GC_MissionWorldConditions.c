[BaseContainerProps(), BaseContainerCustomStringTitleField("AI Casualties (%)")]
class GC_AICasualtiesCondition : GC_MissionCondition
{
	[Attribute("", UIWidgets.ComboBox, "Faction key.", enums: {ParamEnum("(None)", "", "rst-choices:1:GC_MissionFactionChoices")})]
	string m_sFaction;
	[Attribute("1", UIWidgets.ComboBox, "Compare the percentage of AI lives lost.", enums: ParamEnumArray.FromEnum(GC_EMissionCountComparison))]
	GC_EMissionCountComparison m_eComparison;
	[Attribute("75", UIWidgets.EditBox, "AI deaths divided by AI lives observed during GAME, including reinforcements. Empty factions never match. Deleting an AI is not a death.", params: "0 100 1")]
	float m_fPercentage;
	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		context.RequireAI();
		if (context.IsKnownFaction(m_sFaction) && m_eComparison >= 0 && m_eComparison <= 2 && m_fPercentage >= 0 && m_fPercentage <= 100)
			return true;
		error = "AI Casualties: choose a known faction, comparison and percentage from 0 to 100.";
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool IsMet(GC_LobbyMissionContext context)
	{
		float percentage;
		return context.TryGetAICasualties(m_sFaction, percentage)
			&& GC_MissionCompare.Number(percentage, m_eComparison, m_fPercentage);
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Entity Damage State")]
class GC_EntityDamageCondition : GC_MissionCondition
{
	[Attribute("", UIWidgets.EditBox, "Unique name of the character, vehicle or other damageable entity.")]
	string m_sEntity;
	[Attribute("2", UIWidgets.ComboBox, "Damage state to match.", enums: ParamEnumArray.FromEnum(EDamageState))]
	EDamageState m_eState;
	[Attribute("", UIWidgets.EditBox, "Optional hit-zone name. Empty uses the default zone, usually the hull for vehicles.")]
	string m_sHitZone;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (!m_sEntity.IsEmpty())
			return true;
		error = "Entity Damage State: enter the entity name.";
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool IsMet(GC_LobbyMissionContext context)
	{
		IEntity entity = GetGame().GetWorld().FindEntityByName(m_sEntity);
		if (!entity)
			return false;
		SCR_DamageManagerComponent damage = SCR_DamageManagerComponent.GetDamageManager(entity);
		if (!damage)
			return false;
		HitZone zone = damage.GetDefaultHitZone();
		if (!m_sHitZone.IsEmpty())
			zone = damage.GetHitZoneByName(m_sHitZone);
		return zone && zone.GetDamageState() == m_eState;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("AI Group State")]
class GC_AIGroupStateCondition : GC_MissionCondition
{
	[Attribute("", UIWidgets.EditBox, "Unique name of the AI group.")]
	string m_sGroup;
	[Attribute("2", UIWidgets.ComboBox, "State to match. Autonomous is combat movement; soldiers may fight while their group is still following a waypoint.", enums: ParamEnumArray.FromEnum(EGroupControlMode))]
	EGroupControlMode m_eState;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (!m_sGroup.IsEmpty())
			return true;
		error = "AI Group State: enter the group name.";
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool IsMet(GC_LobbyMissionContext context)
	{
		IEntity entity = GetGame().GetWorld().FindEntityByName(m_sGroup);
		if (!entity)
			return false;
		SCR_AIGroupInfoComponent info = SCR_AIGroupInfoComponent.Cast(entity.FindComponent(SCR_AIGroupInfoComponent));
		return info && info.GetGroupControlMode() == m_eState;
	}
}

//! Area authoring is shared; no separate polling entity or trigger system.
[BaseContainerProps(), BaseContainerCustomStringTitleField("Area Condition (choose a type)")]
class GC_AreaCondition : GC_MissionCondition
{
	[Attribute("0", UIWidgets.CheckBox, "Draw this area during local Play. Testing only; not sent to remote clients.", category: "Debug")]
	bool m_bShowArea;
	[Attribute("", UIWidgets.EditBox, "Name of the area-center entity, or a polyline with at least three points. A polyline uses its polygon in X/Z; other entities use the radius.")]
	string m_sArea;
	[Attribute("25", UIWidgets.EditBox, "Radius in meters when the area is not a polygon.", params: "0.1 inf 1")]
	float m_fRadius;
	[Attribute("1", UIWidgets.ComboBox, "Compare the percentage inside or controlling the area.", enums: ParamEnumArray.FromEnum(GC_EMissionCountComparison))]
	GC_EMissionCountComparison m_eComparison;
	[Attribute("100", UIWidgets.EditBox, "Percentage from 0 to 100. An empty population never matches.", params: "0 100 1")]
	float m_fPercentage;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (m_sArea.IsEmpty() || m_fRadius <= 0 || m_fPercentage < 0 || m_fPercentage > 100 || m_eComparison < 0 || m_eComparison > 2)
		{
			error = "Area condition: choose an area name, positive radius and percentage from 0 to 100.";
			return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool Contains(IEntity area, vector position)
	{
		return GC_MissionArea.Contains(area, m_fRadius, position);
	}

	//------------------------------------------------------------------------------------------------
	bool Matches(int inside, int total)
	{
		return total > 0 && GC_MissionCompare.Number(100.0 * inside / total, m_eComparison, m_fPercentage);
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Faction In Area (%)")]
class GC_FactionInAreaCondition : GC_AreaCondition
{
	[Attribute("", UIWidgets.ComboBox, "Faction whose living members must reach the area.", enums: {ParamEnum("(None)", "", "rst-choices:1:GC_MissionFactionChoices")})]
	string m_sFaction;
	[Attribute("1", UIWidgets.CheckBox, "Only players. Off includes living AI.")]
	bool m_bPlayersOnly;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (!m_bPlayersOnly)
			context.RequireAI();
		if (!context.IsKnownFaction(m_sFaction))
		{
			error = "Faction In Area: choose a known faction.";
			return false;
		}
		return super.Validate(context, error);
	}

	//------------------------------------------------------------------------------------------------
	override bool IsMet(GC_LobbyMissionContext context)
	{
		IEntity area = GetGame().GetWorld().FindEntityByName(m_sArea);
		if (!area || !context.IsActive())
			return false;
		if (m_bShowArea)
			context.DrawArea(this, area, m_fRadius);
		int total;
		int inside;
		foreach (SCR_ChimeraCharacter character : context.GetCharacters())
		{
			if (!character || character.GetFactionKey() != m_sFaction
				|| (m_bPlayersOnly && GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(character) <= 0))
				continue;
			total++;
			if (Contains(area, character.GetOrigin()))
				inside++;
		}
		return Matches(inside, total);
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Area Control (%)")]
class GC_AreaControlCondition : GC_FactionInAreaCondition
{
	[Attribute("", UIWidgets.EditBox, "Factions to exclude from the population, for example civilians.")]
	ref array<string> m_aIgnoredFactions;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (m_aIgnoredFactions)
		{
			foreach (string faction : m_aIgnoredFactions)
			{
				if (!context.IsKnownFaction(faction))
				{
					error = "Area Control: unknown ignored faction " + faction;
					return false;
				}
			}
		}
		return super.Validate(context, error);
	}

	//------------------------------------------------------------------------------------------------
	override bool IsMet(GC_LobbyMissionContext context)
	{
		IEntity area = GetGame().GetWorld().FindEntityByName(m_sArea);
		if (!area || !context.IsActive())
			return false;
		if (m_bShowArea)
			context.DrawArea(this, area, m_fRadius);
		int total;
		int friendly;
		foreach (SCR_ChimeraCharacter character : context.GetCharacters())
		{
			if (!character || !Contains(area, character.GetOrigin())
				|| (m_bPlayersOnly && GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(character) <= 0))
				continue;
			string faction = character.GetFactionKey();
			if (faction.IsEmpty() || (m_aIgnoredFactions && m_aIgnoredFactions.Contains(faction)))
				continue;
			total++;
			if (faction == m_sFaction)
				friendly++;
		}
		return Matches(friendly, total);
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Entities In Area (%)")]
class GC_EntitiesInAreaCondition : GC_AreaCondition
{
	[Attribute("", UIWidgets.EditBox, "Unique entity names. Missing entities remain in the total and count as outside.")]
	ref array<string> m_aEntities;
	[Attribute("0", UIWidgets.CheckBox, "Count destroyed entities too.")]
	bool m_bIncludeDestroyed;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (!m_aEntities || m_aEntities.IsEmpty())
		{
			error = "Entities In Area: add entity names.";
			return false;
		}
		set<string> unique = new set<string>();
		foreach (string name : m_aEntities)
		{
			if (name.IsEmpty() || unique.Contains(name))
			{
				error = "Entities In Area: names must be nonempty and unique.";
				return false;
			}
			unique.Insert(name);
		}
		return super.Validate(context, error);
	}

	//------------------------------------------------------------------------------------------------
	override bool IsMet(GC_LobbyMissionContext context)
	{
		IEntity area = GetGame().GetWorld().FindEntityByName(m_sArea);
		if (!area)
			return false;
		if (m_bShowArea)
			context.DrawArea(this, area, m_fRadius);
		int inside;
		foreach (string name : m_aEntities)
		{
			IEntity entity = GetGame().GetWorld().FindEntityByName(name);
			if (!entity || !Contains(area, entity.GetOrigin()))
				continue;
			SCR_DamageManagerComponent damage = SCR_DamageManagerComponent.GetDamageManager(entity);
			if (m_bIncludeDestroyed || !damage || damage.GetState() != EDamageState.DESTROYED)
				inside++;
		}
		return Matches(inside, m_aEntities.Count());
	}
}
