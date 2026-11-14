[BaseContainerProps(), BaseContainerCustomStringTitleField("Capture messages")]
class GC_CaptureMessages
{
	[Attribute("the objective", UIWidgets.EditBox, "Location inserted for %2. %1 is the selected faction names, or the selected force when no faction filter is set.")]
	string m_sLocation;
	[Attribute("", UIWidgets.Auto, "Only these factions receive messages. Empty sends to everyone. These are recipients, not capture contestants.")]
	ref array<FactionKey> m_aFactions;
	[Attribute("%1 captured %2.", UIWidgets.EditBox, "Changed to captured. Blank suppresses this message.")]
	string m_sCaptured;
	[Attribute("%1 is capturing %2.", UIWidgets.EditBox, "Capture started or resumed.")]
	string m_sCapturing;
	[Attribute("Capture of %2 has been interrupted.", UIWidgets.EditBox, "Capture progress interrupted.")]
	string m_sCaptureInterrupted;
	[Attribute("%1 lost %2.", UIWidgets.EditBox, "Changed to not captured.")]
	string m_sLost;
	[Attribute("%1 is losing %2.", UIWidgets.EditBox, "Loss started or resumed.")]
	string m_sLosing;
	[Attribute("Loss of %2 has been interrupted.", UIWidgets.EditBox, "Loss progress interrupted.")]
	string m_sLossInterrupted;

	//------------------------------------------------------------------------------------------------
	string GetText(int status)
	{
		switch (status)
		{
			case 0: return m_sCaptured;
			case 1: return m_sCapturing;
			case 2: return m_sCaptureInterrupted;
			case 3: return m_sLost;
			case 4: return m_sLosing;
			case 5: return m_sLossInterrupted;
		}
		return "";
	}

	//------------------------------------------------------------------------------------------------
	bool IsValid()
	{
		if (m_sLocation.Length() > 80 || m_sLocation.Contains("<") || m_sLocation.Contains(">"))
			return false;
		for (int i; i < 6; i++)
		{
			string message = GetText(i);
			if (message.Length() > 256 || message.Contains("<") || message.Contains(">"))
				return false;
		}
		if (m_aFactions)
		{
			if (m_aFactions.Count() > 256)
				return false;
			foreach (string faction : m_aFactions)
			{
				if (faction.IsEmpty())
					return false;
			}
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	GC_CaptureMessages Copy()
	{
		GC_CaptureMessages result = new GC_CaptureMessages();
		result.m_sLocation = m_sLocation;
		result.m_aFactions = {};
		if (m_aFactions)
			result.m_aFactions.Copy(m_aFactions);
		result.m_sCaptured = m_sCaptured;
		result.m_sCapturing = m_sCapturing;
		result.m_sCaptureInterrupted = m_sCaptureInterrupted;
		result.m_sLost = m_sLost;
		result.m_sLosing = m_sLosing;
		result.m_sLossInterrupted = m_sLossInterrupted;
		return result;
	}

	//------------------------------------------------------------------------------------------------
	GC_RuleAction BuildMessage(int status, array<string> capturingFactions, bool reverseStatus = false)
	{
		if (reverseStatus && status >= 0 && status < 6)
			status = (status + 3) % 6;
		string message = GetText(status);
		if (message.IsEmpty())
			return null;
		string names;
		if (capturingFactions)
		{
			foreach (string key : capturingFactions)
			{
				Faction faction = GetGame().GetFactionManager().GetFactionByKey(key);
				if (!names.IsEmpty())
					names += ", ";
				if (faction)
					names += WidgetManager.Translate(faction.GetFactionName());
				else
					names += key;
			}
		}
		if (names.IsEmpty())
			names = "The selected force";
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.MISSION_MESSAGE;
		action.m_sMessageTitle = "Objective Status";
		action.m_sTextOrName = string.Format(message, names, m_sLocation);
		action.m_aFlagNames = GC_FactionKeys.ToStrings(m_aFactions);
		return action;
	}
}
