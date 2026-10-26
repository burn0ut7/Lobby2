[BaseContainerProps(), BaseContainerCustomStringTitleField("Show Message")]
class GC_ShowMessageAction : GC_MissionAction
{
	[Attribute("Mission Update", UIWidgets.EditBox, "Message title.")]
	string m_sTitle;
	[Attribute("", UIWidgets.EditBoxMultiline, "Message text.")]
	string m_sText;
	[Attribute("10", UIWidgets.EditBox, "Seconds to show the message.", params: "1 300 1")]
	float m_fSeconds;
	[Attribute("", UIWidgets.EditBox, "Recipient faction keys. Empty sends to everyone.")]
	ref array<string> m_aFactions;
	[Attribute("0", UIWidgets.CheckBox, "Only show to logged-in administrators.")]
	bool m_bAdminsOnly;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (m_sText.IsEmpty() || m_fSeconds < 1 || m_fSeconds > 300)
		{
			error = "Show Message: add text and a duration from 1 to 300 seconds.";
			return false;
		}
		if (m_aFactions)
		{
			foreach (string faction : m_aFactions)
			{
				if (!context.IsKnownFaction(faction))
				{
					error = "Show Message: unknown faction " + faction;
					return false;
				}
			}
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		return context.IsActive() && context.GetManager().GC_SendMissionMessage(m_sTitle, m_sText, m_fSeconds, m_aFactions, m_bAdminsOnly, false);
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Show Chat Message")]
class GC_ShowChatMessageAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Text to add to the native chat.")]
	string m_sText;
	[Attribute("0", UIWidgets.CheckBox, "Only show to logged-in administrators.")]
	bool m_bAdminsOnly;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (!m_sText.IsEmpty())
			return true;
		error = "Show Chat Message: add text.";
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		return context.IsActive() && context.GetManager().GC_SendMissionMessage("", m_sText, 10, null, m_bAdminsOnly, true);
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Delete Empty Slots")]
class GC_DeleteEmptySlotsAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Faction keys. Empty affects every faction. Claimed and reserved slots are preserved.")]
	ref array<string> m_aFactions;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (m_aFactions)
		{
			foreach (string faction : m_aFactions)
			{
				if (!context.IsKnownFaction(faction))
				{
					error = "Delete Empty Slots: unknown faction " + faction;
					return false;
				}
			}
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		return context.IsActive() && context.GetManager().GC_DeleteEmptyMissionSlots(m_aFactions, context);
	}
}

enum GC_EMissionNumberOperation
{
	SET,
	ADD,
	SUBTRACT,
	MULTIPLY,
	DIVIDE
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Change Respawn Tickets")]
class GC_ChangeRespawnTicketsAction : GC_MissionAction
{
	[Attribute("", UIWidgets.ComboBox, "Faction whose mission respawn budget changes. Each faction starts with zero tickets.", enums: {ParamEnum("", "", "rst-choices:1:GC_MissionFactionChoices")})]
	string m_sFaction;
	[Attribute("1", UIWidgets.ComboBox, "How to change the ticket count. Division rounds down; tickets never go below zero. Changes exceeding 100000 tickets are rejected.", enums: ParamEnumArray.FromEnum(GC_EMissionNumberOperation))]
	GC_EMissionNumberOperation m_eOperation;
	[Attribute("1", UIWidgets.EditBox, "Amount.", params: "0 100000 1")]
	int m_iAmount;
	[Attribute("1", UIWidgets.CheckBox, "During GAME, spend available tickets now on connected dead players in this faction. One ticket per successful respawn at the original slot location. This does not enable automatic future respawns.")]
	bool m_bRespawnNow;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (context.IsKnownFaction(m_sFaction) && m_eOperation >= 0 && m_eOperation <= 4 && m_iAmount >= 0 && m_iAmount <= 100000
			&& (m_eOperation != GC_EMissionNumberOperation.DIVIDE || m_iAmount > 0))
			return true;
		error = "Change Respawn Tickets: choose a known faction, operation and amount. Cannot divide by zero.";
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		return context.IsActive() && context.GetManager().GC_ChangeMissionTickets(m_sFaction, m_eOperation, m_iAmount, m_bRespawnNow);
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Set Briefing Visibility")]
class GC_SetBriefingVisibilityAction : GC_MissionAction
{
	[Attribute("1", UIWidgets.EditBox, "Briefing order number from Briefing Catalog.", params: "1 inf 1")]
	int m_iBriefing;
	[Attribute("1", UIWidgets.CheckBox, "Show the briefing. Off hides it.")]
	bool m_bVisible;
	[Attribute("0", UIWidgets.CheckBox, "Also replace the briefing's audience with Factions below.")]
	bool m_bChangeAudience;
	[Attribute("", UIWidgets.EditBox, "Faction keys allowed to see the briefing. Empty means everyone.")]
	ref array<string> m_aFactions;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (m_iBriefing <= 0 || m_iBriefing > GC_LobbyBriefingCatalog.MAX_BRIEFINGS)
		{
			error = "Set Briefing Visibility: choose a briefing order from 1 to 32.";
			return false;
		}
		if (m_bChangeAudience && m_aFactions)
		{
			foreach (string faction : m_aFactions)
			{
				if (!context.IsKnownFaction(faction))
				{
					error = "Set Briefing Visibility: unknown faction " + faction;
					return false;
				}
			}
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		if (!context.IsActive())
			return false;
		GC_LobbyManager manager = context.GetManager();
		GC_Briefing briefing = manager.GetBriefing(m_iBriefing);
		if (!briefing)
			return false;
		array<string> factions = briefing.GetFactionKeys();
		if (m_bChangeAudience)
			factions = m_aFactions;
		return manager.SetBriefing(briefing.GetOrder(), briefing.GetTitle(), briefing.GetContent(), factions,
			briefing.GetShowAtGameModeState(), briefing.IsGameModeStateInverted(), m_bVisible);
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Set Marker Visibility")]
class GC_SetMarkerVisibilityAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Names of Lobby2 briefing marker entities.")]
	ref array<string> m_aMarkers;
	[Attribute("1", UIWidgets.CheckBox, "Show these markers. Off hides them.")]
	bool m_bVisible;
	[Attribute("0", UIWidgets.CheckBox, "Also replace the markers' audience with Factions below.")]
	bool m_bChangeAudience;
	[Attribute("", UIWidgets.EditBox, "Faction keys allowed to see the markers. Empty means everyone.")]
	ref array<string> m_aFactions;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (!m_aMarkers || m_aMarkers.IsEmpty())
		{
			error = "Set Marker Visibility: add marker names.";
			return false;
		}
		if (m_bChangeAudience && m_aFactions)
		{
			foreach (string faction : m_aFactions)
			{
				if (!context.IsKnownFaction(faction))
				{
					error = "Set Marker Visibility: unknown faction " + faction;
					return false;
				}
			}
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		if (!context.IsActive())
			return false;
		GC_LobbyManager manager = context.GetManager();
		bool applied = true;
		foreach (string name : m_aMarkers)
		{
			GC_BriefingMarker marker = manager.GetBriefingMarker(name);
			if (!marker)
			{
				applied = false;
				continue;
			}
			array<string> factions = marker.GetFactionKeys();
			if (m_bChangeAudience)
				factions = m_aFactions;
			if (!manager.SetBriefingMarker(marker.GetName(), marker.GetImage(), marker.GetHighlightImage(), marker.GetQuadName(),
				marker.GetWorldPosition(), marker.GetRotation(), marker.IsWorldScale(), marker.GetSize(), marker.GetMinSize(),
				marker.GetDescription(), factions, marker.GetShowAtGameModeState(), marker.IsGameModeStateInverted(), m_bVisible, marker.GetColor()))
				applied = false;
		}
		return applied;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Run Custom Script")]
class GC_RunCustomScriptAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Name of an entity with your GC_MissionScriptComponent subclass. Advanced: override Run() in that component.")]
	string m_sEntity;
	[Attribute("0", UIWidgets.CheckBox, "Run on connected clients instead of the server. The named entity must be available on each client. Not replayed for late joiners.")]
	bool m_bOnClients;
	[Attribute("0", UIWidgets.CheckBox, "Allow this named script to run again.")]
	bool m_bAllowRepeat;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (!m_sEntity.IsEmpty())
			return true;
		error = "Run Custom Script: enter the script entity name.";
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		if (!context.IsActive() || !context.GetRuntime().ClaimScript(m_sEntity, m_bAllowRepeat))
			return false;
		return context.GetManager().GC_RunMissionScript(m_sEntity, m_bOnClients);
	}
}
