//! One authority-owned seat reservation and bounded replication preload.
class GC_JipVehicleMove
{
	protected GC_LobbyPlayerController m_Controller;
	protected ChimeraCharacter m_Character;
	protected IEntity m_Target;
	protected IEntity m_Vehicle;
	protected BaseCompartmentSlot m_Seat;
	protected RplComponent m_VehicleReplication;
	protected RplComponent m_SeatReplication;
	protected RplIdentity m_Identity;
	protected RplId m_CharacterId;
	protected int m_iSequence;
	protected int m_iChecks;
	protected bool m_bActive;
	protected bool m_bSubmitted;
	protected bool m_bPinned;

	//------------------------------------------------------------------------------------------------
	//! Native occupants, accessibility, locks and reservations are the seat authority.
	static BaseCompartmentSlot FindSeat(IEntity vehicle, IEntity character)
	{
		BaseCompartmentManagerComponent manager = BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(BaseCompartmentManagerComponent));
		array<BaseCompartmentSlot> seats = {};
		if (manager)
			manager.GetCompartments(seats);
		array<ECompartmentType> preference = {ECompartmentType.CARGO, ECompartmentType.PILOT, ECompartmentType.TURRET};
		foreach (ECompartmentType type : preference)
		{
			foreach (BaseCompartmentSlot seat : seats)
			{
				if (seat.GetType() == type && !seat.IsOccupied() && !seat.IsReserved()
					&& seat.IsCompartmentAccessible() && !seat.IsGetInLockedFor(character))
					return seat;
			}
		}
		IEntity child = vehicle.GetChildren();
		while (child)
		{
			BaseCompartmentSlot childSeat = FindSeat(child, character);
			if (childSeat)
				return childSeat;
			child = child.GetSibling();
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected bool Validate()
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		return mode && mode.GetJip() && mode.GetJip().ValidateVehicleMove(m_Controller, m_Character, m_Target, m_Vehicle);
	}

