enum GC_ESetupBehavior
{
	SOFT,
	HARD,
	FREEZE
}

//! Enrollment and return history belong to this setup zone, never to the AO limit.
class GC_SetupPlayerState
{
	IEntity m_Character;
	IEntity m_Mover;
	vector m_aTransform[4];
	float m_fOutsideDeadline;
	bool m_bEnrolled;
	bool m_bReturnFailed;
	bool m_bHasSafeTransform;
}

[EntityEditorProps(category: "Lobby2/Mission", description: "Mission-start setup zone. Author a closed polygon; players starting inside are released when its timer expires.", color: "230 160 40 255")]
class GC_SetupZoneEntityClass : PolylineShapeEntityClass
{
}

class GC_SetupZoneEntity : PolylineShapeEntity
{
	[Attribute("120", UIWidgets.EditBox, "Seconds from the start of GAME until release (1-3600).", category: "Setup")]
	protected float m_fDuration;
	[Attribute("0", UIWidgets.ComboBox, "Soft: outside countdown then death. Hard: return to last safe position. Freeze: prevent movement until release.", enums: ParamEnumArray.FromEnum(GC_ESetupBehavior), category: "Setup")]
	protected GC_ESetupBehavior m_eBehavior;
	[Attribute("20", UIWidgets.EditBox, "Soft mode: continuous seconds outside before death (0.1-3600). Re-entry resets this countdown.", category: "Setup")]
	protected float m_fGraceSeconds;
	[Attribute("1", UIWidgets.CheckBox, "Block weapon fire, grenades, melee and turret fire for enrolled players until release.", category: "Setup")]
	protected bool m_bSafeStart;
	[Attribute("", UIWidgets.Auto, "Native faction keys. Empty means all factions. Eligibility is captured at the character's first observation in GAME.", category: "Setup")]
	protected ref array<string> m_aFactionKeys;
	protected ref array<vector> m_aPolygon = {};
	protected ref map<int, ref GC_SetupPlayerState> m_mPlayers = new map<int, ref GC_SetupPlayerState>();
	protected GC_LobbyGameMode m_Mode;
	protected float m_fDeadline;
	protected bool m_bStarted;
	protected bool m_bFinished;
	protected bool m_bValid;

	//------------------------------------------------------------------------------------------------
	void GetBoundaryPolygon(array<vector> points)
	{
		if (m_bValid)
			points.Copy(m_aPolygon);
	}

	//------------------------------------------------------------------------------------------------
	bool Contains(vector position)
	{
		return m_bValid && Math2D.IsPointInPolygonXZ(m_aPolygon, position);
	}

	//------------------------------------------------------------------------------------------------
	//! Trusted mission configuration, only before this zone starts. Points are world-space.
	bool Configure(array<vector> points, array<string> factions, float seconds, GC_ESetupBehavior behavior, float graceSeconds, bool safeStart)
	{
		if (!Replication.IsServer() || m_bStarted || !points || points.Count() < 3
			|| seconds < 1 || seconds > 3600 || graceSeconds < 0.1 || graceSeconds > 3600
			|| behavior < GC_ESetupBehavior.SOFT || behavior > GC_ESetupBehavior.FREEZE)
			return false;
		m_aPolygon.Copy(points);
		m_aFactionKeys = {};
		if (factions)
			m_aFactionKeys.Copy(factions);
		m_fDuration = seconds;
		m_eBehavior = behavior;
		m_fGraceSeconds = graceSeconds;
		m_bSafeStart = safeStart;
		m_bValid = Validate();
		return m_bValid;
	}

	//------------------------------------------------------------------------------------------------
	protected bool Validate()
	{
		if (m_aPolygon.Count() < 3 || m_fDuration < 1 || m_fDuration > 3600
			|| m_fGraceSeconds < 0.1 || m_fGraceSeconds > 3600
			|| m_eBehavior < GC_ESetupBehavior.SOFT || m_eBehavior > GC_ESetupBehavior.FREEZE)
			return false;
		float area;
		for (int i; i < m_aPolygon.Count(); i++)
		{
			vector first = m_aPolygon[i];
			vector second = m_aPolygon[(i + 1) % m_aPolygon.Count()];
			area += first[0] * second[2] - second[0] * first[2];
		}
		return Math.AbsFloat(area) > 0.01;
	}

	//------------------------------------------------------------------------------------------------
	protected void ReleasePlayers()
	{
		foreach (int playerId, GC_SetupPlayerState state : m_mPlayers)
		{
			GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			if (controller && controller.GC_OwnsSetupZone(this))
				controller.GC_SetSetupZone(null, null, 0, -1, false, false);
		}
		m_mPlayers.Clear();
	}

