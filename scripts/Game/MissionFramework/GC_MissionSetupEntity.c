[EntityEditorProps(category: "Lobby2/Mission", description: "Mission Setup: named Events run Actions when their Conditions are met. Place one per mission. Runs only on the Lobby2 server during GAME.")]
class GC_MissionSetupEntityClass : GenericEntityClass
{
}

class GC_MissionSetupEntity : GenericEntity
{
	[Attribute("", UIWidgets.Object, "Events. Each runs once. Invalid configuration prevents the whole setup from running.", category: "Mission Setup")]
	protected ref array<ref GC_MissionEvent> m_aEvents;
	protected ref GC_MissionRuntime m_Runtime;
	protected static GC_MissionSetupEntity s_Active;

	//------------------------------------------------------------------------------------------------
	protected void Initialize()
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!Replication.IsServer() || !mode)
			return;
		if (s_Active && s_Active != this)
		{
			s_Active.Stop();
			Print("GC | Mission Framework | multiple Mission Setup entities: execution stopped; keep exactly one", LogLevel.ERROR);
			return;
		}
		s_Active = this;
		GC_LobbyMissionContext context = new GC_LobbyMissionContext(mode);
		m_Runtime = new GC_MissionRuntime();
		if (m_Runtime.Start(context, m_aEvents))
			GetGame().GetCallqueue().CallLater(Tick, 250, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void Tick()
	{
		if (!GC_LobbyGameMode.GetGameMode() || GC_LobbyGameMode.GetGameMode().GetState() == SCR_EGameModeState.POSTGAME)
		{
			Stop();
			return;
		}
		m_Runtime.Update(GetWorld().GetWorldTime() * 0.001);
	}

	//------------------------------------------------------------------------------------------------
	protected void Stop()
	{
		if (GetGame())
			GetGame().GetCallqueue().Remove(Tick);
		if (m_Runtime)
			m_Runtime.Stop();
	}

	//------------------------------------------------------------------------------------------------
	string Describe()
	{
		if (!m_Runtime)
			return "inactive";
		return m_Runtime.Describe();
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		if (GetGame().InPlayMode() && Replication.IsServer())
			GetGame().GetCallqueue().CallLater(Initialize, 0);
	}

	//------------------------------------------------------------------------------------------------
	void GC_MissionSetupEntity(IEntitySource src, IEntity parent)
	{
		SetEventMask(EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	void ~GC_MissionSetupEntity()
	{
		Stop();
		if (GetGame())
			GetGame().GetCallqueue().Remove(Initialize);
		if (s_Active == this)
			s_Active = null;
	}
}
