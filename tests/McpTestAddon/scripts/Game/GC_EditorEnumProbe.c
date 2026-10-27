#ifdef WORKBENCH
#ifdef GC_EDITOR_ENUM_PROBE
//! Optional test-only controls for the native script enum callback.
modded class GC_LobbyManager
{
	[Attribute("", UIWidgets.EditComboBox, "Test-only direct component enum callback.", category: "Script Enum Probe")]
	string m_sGC_DirectEnumProbe;

	//------------------------------------------------------------------------------------------------
	override array<ref ParamEnum> _WB_GetUserEnums(string varName, IEntity owner, IEntityComponentSource src)
	{
		Print("GC_EDITOR_ENUM_PROBE component: " + varName);
		if (varName == "m_sGC_DirectEnumProbe" || varName == "m_sGC_NestedEnumProbe")
			return {new ParamEnum("Script choice one", "one", ""), new ParamEnum("Script choice two", "two", "")};

		return super._WB_GetUserEnums(varName, owner, src);
	}
}

modded class GC_MissionCondition
{
	[Attribute("", UIWidgets.EditComboBox, "Test-only nested container enum callback.")]
	string m_sGC_NestedEnumProbe;
}

modded class GC_LobbyManagerClass
{
	//------------------------------------------------------------------------------------------------
	static override array<ref ParamEnum> _WB_GetUserEnums(WorldEditorAPI api, IEntityComponentSource compSrc, IEntitySource entSrc, string varName)
	{
		Print("GC_EDITOR_ENUM_PROBE component class: " + varName);
		if (varName == "m_sGC_DirectEnumProbe" || varName == "m_sGC_NestedEnumProbe")
			return {new ParamEnum("Class choice one", "one", ""), new ParamEnum("Class choice two", "two", "")};

		return super._WB_GetUserEnums(api, compSrc, entSrc, varName);
	}
}

modded class GC_LobbyGameMode
{
	[Attribute("", UIWidgets.EditComboBox, "Test-only entity enum callback.", category: "Script Enum Probe")]
	string m_sGC_EntityEnumProbe;

	//------------------------------------------------------------------------------------------------
	override array<ref ParamEnum> _WB_GetUserEnums(string varName, IEntitySource src)
	{
		Print("GC_EDITOR_ENUM_PROBE entity: " + varName);
		if (varName == "m_sGC_EntityEnumProbe")
			return {new ParamEnum("Entity choice one", "one", ""), new ParamEnum("Entity choice two", "two", "")};

		return super._WB_GetUserEnums(varName, src);
	}
}
#endif
#endif

#ifdef WORKBENCH
#ifdef GC_ENUM_FUNCTION_PROBE
//! Function-backed attribute probe, enabled only during editor research.
modded class GC_MissionCondition
{
	[Attribute("", UIWidgets.EditComboBox, "Test-only function-backed enum.", enums: GetGC_ProbeChoices())]
	string m_sGC_FunctionEnumProbe;

	protected static int s_iGC_ProbeCalls;
	protected static ref ParamEnumArray s_aGC_ProbeChoices;

	//------------------------------------------------------------------------------------------------
	static ParamEnumArray GetGC_ProbeChoices()
	{
		s_iGC_ProbeCalls++;
		Print("GC_ENUM_FUNCTION_PROBE call " + s_iGC_ProbeCalls);
		if (!s_aGC_ProbeChoices)
			s_aGC_ProbeChoices = {new ParamEnum("Function initial", "initial", "")};
		return s_aGC_ProbeChoices;
	}

	//------------------------------------------------------------------------------------------------
	static void UpdateGC_ProbeChoices()
	{
		ParamEnumArray choices = GetGC_ProbeChoices();
		choices.Clear();
		choices.Insert(new ParamEnum("Function updated", "updated", ""));
		Print("GC_ENUM_FUNCTION_PROBE shared list updated");
	}
}

modded class GC_LobbyGameMode
{
	//------------------------------------------------------------------------------------------------
	override array<ref ParamEnum> _WB_GetUserEnums(string varName, IEntitySource src)
	{
		if (varName == "coords")
			GC_MissionCondition.UpdateGC_ProbeChoices();
		return super._WB_GetUserEnums(varName, src);
	}
}
#endif
#endif
