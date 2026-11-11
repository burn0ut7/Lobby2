//! Keep native and lobby group labels on the same authored manual call sign.
modded class SCR_CallsignGroupComponent
{
	//------------------------------------------------------------------------------------------------
	override bool GetCallsignNames(out string company, out string platoon, out string squad, out string character, out string format)
	{
		SCR_AIGroup group = SCR_AIGroup.Cast(GetOwner());
		string name;
		if (group)
			name = group.GC_GetManualCallsign();
		if (name.IsEmpty())
			return super.GetCallsignNames(company, platoon, squad, character, format);

		company = name;
		platoon = string.Empty;
		squad = string.Empty;
		character = string.Empty;
		format = "%1";
		return true;
	}
}
