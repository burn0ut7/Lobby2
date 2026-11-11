//! One listener per named AI group. Only the native group-info component owns control mode.
class GC_AIGroupObservation
{
	protected string m_sName;
	protected AIGroup m_Group;
	protected SCR_AIGroupInfoComponent m_Info;
	protected EGroupControlMode m_eMode;
	protected bool m_bActive;
	protected bool m_bReportedMissing;
	protected ref array<GC_RuleCondition> m_aConditions = {};
	protected ref set<GC_RuleCondition> m_aReached = new set<GC_RuleCondition>();
	protected ref ScriptInvoker m_OnChanged = new ScriptInvoker();

	//------------------------------------------------------------------------------------------------
	void GC_AIGroupObservation(string name)
	{
		m_sName = name;
	}

	//------------------------------------------------------------------------------------------------
	void AddCondition(GC_RuleCondition check)
	{
		if (!m_aConditions.Contains(check))
			m_aConditions.Insert(check);
	}

	//------------------------------------------------------------------------------------------------
	array<GC_RuleCondition> GetConditions()
	{
		return m_aConditions;
	}

	//------------------------------------------------------------------------------------------------
	ScriptInvoker GetOnChanged()
	{
		return m_OnChanged;
	}

	//------------------------------------------------------------------------------------------------
	//! Direct named lookup only when unbound; no world enumeration or proximity query.
	void Bind(BaseWorld world)
	{
		m_bActive = true;
		if (m_Group && m_Info)
			return;
		Unbind();
		if (!world)
			return;
		m_Group = AIGroup.Cast(world.FindEntityByName(m_sName));
		if (m_Group)
			m_Info = SCR_AIGroupInfoComponent.Cast(m_Group.FindComponent(SCR_AIGroupInfoComponent));
		if (!m_Info)
		{
			if (!m_bReportedMissing)
				Print("GC | Mission Framework | AI Group unavailable: '" + m_sName + "'. Requires an AI group with SCR_AIGroupInfoComponent; retrying during Match.", LogLevel.WARNING);
			m_bReportedMissing = true;
			return;
		}
		m_Info.GetOnControlModeChanged().Insert(OnControlModeChanged);
		RememberMode(m_Info.GetGroupControlMode());
	}

	//------------------------------------------------------------------------------------------------
	protected void RememberMode(EGroupControlMode mode)
	{
		m_eMode = mode;
		foreach (GC_RuleCondition check : m_aConditions)
		{
			if (!check.m_bAIGroupContinuous && check.m_eAIGroupMode == mode)
				m_aReached.Insert(check);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnControlModeChanged(EGroupControlMode mode)
	{
		if (!m_bActive)
			return;
		RememberMode(mode);
		m_OnChanged.Invoke(this);
	}

	//------------------------------------------------------------------------------------------------
	bool IsAvailable(GC_RuleCondition check)
	{
		return m_bActive && ((!check.m_bAIGroupContinuous && m_aReached.Contains(check)) || (m_Group && m_Info));
	}

	//------------------------------------------------------------------------------------------------
	bool Passes(GC_RuleCondition check)
	{
		return IsAvailable(check) && ((!check.m_bAIGroupContinuous && m_aReached.Contains(check)) || m_eMode == check.m_eAIGroupMode);
	}

	//------------------------------------------------------------------------------------------------
	protected void Unbind()
	{
		if (m_Info)
			m_Info.GetOnControlModeChanged().Remove(OnControlModeChanged);
		m_Info = null;
		m_Group = null;
	}

	//------------------------------------------------------------------------------------------------
	void Stop()
	{
		Unbind();
		m_bActive = false;
		m_aReached.Clear();
	}
}
