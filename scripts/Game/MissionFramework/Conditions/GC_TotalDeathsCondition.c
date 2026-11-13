//! Counts every confirmed death during Match, including replacement lives after respawn.
[BaseContainerProps(), BaseContainerCustomStringTitleField("Total Deaths Condition")]
class GC_TotalDeathsCondition : GC_ConditionBase
{
	[Attribute("", UIWidgets.EditComboBox, "Spectator bodies never count, with any faction or Count setting. Faction keys to include, such as US or USSR. Leave empty for every faction. Deaths from selected factions are added together. Uses the character's faction when it dies.", enums: {ParamEnum("", "", "gc-choices:1:GC_FactionChoices")})]
	ref array<string> m_aFactions;

	[Attribute("0", UIWidgets.ComboBox, "Players counts player lives, including claimed player-slot characters. AI counts mission AI, excluding spectators and unoccupied playable slots. Players and AI counts both. Respawned characters and later reinforcements can add deaths.", enums: {ParamEnum("Players", "0"), ParamEnum("AI", "1"), ParamEnum("Players and AI", "2")})]
	int m_iCount;

	[Attribute("0", UIWidgets.ComboBox, "Compare deaths since Match started with Required Deaths. Greater than 4 means five or more. Less than 5 means zero to four. Equal means exactly that number; Not equal means any other number.", enums: {ParamEnum("Greater than", "0"), ParamEnum("Less than", "1"), ParamEnum("Equal", "2"), ParamEnum("Not equal", "3")})]
	int m_iComparison;

	[Attribute("0", UIWidgets.SpinBox, "Number of deaths to compare against. Each life counts once when it dies; the same player can add another death after respawning. Disconnecting or deleting a living character does not add a death. Starts at zero at Match start.", "0 1000000 1")]
	int m_iRequiredDeaths;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 8;
		check.m_iCasualtyCount = m_iCount;
		check.m_iDeathComparison = m_iComparison;
		check.m_iRequiredDeaths = m_iRequiredDeaths;
		check.m_aCasualtyFactions = {};
		if (m_aFactions)
			check.m_aCasualtyFactions.Copy(m_aFactions);
		if (!check.IsValid(depth))
			return null;
		return check;
	}
}