	//------------------------------------------------------------------------------------------------
	string Start(GC_LobbyPlayerController controller, ChimeraCharacter character, IEntity target, IEntity vehicle, int sequence)
	{
		m_Controller = controller;
		m_Character = character;
		m_Target = target;
		m_Vehicle = vehicle;
		m_iSequence = sequence;
		if (!Replication.IsServer() || !Validate())
			return "#GC-Jip_Unavailable";
		m_Seat = FindSeat(vehicle, character);
		if (!m_Seat)
			return "#GC-Jip_VehicleFull";
		m_VehicleReplication = RplComponent.Cast(vehicle.FindComponent(RplComponent));
		m_SeatReplication = RplComponent.Cast(m_Seat.GetOwner().FindComponent(RplComponent));
		m_CharacterId = Replication.FindItemId(character);
		m_Identity = controller.GetRplIdentity();
		if (!m_VehicleReplication || !m_SeatReplication || !m_CharacterId.IsValid() || !m_Identity.IsValid())
			return "#GC-Jip_Retry";
		m_Seat.SetReserved(character);
		if (!m_Seat.IsReservedBy(character))
			return "#GC-Jip_VehicleFull";
		m_bActive = true;
		GC_LobbyGameMode.GetGameMode().GetJip().SetMovePending(controller, m_CharacterId);
		// Same per-connection stream-out suppression used by native GM/markers.
		// Preload only the destination nodes; never move the source just to stream them.
		if (m_Identity != RplIdentity.Local())
		{
			m_VehicleReplication.EnableStreamingConNode(m_Identity, false);
			if (m_SeatReplication != m_VehicleReplication)
				m_SeatReplication.EnableStreamingConNode(m_Identity, false);
			m_bPinned = true;
		}
		GetGame().GetCallqueue().CallLater(Poll, 250, true);
		controller.GC_PrepareJipSeat(sequence, m_CharacterId, m_SeatReplication.Id(), m_Seat.GetCompartmentSlotID());
		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	void Ready(int sequence)
	{
		if (!m_bActive || m_bSubmitted || sequence != m_iSequence)
			return;
		if (!Validate() || !m_Seat || !m_Seat.IsReservedBy(m_Character)
			|| m_Seat.IsOccupied() || !m_Seat.IsCompartmentAccessible() || m_Seat.IsGetInLockedFor(m_Character))
		{
			Finish(false, true, "#GC-Jip_Retry");
			return;
		}
		m_bSubmitted = true;
		m_iChecks = 0;
		m_Controller.GC_ExecuteJipSeat(sequence);
	}

	//------------------------------------------------------------------------------------------------
	//! A client cannot reopen eligibility after execution has been authorized.
	void Rejected(int sequence)
	{
		if (m_bActive && !m_bSubmitted && sequence == m_iSequence)
			Finish(false, true, "#GC-Jip_Retry");
	}

	//------------------------------------------------------------------------------------------------
	protected void Poll()
	{
		if (!m_bActive)
			return;
		if (m_bSubmitted && m_Character && m_Vehicle
			&& CompartmentAccessComponent.GetVehicleIn(m_Character) == m_Vehicle
			&& m_Seat && m_Seat.GetOccupant() == m_Character)
		{
			Finish(true, true, "#GC-Jip_Arrived");
			return;
		}
		if (!m_Controller || !m_Character || !m_Vehicle
			|| m_Controller.GetControlledEntity() != m_Character || !GC_LobbyGameMode.GetGameMode()
			|| ++m_iChecks >= 40)
		{
			if (m_bSubmitted)
				Finish(false, false, "#GC-Jip_Unverified");
			else
				Finish(false, true, "#GC-Jip_Retry");
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void Finish(bool success, bool resolved, string message)
	{
		Stop();
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!m_Controller || !mode || !mode.GetJip())
			return;
		if (resolved)
			mode.GetJip().CompleteMove(m_Controller, m_CharacterId, success);
		m_Controller.GC_SendJipMessage(message);
	}

	//------------------------------------------------------------------------------------------------
	//! Re-query the native GM owner at release; it may have opened during preload.
	//! Marker streaming uses separate marker entities, not these vehicle nodes.
	static bool EditorKeepsNode(int playerId, RplComponent node)
	{
		if (!node || !node.GetEntity())
			return false;
		SCR_EditorManagerCore managers = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		SCR_EditorManagerEntity editor;
		if (managers)
			editor = managers.GetEditorManager(playerId);
		if (!editor || !editor.IsOpened())
			return false;
		SCR_DynamicSimulationEditorComponent simulation = SCR_DynamicSimulationEditorComponent.Cast(editor.FindComponent(SCR_DynamicSimulationEditorComponent));
		if (!simulation && editor.GetCurrentModeEntity())
			simulation = SCR_DynamicSimulationEditorComponent.Cast(editor.GetCurrentModeEntity().FindComponent(SCR_DynamicSimulationEditorComponent));
		if (!simulation || !simulation.IsActive())
			return false;
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(node.GetEntity().FindComponent(SCR_EditableEntityComponent));
		SCR_EditableEntityCore core = SCR_EditableEntityCore.Cast(SCR_EditableEntityCore.GetInstance(SCR_EditableEntityCore));
		if (!editable || !core)
			return false;
		set<SCR_EditableEntityComponent> entities = new set<SCR_EditableEntityComponent>();
		core.GetAllEntities(entities);
		if (entities.Find(editable) < 0)
			return false;
		array<EEditableEntityType> skipped = {};
		SCR_EditorSettingsEntity settings = SCR_EditorSettingsEntity.GetInstance();
		if (settings)
			settings.GetSkipStreamingRules(skipped);
		return !skipped.Contains(editable.GetEntityType());
	}

	//------------------------------------------------------------------------------------------------
	void Stop()
	{
		m_bActive = false;
		GetGame().GetCallqueue().Remove(Poll);
		if (m_Seat && m_Character && m_Seat.IsReservedBy(m_Character))
			m_Seat.SetReserved(null);
		if (m_bPinned)
		{
			int playerId;
			if (m_Controller)
				playerId = m_Controller.GetPlayerId();
			if (m_VehicleReplication && !EditorKeepsNode(playerId, m_VehicleReplication))
				m_VehicleReplication.EnableStreamingConNode(m_Identity, true);
			if (m_SeatReplication && m_SeatReplication != m_VehicleReplication && !EditorKeepsNode(playerId, m_SeatReplication))
				m_SeatReplication.EnableStreamingConNode(m_Identity, true);
			m_bPinned = false;
		}
		if (m_Controller)
			m_Controller.GC_CancelJipSeat(m_iSequence);
	}
}
