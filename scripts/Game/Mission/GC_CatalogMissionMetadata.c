//! Author-owned catalogue fields. Zero counts and empty strings mean unspecified.
//! The website reads these serialized fields directly from the mission header.
[BaseContainerProps()]
class GC_CatalogMissionMetadata
{
	[Attribute("1", UIWidgets.Hidden)]
	int m_iSchemaVersion;
	[Attribute("", UIWidgets.ComboBox, "Choose how people play this mission.", enums: { ParamEnum("Not set", ""), ParamEnum("CO-OP - Cooperative", "COOP"), ParamEnum("TVT - Team versus team", "TVT"), ParamEnum("COTVT - Cooperative and team versus team", "COTVT"), ParamEnum("Training", "TRNG"), ParamEnum("LOL - Non-serious mission", "LOL"), ParamEnum("Testing", "TEST"), ParamEnum("Broken", "BROKEN") })]
	string m_sMissionType;
	[Attribute("", UIWidgets.Hidden)]
	string m_sMapName;
	[Attribute("", UIWidgets.ComboBox, "Press + and choose an objective. Add more if needed.", enums: { ParamEnum("Not set", ""), ParamEnum("Defense", "Defense"), ParamEnum("Attack & Defend", "Attack & Defend"), ParamEnum("Assault", "Assault"), ParamEnum("Convoy", "Convoy"), ParamEnum("High-value target (HVT)", "HVT"), ParamEnum("Sweep", "Sweep"), ParamEnum("Hostage", "Hostage"), ParamEnum("Delay", "Delay"), ParamEnum("Search & Rescue", "Search & Rescue") })]
	ref array<string> m_aObjectives;
	[Attribute("0", UIWidgets.Hidden)]
	int m_iMinPlayers;
	[Attribute("0", UIWidgets.Hidden)]
	int m_iRecommendedPlayers;
	[Attribute("", UIWidgets.EditBox, "Time period, for example 1980s. Optional.", category: "Optional details")]
	string m_sEra;
	[Attribute("", UIWidgets.ComboBox, "Press + and choose equipment players can use.", enums: { ParamEnum("Not set", ""), ParamEnum("Infantry", "Infantry"), ParamEnum("Cars / light vehicles", "Cars"), ParamEnum("Trucks", "Trucks"), ParamEnum("APCs", "APCs"), ParamEnum("IFVs", "IFVs"), ParamEnum("Tanks", "Tanks"), ParamEnum("Transport helicopters", "Transport helicopters"), ParamEnum("Attack helicopters", "Attack helicopters"), ParamEnum("Aircraft", "Aircraft"), ParamEnum("Boats", "Boats"), ParamEnum("Mortars", "Mortars"), ParamEnum("Artillery", "Artillery"), ParamEnum("Drones", "Drones") })]
	ref array<string> m_aPlayerAssets;
	[Attribute("", UIWidgets.EditBox, "Optional mod names to show on the website. One name per entry.", category: "Optional details")]
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
