//! Author-owned catalogue fields. Zero counts and empty strings mean unspecified.
//! The website reads these serialized fields directly from the mission header.
[BaseContainerProps()]
class GC_CatalogMissionMetadata
{
	[Attribute("1", desc: "Website catalogue contract version. Keep at 1.")]
	int m_iSchemaVersion;
	[Attribute(desc: "TVT, COOP, COTVT, LOL, OTHER, SD, AAS, TRNG or TEST.")]
	string m_sMissionType;
	[Attribute(desc: "Map display name, for example Arland. World resource identity remains in the mission header.")]
	string m_sMapName;
	[Attribute(desc: "Mission objectives: Defense, Attack & Defend, Assault, Convoy, HVT, Sweep, Hostage, Delay, Search & Rescue. Custom labels are allowed.")]
	ref array<string> m_aObjectives;
	[Attribute("0", desc: "Minimum intended players; 0 means unspecified. Maximum uses the header player count.")]
	int m_iMinPlayers;
	[Attribute("0", desc: "Recommended players; 0 means unspecified.")]
	int m_iRecommendedPlayers;
	[Attribute(desc: "Historical era, for example 1980s. Empty means unspecified.")]
	string m_sEra;
	[Attribute(desc: "Equipment available to players, not every object placed in the world.")]
	ref array<string> m_aPlayerAssets;
	[Attribute(desc: "Player-facing mod names. Required addon GUIDs are recorded separately.")]
	ref array<string> m_aMods;
	[Attribute(desc: "Author-declared required addon GUIDs (16 hexadecimal characters each). Not inferred from loaded addons.")]
	ref array<string> m_aRequiredAddonIds;
	[Attribute(desc: "Factions and forces. Specify whether a force is playable instead of assuming the opposition is a player team.")]
	ref array<ref GC_CatalogForce> m_aForces;
	[Attribute(desc: "Optional group identifier shared by variants of a mission.")]
	string m_sGroupId;
	[Attribute("-1", desc: "Similarity to other group variants: 0 to 1; -1 means unspecified.", params: "-1 1")]
	float m_fGroupSimilarity;
}

[BaseContainerProps()]
class GC_CatalogForce
{
	[Attribute(desc: "Faction key from the mission's faction manager.")]
	string m_sFactionKey;
	[Attribute(desc: "LIGHT, MOTORIZED, MECHANIZED, ARMORED, AIRMOBILE, AIRBORNE, COMBINED or OTHER.")]
	string m_sForceType;
	[Attribute(desc: "Optional relation or side label, for example friendly or opposing.")]
	string m_sSide;
	[Attribute("-1", desc: "Player-controlled force: 1 yes, 0 AI only, -1 unspecified.", params: "-1 1")]
	int m_iPlayable;
	[Attribute("0", desc: "Intended player slots for this force; 0 means unspecified. This is descriptive and does not alter slots.")]
	int m_iSlots;
}
