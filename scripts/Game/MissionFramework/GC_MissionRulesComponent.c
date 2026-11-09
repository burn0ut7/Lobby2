[ComponentEditorProps(category: "Lobby2/Mission", description: "Mission Framework: when something happens, check conditions, then run actions.")]
class GC_MissionRulesComponentClass : SCR_BaseGameModeComponentClass
{
}

class GC_MissionRulesComponent : SCR_BaseGameModeComponent
{
	[Attribute("1", UIWidgets.CheckBox, "Allow these rules to run on the server during the match. An empty rule list does nothing.", category: "Mission Framework")]
	protected bool m_bEnabled;
	[Attribute("", UIWidgets.Object, "Add rules here. Each rule has When, If, and Then settings. Rules are checked in list order.", category: "Mission Framework")]
	protected ref array<ref GC_MissionRule> m_aRules;
	protected ref GC_MissionRuleEngine m_Engine;
	protected float m_fStartedAt;
	protected bool m_bStarted;

	//------------------------------------------------------------------------------------------------
	protected bool HasAuthority()
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		return Replication.IsServer() && mode && mode == GetOwner() && mode.GetState() == SCR_EGameModeState.GAME;
	}

	//------------------------------------------------------------------------------------------------
	//! Trusted server extension point, not a player RPC or admin command.
	bool SendTestEvent(string name)
	{
		if (!HasAuthority() || !m_bEnabled || !m_Engine)
			return false;
		return m_Engine.SendTestEvent(name);
	}

	//------------------------------------------------------------------------------------------------
	bool GetFlag(string name)
	{
		return m_Engine && m_Engine.GetFlag(name);
	}

	//------------------------------------------------------------------------------------------------
	int GetRunCount()
	{
		if (!m_Engine)
			return 0;
		return m_Engine.GetRunCount();
	}

	//------------------------------------------------------------------------------------------------
	override void OnGameModeStart()
	{
		super.OnGameModeStart();
		if (m_bStarted || !m_bEnabled || !HasAuthority())
			return;
		m_bStarted = true;
		m_fStartedAt = GetGame().GetWorld().GetWorldTime();
		if (!m_aRules)
			m_aRules = {};
		m_Engine = new GC_MissionRuleEngine();
		m_Engine.Start(m_aRules);
		if (m_aRules && !m_aRules.IsEmpty() && m_Engine.IsRunning())
			GetGame().GetCallqueue().CallLater(TickRules, 100, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void TickRules()
	{
		if (!HasAuthority() || !m_bEnabled || !m_Engine || !m_Engine.IsRunning())
		{
			StopRules();
			return;
		}
		m_Engine.Advance((GetGame().GetWorld().GetWorldTime() - m_fStartedAt) * 0.001);
	}

	//------------------------------------------------------------------------------------------------
	protected void StopRules()
	{
		if (GetGame())
			GetGame().GetCallqueue().Remove(TickRules);
		if (m_Engine)
			m_Engine.Stop();
	}

	//------------------------------------------------------------------------------------------------
	override void OnGameStateChanged(SCR_EGameModeState state)
	{
		super.OnGameStateChanged(state);
		if (state == SCR_EGameModeState.POSTGAME)
			StopRules();
	}

	//------------------------------------------------------------------------------------------------
	override void OnGameEnd()
	{
		StopRules();
		super.OnGameEnd();
	}

	//------------------------------------------------------------------------------------------------
	void ~GC_MissionRulesComponent()
	{
		StopRules();
	}
}
