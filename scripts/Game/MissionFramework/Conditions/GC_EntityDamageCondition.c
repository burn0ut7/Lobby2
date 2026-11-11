[BaseContainerProps(), BaseContainerCustomStringTitleField("Advanced damage settings")]
class GC_EntityDamageOptions
{
	[Attribute("", UIWidgets.EditBox, "Optional damage area name, such as an engine or wheel. Leave blank to check the entity's main damage area. Use the exact area name from its damage component; this does not search child entities.")]
	string m_sDamageArea;
}

//! Observes an existing damage component; never adds components or changes health.
[BaseContainerProps(), BaseContainerCustomStringTitleField("Entity Damage Condition")]
class GC_EntityDamageCondition : GC_ConditionBase
{
	[Attribute("", UIWidgets.EditComboBox, "Exact, unique name of an entity in the world, not a prefab file path or player name. Works with characters, vehicles and destructible objects that have a supported damage component. Missing or unsupported targets cannot pass, including inside a None group.", enums: {ParamEnum("", "", "rst-choices:2:GC_DamageEntityChoices")})]
	string m_sEntityName;

	[Attribute("0", UIWidgets.ComboBox, "Destroyed checks whether the selected damage area is destroyed (dead for a character's main area). Not destroyed checks the opposite. Damage amount checks whether damage has reached Damage Needed. Repairs update the answer; the event's Advanced settings control repetition.", enums: {ParamEnum("Destroyed", "0"), ParamEnum("Not destroyed", "1"), ParamEnum("Damage amount", "2")})]
	int m_iCheck;

	[Attribute("1", UIWidgets.Slider, "Only used by Damage amount. Minimum damage from 0 to 1: 0.5 means at least 50% damage, 1 means all health is gone, and 0 includes an undamaged target. Use Destroyed to check the engine's actual destroyed state.", "0 1 0.01")]
	float m_fDamageNeeded;

	[Attribute("", UIWidgets.Object, "Optional specific damage area. Leave unset to check the main area using the same damage system for characters, vehicles and buildings.")]
	ref GC_EntityDamageOptions m_Advanced;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 9;
		check.m_sDamageEntity = m_sEntityName;
		check.m_iDamageCheck = m_iCheck;
		check.m_fDamageNeeded = m_fDamageNeeded;
		if (m_Advanced)
			check.m_sDamageArea = m_Advanced.m_sDamageArea;
		if (!check.IsValid(depth))
			return null;
		return check;
	}
}