	//------------------------------------------------------------------------------------------------
	protected void Evaluate()
	{
		if (!Replication.IsServer() || !m_bValid || m_bFinished)
			return;
		if (!m_Mode || GC_LobbyGameMode.GetGameMode() != m_Mode || m_Mode.GetState() == SCR_EGameModeState.POSTGAME)
		{
			ReleasePlayers();
			m_bFinished = true;
			return;
		}
		if (m_Mode.GetState() != SCR_EGameModeState.GAME)
			return;
		float now = GetWorld().GetWorldTime() * 0.001;
		if (!m_bStarted)
		{
			m_bStarted = true;
			m_fDeadline = now + Math.Max(0, m_fDuration - m_Mode.GetElapsedTime());
		}
		if (now >= m_fDeadline)
		{
			ReleasePlayers();
			m_bFinished = true;
			PrintFormat("GC | Setup | released zone=%1", GetName());
			return;
		}
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (!manager)
			return;
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		array<IEntity> corrected = {};
		foreach (int playerId : players)
		{
			GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			if (!controller || controller.GetLobbyPlayerState() != GC_ELobbyPlayerState.PLAYING)
				continue;
			IEntity character = controller.GetControlledEntity();
			if (!ChimeraCharacter.Cast(character) || !SCR_AIDamageHandling.IsAlive(character))
				continue;
			IEntity mover = CompartmentAccessComponent.GetVehicleIn(character);
			if (!mover)
				mover = character;
			GC_SetupPlayerState state;
			if (!m_mPlayers.Find(playerId, state) || state.m_Character != character)
			{
				state = new GC_SetupPlayerState();
				state.m_Character = character;
				state.m_Mover = mover;
				mover.GetWorldTransform(state.m_aTransform);
				state.m_bHasSafeTransform = Contains(mover.GetOrigin());
				Faction faction = manager.GetPlayerFaction(playerId);
				bool matches = !m_aFactionKeys || m_aFactionKeys.IsEmpty() || (faction && m_aFactionKeys.Contains(faction.GetFactionKey()));
				state.m_bEnrolled = matches && Contains(character.GetOrigin()) && !controller.GC_GetSetupZone();
				m_mPlayers.Set(playerId, state);
			}
			if (!state.m_bEnrolled)
				continue;
			float outside = -1;
			bool inside = Contains(mover.GetOrigin());
			if (state.m_Mover != mover)
			{
				state.m_Mover = mover;
				// A different body needs its own safe pose, especially a newly boarded vehicle.
				state.m_bHasSafeTransform = inside;
				if (inside)
					mover.GetWorldTransform(state.m_aTransform);
			}
			if (m_eBehavior == GC_ESetupBehavior.SOFT)
			{
				if (inside)
					state.m_fOutsideDeadline = 0;
				else
				{
					if (state.m_fOutsideDeadline == 0)
						state.m_fOutsideDeadline = now + m_fGraceSeconds;
					outside = Math.Max(0, state.m_fOutsideDeadline - now);
					if (outside == 0)
					{
						SCR_DamageManagerComponent damage = SCR_DamageManagerComponent.Cast(character.FindComponent(SCR_DamageManagerComponent));
						if (damage)
							damage.SetHealthScaled(0);
					}
				}
			}
			else if (m_eBehavior == GC_ESetupBehavior.HARD && inside)
			{
				mover.GetWorldTransform(state.m_aTransform);
				state.m_bHasSafeTransform = true;
			}
			else if (!inside || (m_eBehavior == GC_ESetupBehavior.FREEZE && vector.DistanceSq(mover.GetOrigin(), state.m_aTransform[3]) > 0.0025))
			{
				// Multiple enrolled occupants must not correct the same body more than once.
				if (!corrected.Contains(mover))
				{
					SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(mover.FindComponent(SCR_EditableEntityComponent));
					bool accepted = state.m_bHasSafeTransform && editable && editable.GC_SetBoundaryTransform(state.m_aTransform);
					if (!accepted && !state.m_bReturnFailed)
						PrintFormat("GC | Setup | return unavailable zone=%1 player=%2 mover=%3", GetName(), playerId, mover, level: LogLevel.ERROR);
					state.m_bReturnFailed = !accepted;
					if (accepted)
						corrected.Insert(mover);
				}
			}
			controller.GC_SetSetupZone(this, character, m_fDeadline - now, outside, m_bSafeStart, m_eBehavior == GC_ESetupBehavior.FREEZE);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void Initialize()
	{
		m_Mode = GC_LobbyGameMode.GetGameMode();
		if (!m_Mode || !Replication.IsServer())
			return;
		if (!m_bValid)
		{
			GetPointsPositions(m_aPolygon);
			for (int i; i < m_aPolygon.Count(); i++)
				m_aPolygon[i] = CoordToParent(m_aPolygon[i]);
			m_bValid = IsClosed() && Validate();
		}
		if (!m_bValid)
			PrintFormat("GC | Setup | invalid polygon or settings: %1", GetName(), level: LogLevel.ERROR);
		GetGame().GetCallqueue().CallLater(Evaluate, 100, true);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		if (!SCR_Global.IsEditMode() && Replication.IsServer())
			GetGame().GetCallqueue().CallLater(Initialize, 0);
	}

	//------------------------------------------------------------------------------------------------
	void GC_SetupZoneEntity(IEntitySource src, IEntity parent)
	{
		SetEventMask(EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	void ~GC_SetupZoneEntity()
	{
		if (!GetGame())
			return;
		GetGame().GetCallqueue().Remove(Initialize);
		GetGame().GetCallqueue().Remove(Evaluate);
		if (Replication.IsServer())
			ReleasePlayers();
	}
}
