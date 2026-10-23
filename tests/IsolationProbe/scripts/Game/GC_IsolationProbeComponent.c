//! Optional project test harness shared by addon-free and addon-loaded runs.
//! No Lobby2 type dependencies and no automatic runtime actions.
#ifdef WORKBENCH
class GC_IsolationProbeComponentClass : ScriptComponentClass
{
}

class GC_IsolationProbeComponent : ScriptComponent
{
	//------------------------------------------------------------------------------------------------
	string PossessCharacter()
	{
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		IEntity character = GetGame().GetWorld().FindEntityByName("IsolationCharacter");
		if (!Replication.IsServer() || !controller || !character)
			return "rejected: missing authority, controller or character";
		controller.SetInitialMainEntity(character);
		return string.Format("possessed=%1", controller.GetControlledEntity() == character);
	}

	//------------------------------------------------------------------------------------------------
	string Snapshot()
	{
		BaseGameMode mode = GetGame().GetGameMode();
		PlayerController controller = GetGame().GetPlayerController();
		string modeType = "none";
		string controllerType = "none";
		if (mode)
			modeType = mode.Type().ToString();
		if (controller)
			controllerType = controller.Type().ToString();
		SCR_AIGroup group = SCR_AIGroup.Cast(GetGame().GetWorld().FindEntityByName("IsolationGroup"));
		int agents = -1;
		if (group)
		{
			array<AIAgent> members = {};
			group.GetAgents(members);
			agents = members.Count();
		}
		string result = string.Format("mode=%1 controller=%2 groupAgents=%3", modeType, controllerType, agents);
		SCR_VONController voice;
		if (controller)
			voice = SCR_VONController.Cast(controller.FindComponent(SCR_VONController));
		if (voice)
		{
			array<ref SCR_VONEntry> entries = {};
			result += string.Format(" voiceDisabled=%1 voiceEntries=%2", voice.IsVONDisabled(), voice.GetVONEntries(entries));
		}
		return result;
	}

	//------------------------------------------------------------------------------------------------
	string SpawnGroup()
	{
		SCR_AIGroup group = SCR_AIGroup.Cast(GetGame().GetWorld().FindEntityByName("IsolationGroup"));
		if (!group)
			return "rejected: missing group";
		array<AIAgent> members = {};
		group.GetAgents(members);
		if (!members.IsEmpty())
			return "rejected: group already populated";
		group.SpawnUnits();
		return "dispatched: stock group SpawnUnits";
	}

	//------------------------------------------------------------------------------------------------
	string VoiceCallbacks()
	{
		IEntity character = GetGame().GetWorld().FindEntityByName("IsolationCharacter");
		SCR_VoNComponent voice;
		if (character)
			voice = SCR_VoNComponent.Cast(character.FindComponent(SCR_VoNComponent));
		if (!voice || !GetGame().GetPlayerController())
			return "rejected: missing character voice or local controller";
		return voice.GC_TestStockCallbacks();
	}
}

modded class SCR_VoNComponent
{
	protected int m_iGC_TestReceives;
	protected int m_iGC_TestCaptures;

	//------------------------------------------------------------------------------------------------
	//! Calls the real script callback chain, not microphone/network transport.
	string GC_TestStockCallbacks()
	{
		m_iGC_TestReceives = 0;
		m_iGC_TestCaptures = 0;
		m_OnReceivedVON.Insert(GC_TestReceived);
		m_OnCaptureVON.Insert(GC_TestCaptured);
		OnReceive(GetGame().GetPlayerController().GetPlayerId(), false, null, 0, 1);
		OnCapture(null);
		m_OnReceivedVON.Remove(GC_TestReceived);
		m_OnCaptureVON.Remove(GC_TestCaptured);
		return string.Format("received=%1 captured=%2 transmitting=%3 radioTransmitting=%4", m_iGC_TestReceives, m_iGC_TestCaptures, IsTransmiting(), IsTransmitingRadio());
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TestReceived(int playerId, BaseTransceiver receiver, int frequency, float quality)
	{
		m_iGC_TestReceives++;
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TestCaptured(BaseTransceiver transmitter)
	{
		m_iGC_TestCaptures++;
	}
}
#endif
