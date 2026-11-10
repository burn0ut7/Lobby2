[BaseContainerProps(description: "Shared action base; choose a concrete action instead.", visible: false, insertable: false), BaseContainerCustomStringTitleField("Action Base - Do not use")]
class GC_ActionBase
{
	//------------------------------------------------------------------------------------------------
	GC_RuleAction BuildAction()
	{
		return null;
	}
}
