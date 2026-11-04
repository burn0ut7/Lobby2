//! Author-owned catalogue fields. Zero counts and empty strings mean unspecified.
//! The website reads these serialized fields directly from the mission header.
[BaseContainerProps()]
class GC_CatalogMissionMetadata
{
	[Attribute("1", UIWidgets.Hidden)]
	int m_iSchemaVersion;
	[Attribute("", UIWidgets.EditBox, "Time period, for example 1980s. Optional.")]
	string m_sEra;
	[Attribute("", UIWidgets.ComboBox, "Choose how people play this mission.", enums: GC_MissionCatalogChoices.MissionTypes())]
	string m_sMissionType;
	[Attribute("", UIWidgets.Hidden)]
	string m_sMapName;
	[Attribute("", UIWidgets.ComboBox, "Press + and choose an objective. Add more if needed.", enums: GC_MissionCatalogChoices.Objectives())]
	ref array<string> m_aObjectives;
	[Attribute("0", UIWidgets.EditBox, "Minimum players needed; zero means unspecified.")]
	int m_iMinPlayers;
	[Attribute("0", UIWidgets.Hidden)]
	int m_iRecommendedPlayers;
	[Attribute("", UIWidgets.ComboBox, "Choose the force types and support available to players.", enums: GC_MissionCatalogChoices.ForceComposition())]
	ref array<string> m_aPlayerAssets;
	[Attribute("", UIWidgets.Hidden)]
	ref array<string> m_aMods;
	[Attribute("", UIWidgets.Hidden)]
	ref array<string> m_aRequiredAddonIds;
	[Attribute("", UIWidgets.Hidden)]
	ref array<ref GC_CatalogForce> m_aFactions;

	// Retain old serialized data for compatibility; no longer author-facing.
	[Attribute("", UIWidgets.Hidden)]
	ref array<ref GC_CatalogForce> m_aForces;
	[Attribute("", UIWidgets.Hidden)]
	string m_sGroupId;
	[Attribute("-1", UIWidgets.Hidden)]
	float m_fGroupSimilarity;
}

[BaseContainerProps()]
class GC_CatalogForce
{
	[Attribute(desc: "Faction key from the mission's faction manager.")]
	string m_sFactionKey;
	[Attribute("", UIWidgets.ComboBox, "Choose the team's equipment style.", enums: { ParamEnum("Not set", ""), ParamEnum("Light infantry", "LIGHT"), ParamEnum("Motorized", "MOTORIZED"), ParamEnum("Mechanized", "MECHANIZED"), ParamEnum("Armored", "ARMORED"), ParamEnum("Airmobile", "AIRMOBILE"), ParamEnum("Airborne", "AIRBORNE"), ParamEnum("Combined arms", "COMBINED"), ParamEnum("Other", "OTHER") })]
	string m_sForceType;
	[Attribute(desc: "Optional relation or side label, for example friendly or opposing.")]
	string m_sSide;
	[Attribute("-1", UIWidgets.ComboBox, "Who controls this team?", enums: { ParamEnum("Not set", "-1"), ParamEnum("Players", "1"), ParamEnum("AI only", "0") })]
	int m_iPlayable;
	[Attribute("0", desc: "Intended player slots for this force; 0 means unspecified. This is descriptive and does not alter slots.")]
	int m_iSlots;
}

//! Shared labels for the native header and the compact Workbench form.
class GC_MissionCatalogChoices
{
	//------------------------------------------------------------------------------------------------
	static ParamEnumArray MissionTypes()
	{
		ParamEnumArray choices = new ParamEnumArray();
		choices.Insert(new ParamEnum("Not set", ""));
		choices.Insert(new ParamEnum("CO-OP - Cooperative", "COOP"));
		choices.Insert(new ParamEnum("TVT - Team versus team", "TVT"));
		choices.Insert(new ParamEnum("COTVT - Cooperative and team versus team", "COTVT"));
		choices.Insert(new ParamEnum("Training", "TRNG"));
		choices.Insert(new ParamEnum("LOL - Non-serious mission", "LOL"));
		choices.Insert(new ParamEnum("Testing", "TEST"));
		choices.Insert(new ParamEnum("Broken", "BROKEN"));
		return choices;
	}

	//------------------------------------------------------------------------------------------------
	static ParamEnumArray Objectives()
	{
		ParamEnumArray choices = new ParamEnumArray();
		choices.Insert(new ParamEnum("Not set", ""));
		choices.Insert(new ParamEnum("Defense", "Defense"));
		choices.Insert(new ParamEnum("Attack & Defend", "Attack & Defend"));
		choices.Insert(new ParamEnum("Assault", "Assault"));
		choices.Insert(new ParamEnum("Convoy", "Convoy"));
		choices.Insert(new ParamEnum("High-value target (HVT)", "HVT"));
		choices.Insert(new ParamEnum("Sweep", "Sweep"));
		choices.Insert(new ParamEnum("Hostage", "Hostage"));
		choices.Insert(new ParamEnum("Delay", "Delay"));
		choices.Insert(new ParamEnum("Search & Rescue", "Search & Rescue"));
		return choices;
	}

	//------------------------------------------------------------------------------------------------
	static ParamEnumArray ForceComposition()
	{
		ParamEnumArray choices = new ParamEnumArray();
		choices.Insert(new ParamEnum("Not set", ""));
		choices.Insert(new ParamEnum("Infantry", "Infantry", "Troops fighting on foot."));
		choices.Insert(new ParamEnum("Motorized", "Motorized", "Troops moving by car or truck."));
		choices.Insert(new ParamEnum("Mechanized", "Mechanized", "Infantry operating with APCs or IFVs."));
		choices.Insert(new ParamEnum("Armored", "Armored", "Tank units."));
		choices.Insert(new ParamEnum("Airborne", "Airborne", "Parachute troops, helicopters and other crewed aircraft."));
		choices.Insert(new ParamEnum("Naval", "Naval", "Boat or ship operations."));
		choices.Insert(new ParamEnum("Mortars", "Mortars", "Mortar support."));
		choices.Insert(new ParamEnum("Artillery", "Artillery", "Artillery support."));
		choices.Insert(new ParamEnum("Drones", "Drones", "Uncrewed aircraft or systems."));
		return choices;
	}
}
