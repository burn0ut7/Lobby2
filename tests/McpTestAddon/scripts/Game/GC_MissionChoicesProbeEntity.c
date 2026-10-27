[EntityEditorProps(category: "GameScripted/Lobby2 Tests", description: "Experimental script-only mission dropdowns. No gameplay behavior.")]
class GC_MissionChoicesProbeEntityClass : GenericEntityClass
{
}

//! Optional authoring experiment; never registers or executes a mission event.
class GC_MissionChoicesProbeEntity : GenericEntity
{
	[Attribute("", UIWidgets.EditComboBox, "Choose an existing mission flag or type a name.", category: "Mission Dropdown Prototype")]
	string m_sFlag;

	[Attribute("", UIWidgets.EditComboBox, "Choose a faction from playable groups with slots or type a key.", category: "Mission Dropdown Prototype")]
	string m_sPlayableFaction;

	[Attribute("", UIWidgets.EditComboBox, "Choose a faction from the faction manager or type a key.", category: "Mission Dropdown Prototype")]
	string m_sFaction;

#ifdef WORKBENCH
	//------------------------------------------------------------------------------------------------
	override array<ref ParamEnum> _WB_GetUserEnums(string varName, IEntitySource src)
	{
		if (varName != "m_sFlag" && varName != "m_sPlayableFaction" && varName != "m_sFaction")
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
		else if (varName == "m_sPlayableFaction")
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
		Print("GC_ENTITY_CHOICES_PROBE " + varName + " choices=" + choices.Count());
		return choices;
	}
#endif
}
