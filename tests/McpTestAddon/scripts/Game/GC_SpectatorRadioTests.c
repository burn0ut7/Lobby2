class GC_TestReceiverMessage : ScriptedRadioMessage
{
	static int s_iReceived;
	override void OnDelivery(BaseTransceiver receiver, int freq, float quality)
	{
		if (receiver.GetRadio().GetOwner().GetPrefabData().GetPrefabName() == GC_SpectatorRadio.RECEIVER)
			s_iReceived++;
	}
}

modded class GC_LobbyPlayerController
{
	string GC_TestVoiceBody(string action, vector position)
	{
		if (action == "move" && m_SpectatorCamera)
		{
			m_SpectatorCamera.SetFreeMode();
			m_SpectatorCamera.SetOrigin(position);
		}
		if (action == "request" && GetControlledEntity())
			Rpc(GC_RpcAskSpectatorBodyPosition, Replication.FindItemId(GetControlledEntity()), position);
		return GC_GetSpectatorVoiceDiagnostics();
	}

	string GC_TestRadioListen(int target, int operation, int radioIndex)
	{
		array<BaseRadioComponent> radios;
		GC_SpectatorRadio.GetRadios(GetGame().GetPlayerManager().GetPlayerControlledEntity(target), radios);
		RplId id = RplId.Invalid();
		if (radioIndex >= 0 && radioIndex < radios.Count())
			id = GC_SpectatorRadio.GetId(radios[radioIndex]);
		if (operation >= 0)
		{
			if (operation >= 10)
				Rpc(GC_RpcAsk_RadioListening, target, GC_ContextMenuContext.ForPlayer(target).GetPlayerIdentity(), operation - 10, id);
			else
				GC_RequestRadioListening(target, GC_ContextMenuContext.ForPlayer(target).GetPlayerIdentity(), operation, id);
		}
		string report = string.Format("target=%1 all=%2 selected=%3 receivers=%4 sourceRadios=%5", m_iGC_RadioTarget, m_bGC_RadioAll, m_aGC_RadioSelected.Count(), m_aGC_RadioReceivers.Count(), radios.Count());
		IEntity body = GetControlledEntity();
		report += string.Format(" canListen=%1 state=%2 serverTarget=%3 storage=%4", GC_CanJumpSpectatorCamera(), GetLobbyPlayerState(), m_iGC_ServerRadioTarget, body.FindComponent(GC_SpectatorRadioStorageComponent) != null);
		BaseRadioComponent lobby = GC_LobbyRadio.FindRadio(body);
		array<IEntity> items = {};
		InventoryStorageManagerComponent.Cast(body.FindComponent(InventoryStorageManagerComponent)).GetItems(items);
		int localReceivers;
		foreach (IEntity item : items)
		{
			if (item.GetPrefabData().GetPrefabName() == GC_SpectatorRadio.RECEIVER)
				localReceivers++;
		}
		report += " inventoryReceivers=" + localReceivers.ToString();
		if (lobby)
			report += string.Format(" lobbyKey=%1 chat=%2 faction=%3 powered=%4", lobby.GetEncryptionKey(), lobby.GetTransceiver(1).GetFrequency(), lobby.GetTransceiver(2).GetFrequency(), lobby.IsPowered());
		foreach (IEntity receiver : m_aGC_RadioReceivers)
		{
			BaseRadioComponent radio = BaseRadioComponent.Cast(receiver.FindComponent(BaseRadioComponent));
			InventoryStorageManagerComponent inventory = InventoryStorageManagerComponent.Cast(body.FindComponent(InventoryStorageManagerComponent));
			report += string.Format(" | receiver key=%1 freq=%2 power=%3 stored=%4 gadget=%5", radio.GetEncryptionKey(), radio.GetTransceiver(0).GetFrequency(), radio.IsPowered(), inventory.Contains(receiver), receiver.FindComponent(SCR_RadioComponent) != null);
		}
		return report;
	}

	string GC_TestRadioCommand(string message)
	{
		array<string> values = {};
		message.Split("|", values, false);
		if (values.Count() != 3)
			return "expected target|operation|index";
		return GC_TestRadioListen(values[0].ToInt(), values[1].ToInt(), values[2].ToInt());
	}

	string GC_TestRadioPosition(vector position, bool move)
	{
		string report;
		foreach (IEntity receiver : m_aGC_RadioReceivers)
		{
			if (move)
				receiver.SetOrigin(position);
			report += receiver.GetOrigin().ToString() + " ";
		}
		return report;
	}
}

