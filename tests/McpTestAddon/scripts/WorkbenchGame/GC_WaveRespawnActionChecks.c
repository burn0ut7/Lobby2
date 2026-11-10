#ifdef WORKBENCH
//! Explicit, non-gameplay checks of the production action's authoring and validation path.
[WorkbenchPluginAttribute(name: "Check respawn wave action", wbModules: {"WorldEditor"}, category: "GC Tests")]
class GC_WaveRespawnActionChecks : WorkbenchPlugin
{
	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		Resource resource = BaseContainerTools.CreateContainer("GC_WaveRespawnAction");
		if (!resource || !resource.IsValid())
		{
			Print("GC_WAVE_CHECK FAIL cannot create action", LogLevel.ERROR);
			return;
		}
		GC_WaveRespawnAction definition = GC_WaveRespawnAction.Cast(BaseContainerTools.CreateInstanceFromContainer(resource.GetResource().ToBaseContainer()));
		GC_RuleAction action = definition.BuildAction();
		Check(action.IsValid() && action.m_eDo == GC_ERuleAction.RESPAWN_WAVE && action.m_iAmount == 0
			&& action.m_bFlagAnswer && !action.m_bRandomOrder && action.m_aFlagNames.IsEmpty(), "editor defaults: all factions, unlimited, tickets, slot order");
		definition.m_aFactions = {"US", "USSR", "US"};
		definition.m_iMaximumPlayers = 5;
		definition.m_bUseTickets = false;
		definition.m_bRandomOrder = true;
		action = definition.BuildAction();
		Check(action.IsValid() && action.m_iAmount == 5 && !action.m_bFlagAnswer && action.m_bRandomOrder
			&& action.m_aFlagNames.Count() == 3, "configured scope, cap and ticket/order choices survive compilation");
		definition.m_aFactions.Clear();
		Check(action.m_aFlagNames.Count() == 3, "compiled faction list is independent of authoring edits");
		action.m_iAmount = -1;
		Check(!action.IsValid(), "negative cap rejected");
		action.m_iAmount = 1000001;
		Check(!action.IsValid(), "oversized cap rejected");
		action.m_iAmount = 0;
		action.m_aFlagNames = {""};
		Check(!action.IsValid(), "empty faction entry rejected, not interpreted as all factions");
		action.m_aFlagNames = {};
		for (int i = 0; i < 129; i++)
			action.m_aFlagNames.Insert("US");
		Check(!action.IsValid(), "oversized faction list rejected");
		Print("GC_WAVE_CHECK complete: 7 checks; see individual results. No world changes.");
	}

	//------------------------------------------------------------------------------------------------
	protected void Check(bool passed, string description)
	{
		if (passed)
			Print("GC_WAVE_CHECK PASS " + description);
		else
			Print("GC_WAVE_CHECK FAIL " + description, LogLevel.ERROR);
	}
}
#endif
