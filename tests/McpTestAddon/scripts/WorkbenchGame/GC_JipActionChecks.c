#ifdef WORKBENCH
[WorkbenchPluginAttribute(name: "Check JIP action", wbModules: {"WorldEditor"}, category: "GC Tests")]
class GC_JipActionChecks : WorkbenchPlugin
{
	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		Resource resource = BaseContainerTools.CreateContainer("GC_JipAction");
		if (!resource || !resource.IsValid())
		{
			Print("GC_JIP_ACTION_CHECK FAIL cannot create action", LogLevel.ERROR);
			return;
		}
		GC_JipAction definition = GC_JipAction.Cast(BaseContainerTools.CreateInstanceFromContainer(resource.GetResource().ToBaseContainer()));
		GC_RuleAction action = definition.BuildAction();
		Check(action.IsValid() && action.m_eDo == GC_ERuleAction.JIP && action.m_iOperation == GC_EJipMode.MANUAL, "editor defaults to Manual");
		definition.m_eJipMode = GC_EJipMode.AUTO;
		Check(definition.BuildAction().IsValid() && definition.BuildAction().m_iOperation == GC_EJipMode.AUTO, "Automatic accepted");
		definition.m_eJipMode = GC_EJipMode.DISABLED;
		Check(definition.BuildAction().IsValid() && definition.BuildAction().m_iOperation == GC_EJipMode.DISABLED, "Disabled accepted");
		action.m_iOperation = -1;
		Check(!action.IsValid(), "negative mode rejected");
		action.m_iOperation = 3;
		Check(!action.IsValid(), "unknown mode rejected");
		Print("GC_JIP_ACTION_CHECK complete: 5 checks; no world changes.");
	}

	//------------------------------------------------------------------------------------------------
	protected void Check(bool passed, string description)
	{
		if (passed)
			Print("GC_JIP_ACTION_CHECK PASS " + description);
		else
			Print("GC_JIP_ACTION_CHECK FAIL " + description, LogLevel.ERROR);
	}
}
#endif
