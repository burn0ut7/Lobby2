//! Transport-only fixture: substitutes AI for the unavailable second authenticated player.
//! Does not validate production group selection or remote streaming.
class GC_JipVehicleProbe : GC_JipVehicleMove
{
	protected override bool Validate()
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		return Replication.IsServer() && mode && mode.GetState() == SCR_EGameModeState.GAME
			&& m_Controller && m_Controller.GetControlledEntity() == m_Character
			&& m_Target && m_Vehicle && !CompartmentAccessComponent.GetVehicleIn(m_Character)
			&& CompartmentAccessComponent.GetVehicleIn(m_Target) == m_Vehicle;
	}
}

modded class GC_LobbyPlayerController
{
	string GC_TestJipBoard(IEntity target, IEntity vehicle)
	{
		if (m_GC_JipVehicleMove)
			m_GC_JipVehicleMove.Stop();
		m_GC_JipVehicleMove = new GC_JipVehicleProbe();
		string result = m_GC_JipVehicleMove.Start(this, ChimeraCharacter.Cast(GetControlledEntity()), target, vehicle, ++m_iGC_JipVehicleSequence);
		if (!result.IsEmpty())
			GC_SendJipMessage(result);
		return result;
	}
}

modded class GC_LobbyManager
{
	protected IEntity m_GC_JipTestVehicle;
	protected ref array<ChimeraCharacter> m_aGC_JipTestOccupants = {};
	protected ref array<BaseCompartmentSlot> m_aGC_JipTestSeats = {};

	string GC_TestJipVehicle(string action)
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!mode || mode.GetState() != SCR_EGameModeState.GAME || !controller
			|| !Replication.IsServer() || !SCR_Global.IsAdmin(controller.GetPlayerId()))
			return "UNAVAILABLE local authenticated admin in GAME";
		ChimeraCharacter character = ChimeraCharacter.Cast(controller.GetControlledEntity());
		if (!character)
			return "UNAVAILABLE character";
		if (action == "setup")
		{
			if (m_GC_JipTestVehicle)
				return "UNAVAILABLE fixture already exists";
			EntitySpawnParams params = new EntitySpawnParams();
			params.TransformMode = ETransformMode.WORLD;
			params.Transform[3] = character.GetOrigin() + "10 0 0";
			m_GC_JipTestVehicle = GetGame().SpawnEntityPrefab(Resource.Load("Prefabs/Vehicles/Wheeled/M998/M1025_armed_M2HB.et"), GetGame().GetWorld(), params);
			if (!m_GC_JipTestVehicle)
				return "FAIL vehicle spawn";
			for (int i = 0; i < 10; i++)
			{
				BaseCompartmentSlot seat = GC_JipVehicleMove.FindSeat(m_GC_JipTestVehicle, character);
				if (!seat)
					break;
				params.Transform[3] = character.GetOrigin() + "0 0 5";
				ChimeraCharacter occupant = ChimeraCharacter.Cast(GetGame().SpawnEntityPrefab(Resource.Load("{26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et"), GetGame().GetWorld(), params));
				if (!occupant)
					return "FAIL occupant spawn";
				AIControlComponent.Cast(occupant.FindComponent(AIControlComponent)).DeactivateAI();
				m_aGC_JipTestOccupants.Insert(occupant);
				seat.SetReserved(occupant);
				m_aGC_JipTestSeats.Insert(seat);
				if (!occupant.GetCompartmentAccessComponent().GetInVehicle(seat.GetOwner(), seat, true, -1, ECloseDoorAfterActions.INVALID, false))
					return "FAIL native occupant entry";
			}
		}
		else if (action == "free")
		{
			if (m_aGC_JipTestOccupants.Count() < 2)
				return "UNAVAILABLE second occupant";
			ChimeraCharacter departing = m_aGC_JipTestOccupants[m_aGC_JipTestOccupants.Count() - 1];
			m_aGC_JipTestSeats[m_aGC_JipTestSeats.Count() - 1].SetReserved(null);
			vector transform[4];
			departing.GetWorldTransform(transform);
			transform[3] = character.GetOrigin() + "0 0 -5";
			departing.GetCompartmentAccessComponent().GetOutVehicle_NoDoor(transform, false, false, true);
		}
		else if (action == "board")
		{
			if (!m_GC_JipTestVehicle || m_aGC_JipTestOccupants.IsEmpty())
				return "UNAVAILABLE vehicle occupants";
			string result = controller.GC_TestJipBoard(m_aGC_JipTestOccupants[0], m_GC_JipTestVehicle);
			return "transportResult=" + result + " " + mode.GetJip().GC_TestSnapshot(controller.GetPlayerId());
		}
		else if (action == "editor-open" || action == "editor-close")
		{
			SCR_EditorManagerEntity editor = SCR_EditorManagerEntity.GetInstance();
			if (!editor || editor.IsLimited())
				return "UNAVAILABLE unlimited editor";
			if (action == "editor-open")
			{
				editor.SetCurrentMode(false);
				editor.Open();
			}
			else
				editor.Close();
		}
		else if (action == "stream-owner")
		{
			RplComponent node = RplComponent.Cast(m_GC_JipTestVehicle.FindComponent(RplComponent));
			SCR_EditorManagerEntity editor = SCR_EditorManagerEntity.GetInstance();
			return string.Format("editorOpen=%1 keepsVehicle=%2", editor && editor.IsOpened(), GC_JipVehicleMove.EditorKeepsNode(controller.GetPlayerId(), node));
		}
		else if (action != "snapshot")
			return "UNAVAILABLE action";
		int occupied;
		foreach (ChimeraCharacter actor : m_aGC_JipTestOccupants)
		{
			if (actor && CompartmentAccessComponent.GetVehicleIn(actor) == m_GC_JipTestVehicle)
				occupied++;
		}
		bool freeSeat = m_GC_JipTestVehicle && GC_JipVehicleMove.FindSeat(m_GC_JipTestVehicle, character);
		bool boarded = m_GC_JipTestVehicle && CompartmentAccessComponent.GetVehicleIn(character) == m_GC_JipTestVehicle;
		return string.Format("occupants=%1 free=%2 boarded=%3 %4 message=%5", occupied, freeSeat, boarded,
			mode.GetJip().GC_TestSnapshot(controller.GetPlayerId()), controller.GC_TestJipMessage());
	}
}
