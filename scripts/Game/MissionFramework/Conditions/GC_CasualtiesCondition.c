//! One complete casualty check; faction selection belongs to this condition.
[BaseContainerProps(), BaseContainerCustomStringTitleField("Casualties Condition")]
class GC_CasualtiesCondition : GC_ConditionBase
{
	[Attribute("", UIWidgets.EditComboBox, "Spectator bodies never count, with any faction or Count setting. Faction keys to include, such as US or USSR. Leave the list empty to include every faction. Selected factions are counted together, not checked separately.", enums: {ParamEnum("", "", "rst-choices:1:GC_FactionChoices")})]
	ref array<string> m_aFactions;

	[Attribute("0", UIWidgets.ComboBox, "Choose who to count at Match start. Players includes connected players' living claimed slot characters, even while possession is catching up. AI excludes spectators and unoccupied player slots. Later spawns and respawns do not change the starting group.", enums: {ParamEnum("Players", "0"), ParamEnum("AI", "1"), ParamEnum("Players and AI", "2")})]
	int m_iCount;

	[Attribute("1", UIWidgets.Slider, "Share of the starting group that must have died: 0.7 = 70%, 1 = everyone, and 0 = no deaths required. Passes when deaths reach or exceed this ratio. The group is the living characters at Match start, with their starting factions and Players/AI classification. Each death counts once; respawning does not undo it. Disconnecting or deleting a living character is not a death. An empty starting group never passes.", "0 1 0.01")]
	float m_fCasualtyRatio;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 7;
		check.m_iCasualtyCount = m_iCount;
		check.m_fCasualtyRatio = m_fCasualtyRatio;
		check.m_aCasualtyFactions = {};
		if (m_aFactions)
			check.m_aCasualtyFactions.Copy(m_aFactions);
		if (!check.IsValid(depth))
			return null;
		return check;
	}
}

//! Aggregates of starting lives, not current ownership or current faction membership.
class GC_CasualtyTotals
{
	string m_sFaction;
	bool m_bPlayers;
	int m_iStarting;
	int m_iDeaths;
	int m_iTotalDeaths;

	//------------------------------------------------------------------------------------------------
	bool Matches(array<string> factions, int selection)
	{
		return (!factions || factions.IsEmpty() || factions.Contains(m_sFaction))
			&& (selection == 2 || (selection == 0 && m_bPlayers) || (selection == 1 && !m_bPlayers));
	}
}

//! Read-only to condition evaluation. The active manager's tracker records confirmed deaths.
class GC_CasualtySnapshot
{
	ref array<ref GC_CasualtyTotals> m_aTotals = {};

	//------------------------------------------------------------------------------------------------
	protected GC_CasualtyTotals GetTotals(string faction, bool player)
	{
		foreach (GC_CasualtyTotals totals : m_aTotals)
		{
			if (totals.m_sFaction == faction && totals.m_bPlayers == player)
				return totals;
		}
		GC_CasualtyTotals created = new GC_CasualtyTotals();
		created.m_sFaction = faction;
		created.m_bPlayers = player;
		m_aTotals.Insert(created);
		return created;
	}

	//------------------------------------------------------------------------------------------------
	GC_CasualtyTotals AddStartingLife(string faction, bool player)
	{
		GC_CasualtyTotals totals = GetTotals(faction, player);
		totals.m_iStarting++;
		return totals;
	}

	//------------------------------------------------------------------------------------------------
	void RecordTotalDeath(string faction, bool player)
	{
		GC_CasualtyTotals totals = GetTotals(faction, player);
		totals.m_iTotalDeaths++;
	}

	//------------------------------------------------------------------------------------------------
	bool PassesDeathCount(array<string> factions, int selection, int comparison, int required)
	{
		int deaths;
		foreach (GC_CasualtyTotals totals : m_aTotals)
		{
			if (totals.Matches(factions, selection))
				deaths += totals.m_iTotalDeaths;
		}
		if (comparison == 0)
			return deaths > required;
		if (comparison == 1)
			return deaths < required;
		if (comparison == 2)
			return deaths == required;
		return deaths != required;
	}

	//------------------------------------------------------------------------------------------------
	bool Passes(array<string> factions, int selection, float ratio)
	{
		int starting;
		int deaths;
		foreach (GC_CasualtyTotals totals : m_aTotals)
		{
			if (!totals.Matches(factions, selection))
				continue;
			starting += totals.m_iStarting;
			deaths += totals.m_iDeaths;
		}
		if (starting == 0)
			return false;
		float lost = deaths;
		return lost / starting >= ratio;
	}
}
