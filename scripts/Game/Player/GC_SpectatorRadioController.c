//! Separate receivers: spectator chat/transmit slots are never retuned or replaced.
modded class GC_LobbyPlayerController
{
	protected int m_iGC_RadioTarget;
	protected bool m_bGC_RadioAll;
	protected ref array<RplId> m_aGC_RadioSelected = {};
	protected int m_iGC_ServerRadioTarget;
	protected string m_sGC_ServerRadioIdentity;
	protected IEntity m_GC_ServerRadioCharacter;
	protected bool m_bGC_ServerRadioAll;
	protected ref array<RplId> m_aGC_ServerRadioSelected = {};
	protected ref array<IEntity> m_aGC_RadioReceivers = {};

	bool GC_IsListeningToRadio(int target, RplId radioId, bool all = false)
	{
		return m_iGC_RadioTarget == target && ((all && m_bGC_RadioAll) || (!all && (m_bGC_RadioAll || m_aGC_RadioSelected.Contains(radioId))));
	}

	bool GC_IsListeningToRadios()
	{
		return m_iGC_RadioTarget > 0;
	}

	//! operation: 0 stop, 1 all radios, 2 toggle one physical radio (all its channels).
	void GC_RequestRadioListening(int target, string identity, int operation, RplId radioId)
	{
		if (GC_LobbyPlayerController.Cast(this).GC_CanJumpSpectatorCamera())
			Rpc(GC_RpcAsk_RadioListening, target, identity, operation, radioId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GC_RpcAsk_RadioListening(int target, string identity, int operation, RplId radioId)
	{
		if (!GC_CanReceivePlayerRadios())
			return;
		if (operation == 0)
		{
			GC_StopRadioListening();
			GC_SendRadioListeningState();
			return;
		}
		PlayerManager players = GetGame().GetPlayerManager();
		if (target <= 0 || !players.IsPlayerConnected(target) || identity.IsEmpty() || SCR_PlayerIdentityUtils.GetPlayerIdentityId(target) != identity)
			return;
		IEntity character = players.GetPlayerControlledEntity(target);
		array<BaseRadioComponent> radios;
		GC_SpectatorRadio.GetRadios(character, radios);
		if (radios.IsEmpty())
		{
			GC_SendRadioListeningState("No player radios available.");
			return;
		}
		array<RplId> available = {};
		int count;
		foreach (BaseRadioComponent radio : radios)
		{
			available.Insert(GC_SpectatorRadio.GetId(radio));
			count += radio.TransceiversCount();
		}
		if ((operation != 1 && operation != 2) || (operation == 2 && (!radioId.IsValid() || !available.Contains(radioId))))
			return;
		if (count > GC_SpectatorRadio.MAX_RECEIVERS)
		{
			GC_SendRadioListeningState("Too many radio channels to monitor.");
			return;
		}
		if (m_iGC_ServerRadioTarget != target || m_GC_ServerRadioCharacter != character)
			GC_StopRadioListening();
		m_iGC_ServerRadioTarget = target;
		m_sGC_ServerRadioIdentity = identity;
		m_GC_ServerRadioCharacter = character;
		if (operation == 1)
		{
			m_bGC_ServerRadioAll = true;
			m_aGC_ServerRadioSelected.Copy(available);
		}
		else
		{
			m_bGC_ServerRadioAll = false;
			int index = m_aGC_ServerRadioSelected.Find(radioId);
			if (index >= 0)
				m_aGC_ServerRadioSelected.Remove(index);
			else
				m_aGC_ServerRadioSelected.Insert(radioId);
		}
		GetGame().GetCallqueue().Remove(GC_UpdateRadioListening);
		GC_UpdateRadioListening();
		if (m_iGC_ServerRadioTarget > 0)
		{
			GC_SendRadioListeningState();
			GetGame().GetCallqueue().CallLater(GC_UpdateRadioListening, 500, true);
		}
	}

	protected bool GC_CanReceivePlayerRadios()
	{
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		IEntity body = GetControlledEntity();
		return Replication.IsServer() && manager && manager.GetLobbyPhase() == SCR_EGameModeState.GAME
			&& GetLobbyPlayerState() == GC_ELobbyPlayerState.SPECTATOR && body && body.FindComponent(GC_LobbySpectatorBodyComponent);
	}

	protected void GC_UpdateRadioListening()
	{
		PlayerManager players = GetGame().GetPlayerManager();
		if (!GC_CanReceivePlayerRadios() || !players.IsPlayerConnected(m_iGC_ServerRadioTarget)
			|| SCR_PlayerIdentityUtils.GetPlayerIdentityId(m_iGC_ServerRadioTarget) != m_sGC_ServerRadioIdentity
			|| players.GetPlayerControlledEntity(m_iGC_ServerRadioTarget) != m_GC_ServerRadioCharacter)
		{
			GC_StopRadioListening();
			GC_SendRadioListeningState();
			return;
		}
		array<BaseRadioComponent> radios;
		GC_SpectatorRadio.GetRadios(m_GC_ServerRadioCharacter, radios);
		array<BaseTransceiver> channels = {};
		array<RplId> selected = {};
		foreach (BaseRadioComponent radio : radios)
		{
			RplId id = GC_SpectatorRadio.GetId(radio);
			if (!m_bGC_ServerRadioAll && !m_aGC_ServerRadioSelected.Contains(id))
				continue;
			selected.Insert(id);
			for (int i; i < radio.TransceiversCount(); i++)
				channels.Insert(radio.GetTransceiver(i));
		}
		if (channels.IsEmpty() || channels.Count() > GC_SpectatorRadio.MAX_RECEIVERS)
		{
			GC_StopRadioListening();
			GC_SendRadioListeningState();
			return;
		}
		bool changed = selected.Count() != m_aGC_ServerRadioSelected.Count();
		foreach (RplId id : selected)
			changed = changed || !m_aGC_ServerRadioSelected.Contains(id);
		m_aGC_ServerRadioSelected.Copy(selected);
		IEntity body = GetControlledEntity();
		InventoryStorageManagerComponent inventory = InventoryStorageManagerComponent.Cast(body.FindComponent(InventoryStorageManagerComponent));
		GC_SpectatorRadioStorageComponent storage = GC_SpectatorRadioStorageComponent.Cast(body.FindComponent(GC_SpectatorRadioStorageComponent));
		if (!inventory || !storage)
		{
			GC_StopRadioListening();
			GC_SendRadioListeningState("Radio listening is unavailable.");
			return;
		}
		if (m_aGC_RadioReceivers.IsEmpty())
			inventory.AddStorage(storage);
		while (m_aGC_RadioReceivers.Count() < channels.Count())
		{
			EntitySpawnParams parameters = new EntitySpawnParams();
			parameters.TransformMode = ETransformMode.WORLD;
			parameters.Transform[3] = body.GetOrigin();
			IEntity receiver = GetGame().SpawnEntityPrefab(Resource.Load(GC_SpectatorRadio.RECEIVER), GetGame().GetWorld(), parameters);
			if (!receiver || !inventory.TryInsertItemInStorage(receiver, storage))
			{
				if (receiver)
					SCR_EntityHelper.DeleteEntityAndChildren(receiver);
				GC_StopRadioListening();
				GC_SendRadioListeningState("Could not create listening receivers.");
				return;
			}
			m_aGC_RadioReceivers.Insert(receiver);
		}
		for (int index = m_aGC_RadioReceivers.Count() - 1; index >= channels.Count(); index--)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(m_aGC_RadioReceivers[index]);
			m_aGC_RadioReceivers.Remove(index);
		}
		array<string> heard = {};
		for (int index; index < channels.Count(); index++)
		{
			BaseTransceiver source = channels[index];
			BaseRadioComponent sourceRadio = source.GetRadio();
			// Keep native reception range at the watched radio, retaining spectator inventory ownership.
			vector position = sourceRadio.GetOwner().GetOrigin();
			if (vector.DistanceSq(m_aGC_RadioReceivers[index].GetOrigin(), position) > 0.01)
				m_aGC_RadioReceivers[index].SetOrigin(position);
			BaseRadioComponent receiverRadio = BaseRadioComponent.Cast(m_aGC_RadioReceivers[index].FindComponent(BaseRadioComponent));
			BaseTransceiver receiverChannel = receiverRadio.GetTransceiver(0);
			string key = sourceRadio.GetEncryptionKey();
			int frequency = source.GetFrequency();
			string signature = key.Length().ToString() + ":" + key + ":" + frequency.ToString();
			bool powered = sourceRadio.IsPowered() && !source.IsMuted() && !m_bLobbyVoiceDeafened && !heard.Contains(signature);
			if (powered)
				heard.Insert(signature);
			if (receiverRadio.GetEncryptionKey() != key)
				receiverRadio.SetEncryptionKey(key);
			if (receiverChannel.GetFrequency() != frequency)
				receiverChannel.SetFrequency(frequency);
			if (receiverRadio.IsPowered() != powered)
				receiverRadio.SetPower(powered);
		}
		if (changed)
			GC_SendRadioListeningState();
	}

	void GC_StopRadioListening()
	{
		GetGame().GetCallqueue().Remove(GC_UpdateRadioListening);
		foreach (IEntity receiver : m_aGC_RadioReceivers)
		{
			if (receiver)
				SCR_EntityHelper.DeleteEntityAndChildren(receiver);
		}
		m_aGC_RadioReceivers.Clear();
		m_iGC_ServerRadioTarget = 0;
		m_sGC_ServerRadioIdentity = "";
		m_GC_ServerRadioCharacter = null;
		m_bGC_ServerRadioAll = false;
		m_aGC_ServerRadioSelected.Clear();
		m_iGC_RadioTarget = 0;
		m_bGC_RadioAll = false;
		m_aGC_RadioSelected.Clear();
	}

	protected void GC_SendRadioListeningState(string error = "")
	{
		Rpc(GC_RpcDo_RadioListening, m_iGC_ServerRadioTarget, m_bGC_ServerRadioAll, m_aGC_ServerRadioSelected, error);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GC_RpcDo_RadioListening(int target, bool all, array<RplId> selected, string error)
	{
		if (target > 0 && !GC_LobbyPlayerController.Cast(this).GC_CanJumpSpectatorCamera())
			return;
		m_iGC_RadioTarget = target;
		m_bGC_RadioAll = all;
		m_aGC_RadioSelected.Copy(selected);
		if (!error.IsEmpty())
			SCR_HintManagerComponent.ShowCustomHint(error, "Listen only", 4);
	}
}
