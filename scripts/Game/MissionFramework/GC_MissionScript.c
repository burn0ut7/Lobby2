//! One compiled script hook for both actions and conditions. Keep condition implementations read-only.
[BaseContainerProps(visible: false, insertable: false)]
class GC_MissionScript
{
	//------------------------------------------------------------------------------------------------
	//! Override to validate custom settings if needed. Do not change world state here.
	bool IsValid()
	{
		return Type() != GC_MissionScript;
	}

	//------------------------------------------------------------------------------------------------
	//! Called synchronously on the authoritative server. Action: success. Condition: truth value.
	bool Run(GC_LobbyManager manager)
	{
		return false;
	}
}