#ifdef WORKBENCH
modded class GC_LobbyManager
{
	string GC_TestVoiceBody(int playerId, string action, vector position)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!controller || !controller.GetControlledEntity())
			return "NO BODY";
		return "serverBody=" + controller.GetControlledEntity().GetOrigin().ToString() + " " + controller.GC_TestVoiceBody(action, position);
	}

	string GC_TestMarkerContext(int target, bool pressed)
	{
		if (pressed)
		{
			Widget row = GC_TestFindFocusMarker(GetGame().GetWorkspace(), target);
			if (!row)
				return "NO MARKER";
			Widget button = row.FindAnyWidget("LobbySpectatorMarkerButton");
			if (!button)
				return "NO BUTTON";
			float x, y, width, height;
			button.GetScreenPos(x, y);
			button.GetScreenSize(width, height);
			GetGame().GetInputManager().GetMouseDeviceHandler().SetCursorPosition(x + width * 0.5, y + height * 0.5);
			WidgetManager.ReportMouse(x + width * 0.5, y + height * 0.5, GetGame().GetWorkspace());
		}
		GetGame().GetInputManager().SetActionValue("GC_SpectatorContextClick", pressed);
		if (pressed)
			GetGame().GetInputManager().SetActionValue("GC_SpectatorContextClick", 0);
		return "input dispatched; inspect menu after release";
	}

	protected ref GC_TestReceiverMessage m_GC_TestRadioMessage;

	string GC_TestRadioAdd(int target)
	{
		IEntity body = GetGame().GetPlayerManager().GetPlayerControlledEntity(target);
		InventoryStorageManagerComponent inventory = InventoryStorageManagerComponent.Cast(body.FindComponent(InventoryStorageManagerComponent));
		EntitySpawnParams parameters = new EntitySpawnParams();
		parameters.TransformMode = ETransformMode.WORLD;
		parameters.Transform[3] = body.GetOrigin();
		IEntity item = GetGame().SpawnEntityPrefab(Resource.Load("{73950FBA2D7DB5C5}Prefabs/Items/Equipment/Radios/Radio_ANPRC68.et"), GetGame().GetWorld(), parameters);
		array<BaseInventoryStorageComponent> storages = {};
		inventory.GetStorages(storages);
		bool inserted;
		foreach (BaseInventoryStorageComponent storage : storages)
		{
			if (storage.CanStoreItem(item, -1))
			{
				inserted = inventory.TryInsertItemInStorage(item, storage);
				if (inserted)
					break;
			}
		}
		if (!inserted)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(item);
			return "insertion rejected; storages=" + storages.Count().ToString();
		}
		BaseRadioComponent radio = BaseRadioComponent.Cast(item.FindComponent(BaseRadioComponent));
		radio.SetEncryptionKey("second-key");
		radio.GetTransceiver(0).SetFrequency(51000);
		radio.SetPower(true);
		return "insertion dispatched; inspect source and receivers";
	}

	string GC_TestRadioViewer(int viewer, int target)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(viewer));
		if (!controller)
			return "viewer disconnected";
		return controller.GC_TestRadioListen(target, -1, 0);
	}

	string GC_TestRadioRange(int viewer, int target, bool follow)
	{
		array<BaseRadioComponent> radios;
		GC_SpectatorRadio.GetRadios(GetGame().GetPlayerManager().GetPlayerControlledEntity(target), radios);
		if (radios.IsEmpty())
			return "no source";
		radios[0].GetTransceiver(0).SetRange(1);
		vector position = radios[0].GetOwner().GetOrigin();
		return "source=" + position.ToString() + " receivers=" + GC_LobbyPlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(viewer)).GC_TestRadioPosition(position, follow);
	}

	string GC_TestRadioDelivery(int target, string key, bool send)
	{
		if (send)
		{
			array<BaseRadioComponent> radios;
			GC_SpectatorRadio.GetRadios(GetGame().GetPlayerManager().GetPlayerControlledEntity(target), radios);
			if (radios.IsEmpty())
				return "no source";
			GC_TestReceiverMessage.s_iReceived = 0;
			m_GC_TestRadioMessage = new GC_TestReceiverMessage();
			m_GC_TestRadioMessage.SetEncryptionKey(key);
			radios[0].GetTransceiver(0).BeginTransmission(m_GC_TestRadioMessage);
		}
		return "deliveries=" + GC_TestReceiverMessage.s_iReceived.ToString();
	}

	string GC_TestRadioContext(int target)
	{
		return GC_ContextMenuComponent.Open(GC_ContextMenuContext.ForPlayer(target, GetGame().GetPlayerManager().GetPlayerControlledEntity(target)), GC_TestMenuRoot()).ToString();
	}

	string GC_TestRadioListen(int target, int operation, int radioIndex)
	{
		return GC_LobbyPlayerController.Cast(GetGame().GetPlayerController()).GC_TestRadioListen(target, operation, radioIndex);
	}

	string GC_TestRadioSource(int target, int radioIndex, int frequency, string key, bool power)
	{
		if (!Replication.IsServer())
			return "NOT AUTHORITY";
		array<BaseRadioComponent> radios;
		GC_SpectatorRadio.GetRadios(GetGame().GetPlayerManager().GetPlayerControlledEntity(target), radios);
		if (radioIndex < 0 || radioIndex >= radios.Count())
			return "radios=" + radios.Count().ToString();
		BaseRadioComponent radio = radios[radioIndex];
		if (frequency >= 0)
		{
			radio.SetEncryptionKey(key);
			radio.GetTransceiver(0).SetFrequency(frequency);
			radio.SetPower(power);
		}
		return string.Format("radios=%1 channels=%2 key=%3 frequency=%4 power=%5", radios.Count(), radio.TransceiversCount(), radio.GetEncryptionKey(), radio.GetTransceiver(0).GetFrequency(), radio.IsPowered());
	}
}
#endif
