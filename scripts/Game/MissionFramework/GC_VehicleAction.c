[BaseContainerProps(), BaseContainerCustomStringTitleField("Vehicle Fuel, Supplies and Repair Action")]
class GC_VehicleAction : GC_ActionBase
{
	[Attribute("", UIWidgets.EditComboBox, "Exact entity names to change. Missing entities and unsupported components are skipped. Fuel, supply storage and damage are changed on the existing entity, without replacing it or its occupants. Repair also works on other entities with a supported damage manager.", enums: {ParamEnum("", "", "rst-choices:2:GC_VehicleTargetChoices")})]
	ref array<string> m_aEntities;
	[Attribute("0", UIWidgets.CheckBox, "Set fuel to the selected level. Unchecked leaves fuel unchanged.")]
	bool m_bChangeFuel;
	[Attribute("1", UIWidgets.Slider, "Fuel level: 0 is empty, 0.5 is half full, and 1 is full. Includes attached fuel managers and storage tanks. Only used when Change Fuel is checked.", "0 1 0.01")]
	float m_fFuelRatio;
	[Attribute("0", UIWidgets.CheckBox, "Set carried supplies to Supplies Amount. Unchecked leaves supplies unchanged. This does not refill weapon magazines.")]
	bool m_bChangeSupplies;
	[Attribute("0", UIWidgets.EditBox, "Total supplies to carry, not an amount to add. Zero empties the supply container. The engine limits this to the container's capacity. Only used when Change Supplies is checked.", "0 1000000")]
	int m_iSuppliesAmount;
	[Attribute("0", UIWidgets.CheckBox, "Fully repair supported damage, including vehicle hit zones and a drowned engine. This does not replace deleted entities or restore missing parts by spawning them.")]
	bool m_bRepair;

	//------------------------------------------------------------------------------------------------
	override GC_RuleAction BuildAction()
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.VEHICLE;
		action.m_aFlagNames = {};
		if (m_aEntities)
			action.m_aFlagNames.Copy(m_aEntities);
		if (!action.m_aFlagNames.IsEmpty())
			action.m_sTextOrName = action.m_aFlagNames[0];
		action.m_bChangeFuel = m_bChangeFuel;
		action.m_fFuelRatio = m_fFuelRatio;
		action.m_bChangeSupplies = m_bChangeSupplies;
		action.m_iAmount = m_iSuppliesAmount;
		action.m_bRepair = m_bRepair;
		return action;
	}
}
