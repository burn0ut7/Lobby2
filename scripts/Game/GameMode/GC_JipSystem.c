enum GC_EJipMode
{
	MANUAL,
	AUTO,
	DISABLED
}

//! Mission-lifetime account record. Never keyed by a slot or a reusable player ID.
class GC_JipPlayer
{
	bool m_bEligible;
	bool m_bOffered;
	bool m_bConsumed;
	bool m_bPending;
	RplId m_Character = RplId.Invalid();
}

//! All policy, membership, destination selection and completion checks run on authority.
class GC_JipSystem
{
	protected GC_LobbyGameMode m_Mode;
	protected ref map<string, ref GC_JipPlayer> m_mPlayers = new map<string, ref GC_JipPlayer>();
	protected ref map<int, float> m_mConnections = new map<int, float>();
	static const float MINIMUM_JOIN_SECONDS = 120;

	//------------------------------------------------------------------------------------------------
	void Start(GC_LobbyGameMode mode)
	{
		m_Mode = mode;
		GetGame().GetCallqueue().CallLater(Tick, 1000, true);
	}

	//------------------------------------------------------------------------------------------------
	void Stop()
	{
		GetGame().GetCallqueue().Remove(Tick);
		m_Mode = null;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsActive()
	{
		return Replication.IsServer() && m_Mode && GC_LobbyGameMode.GetGameMode() == m_Mode;
	}

	//------------------------------------------------------------------------------------------------
	float Elapsed()
	{
		if (!m_Mode || m_Mode.GetState() != SCR_EGameModeState.GAME)
			return -1;
		return m_Mode.GetElapsedTime();
	}

	//------------------------------------------------------------------------------------------------
	void Connected(int playerId)
	{
		if (IsActive() && !m_mConnections.Contains(playerId))
			m_mConnections.Set(playerId, Elapsed());
	}

	//------------------------------------------------------------------------------------------------
	void Disconnected(int playerId)
	{
		m_mConnections.Remove(playerId);
	}

	//------------------------------------------------------------------------------------------------
	GC_JipPlayer Register(int playerId)
	{
		if (!IsActive())
			return null;
		string identity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		if (identity.IsEmpty())
			return null;
		GC_JipPlayer record = m_mPlayers.Get(identity);
		if (record)
			return record;
		Connected(playerId);
		record = new GC_JipPlayer();
		record.m_bEligible = m_mConnections.Get(playerId) >= MINIMUM_JOIN_SECONDS && WindowOpen();
		m_mPlayers.Set(identity, record);
		return record;
	}

	//------------------------------------------------------------------------------------------------
	protected bool WindowOpen()
	{
		return IsActive() && m_Mode.GetState() == SCR_EGameModeState.GAME
			&& m_Mode.GetJipMode() != GC_EJipMode.DISABLED
			&& (m_Mode.GetJipDenySeconds() <= 0 || Elapsed() < m_Mode.GetJipDenySeconds());
	}

	//------------------------------------------------------------------------------------------------
	protected ChimeraCharacter LivingCharacter(int playerId)
	{
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players.IsPlayerConnected(playerId))
			return null;
		ChimeraCharacter character = ChimeraCharacter.Cast(players.GetPlayerControlledEntity(playerId));
		if (!character || character.FindComponent(GC_LobbySpectatorBodyComponent))
			return null;
		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller || controller.IsDead() || controller.IsUnconscious())
			return null;
		return character;
	}

	//------------------------------------------------------------------------------------------------
	protected bool FactionAllowed(int playerId)
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		Faction faction;
		if (manager)
			faction = manager.GetPlayerFaction(playerId);
		return faction && m_Mode.IsJipFactionAllowed(faction.GetFactionKey());
	}

	//------------------------------------------------------------------------------------------------
	protected void Tick()
	{
		if (!IsActive())
			return;
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		foreach (int playerId : players)
		{
			GC_JipPlayer record = Register(playerId);
			if (!record || !record.m_bEligible || record.m_bConsumed)
				continue;
			if (!WindowOpen())
			{
				record.m_bConsumed = true;
				continue;
			}
			ChimeraCharacter character = LivingCharacter(playerId);
			if (record.m_bOffered)
			{
				ChimeraCharacter current = ChimeraCharacter.Cast(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId));
				if (!current || Replication.FindItemId(current) != record.m_Character || current.GetCharacterController().IsDead())
					record.m_bConsumed = true;
				continue;
			}
			if (!character)
				continue;
			GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			RplId characterId = Replication.FindItemId(character);
			if (!controller || !characterId.IsValid())
				continue;
			record.m_bOffered = true;
			record.m_Character = characterId;
			if (!FactionAllowed(playerId))
			{
				record.m_bConsumed = true;
				continue;
			}
			controller.GC_SendJipOffer(characterId, m_Mode.GetJipMode() == GC_EJipMode.AUTO);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! No target, coordinates, account or player ID is accepted from the client.
	void Request(GC_LobbyPlayerController controller)
	{
		if (!IsActive() || !controller)
			return;
		int playerId = controller.GetPlayerId();
		if (GetGame().GetPlayerManager().GetPlayerController(playerId) != controller)
			return;
		string identity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		GC_JipPlayer record = m_mPlayers.Get(identity);
		ChimeraCharacter character = LivingCharacter(playerId);
		if (identity.IsEmpty() || !WindowOpen() || !record || !record.m_bEligible || !record.m_bOffered || record.m_bConsumed
			|| !character || Replication.FindItemId(character) != record.m_Character || !FactionAllowed(playerId))
		{
			controller.GC_SendJipMessage("#GC-Jip_Unavailable");
			return;
		}
		if (record.m_bPending)
		{
			controller.GC_SendJipMessage("#GC-Jip_Pending");
			return;
		}
		if (character.GetRootParent() != character || CompartmentAccessComponent.GetVehicleIn(character))
		{
			controller.GC_SendJipMessage("#GC-Jip_LeaveVehicle");
			return;
		}
		ChimeraCharacter target = ResolveTarget(playerId);
		if (!target)
		{
			controller.GC_SendJipMessage("#GC-Jip_NoTarget");
			return;
		}
		IEntity vehicle = CompartmentAccessComponent.GetVehicleIn(target);
		if (vehicle)
		{
			controller.GC_BeginJipVehicle(character, target, vehicle);
			return;
		}
		vector destination;
		if (!FindBehindDestination(character, target, destination))
		{
			controller.GC_SendJipMessage("#GC-Jip_NoLanding");
			return;
		}
		record.m_bPending = true;
		controller.GC_DispatchJip(record.m_Character, destination, target.GetCharacterController().GetStance());
	}

	//------------------------------------------------------------------------------------------------
	//! Keep the landing on the target's rear axis; never accept a sideways/front fallback.
	static bool FindBehindDestination(ChimeraCharacter character, ChimeraCharacter target, out vector destination)
	{
		BaseWorld world = character.GetWorld();
		vector origin = target.GetOrigin();
		if (Math.AbsFloat(origin[1] - world.GetSurfaceY(origin[0], origin[2])) > 3)
			return false;
		vector transform[4];
		target.GetWorldTransform(transform);
		vector forward = transform[2];
		forward[1] = 0;
		if (forward.LengthSq() < 0.01)
			return false;
		forward.Normalize();
		for (float distance = 3; distance <= 5; distance++)
		{
			vector desired = origin - forward * distance;
			vector floor;
			if (!SCR_EmptyPositionHelper.TryFindNearbyFloorPositionForEntity(world, character, desired, floor)
				|| vector.DistanceSqXZ(floor, desired) > 0.0625
				|| !SCR_Global.IsPositionWithinTerrainBounds(floor)
				|| Math.AbsFloat(floor[1] - world.GetSurfaceY(floor[0], floor[2])) > 1
				|| floor[1] < world.GetOceanHeight(floor[0], floor[2]))
				continue;
			destination = floor;
			return true;
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected ChimeraCharacter ResolveTarget(int playerId)
	{
		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		SCR_AIGroup group;
		if (groups)
			group = groups.GetPlayerGroup(playerId);
		ChimeraCharacter target;
		if (group && group.GetPlayerIDs().Contains(playerId))
		{
			int leader = group.GetLeaderID();
			if (leader != playerId && group.GetPlayerIDs().Contains(leader))
				target = LivingCharacter(leader);
			if (!target)
			{
				foreach (int member : group.GetPlayerIDs())
				{
					if (member == playerId)
						continue;
					target = LivingCharacter(member);
					if (target)
						break;
				}
			}
		}
		return target;
	}

	//------------------------------------------------------------------------------------------------
	bool ValidateVehicleMove(GC_LobbyPlayerController controller, IEntity character, IEntity target, IEntity vehicle)
	{
		if (!IsActive() || !WindowOpen() || !controller || !character || !target || !vehicle)
			return false;
		int playerId = controller.GetPlayerId();
		GC_JipPlayer record = m_mPlayers.Get(SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId));
		return record && record.m_bEligible && record.m_bOffered && !record.m_bConsumed
			&& GetGame().GetPlayerManager().GetPlayerController(playerId) == controller
			&& LivingCharacter(playerId) == character && record.m_Character == Replication.FindItemId(character)
			&& FactionAllowed(playerId) && ResolveTarget(playerId) == target
			&& character.GetRootParent() == character
			&& !CompartmentAccessComponent.GetVehicleIn(character)
			&& CompartmentAccessComponent.GetVehicleIn(target) == vehicle;
	}

	//------------------------------------------------------------------------------------------------
	void SetMovePending(GC_LobbyPlayerController controller, RplId character)
	{
		if (!IsActive())
			return;
		GC_JipPlayer record = m_mPlayers.Get(SCR_PlayerIdentityUtils.GetPlayerIdentityId(controller.GetPlayerId()));
		if (record && record.m_Character == character)
			record.m_bPending = true;
	}

	//------------------------------------------------------------------------------------------------
	//! Failure means no movement was submitted. Unknown outcomes remain pending.
	void CompleteMove(GC_LobbyPlayerController controller, RplId character, bool success)
	{
		if (!IsActive())
			return;
		GC_JipPlayer record = m_mPlayers.Get(SCR_PlayerIdentityUtils.GetPlayerIdentityId(controller.GetPlayerId()));
		if (!record || record.m_Character != character)
			return;
		record.m_bPending = false;
		if (success)
			record.m_bConsumed = true;
	}
}
