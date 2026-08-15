//! Resolves the base game's authoritative group callsign into display text.
class GC_GroupCallsignHelper
{
	static string GetDisplayName(SCR_AIGroup group)
	{
		if (!group)
			return string.Empty;

		SCR_CallsignGroupComponent callsignComponent = SCR_CallsignGroupComponent.Cast(group.FindComponent(SCR_CallsignGroupComponent));
		if (callsignComponent)
		{
			string company;
			string platoon;
			string squad;
			string character;
			string format;
			if (callsignComponent.GetCallsignNames(company, platoon, squad, character, format))
				return WidgetManager.Translate(format, company, platoon, squad, character);
		}

		// Custom groups may not author a callsign component or valid indices.
		return group.GetName();
	}
}
