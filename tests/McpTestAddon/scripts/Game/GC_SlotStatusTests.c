#ifdef WORKBENCH
modded class GC_LobbyManager
{
	protected ref array<ref GC_Slot> m_aGCStatusGallerySlots = {};

	//------------------------------------------------------------------------------------------------
	//! Presentation-only copies: never changes the registered slots or player claims.
	//! Reopen the menu or pass restore=true to restore its authoritative bindings.
	string GC_TestSlotStatusGallery(bool restore)
	{
		array<Widget> widgets = {};
		GC_TestCollect(GC_TestMenuRoot(), widgets);
		int row;
		foreach (Widget widget : widgets)
		{
			GC_SlotButtonComponent button = GC_SlotButtonComponent.Cast(widget.FindHandler(GC_SlotButtonComponent));
			if (!button || !button.GetSlot())
				continue;
			GC_Slot source = GetSlot(button.GetSlot().GetID());
			if (!source)
				continue;
			if (restore)
			{
				button.SetSlot(source);
				continue;
			}
			if (row >= 8)
				continue;
			GC_Slot sample = new GC_Slot();
			sample.CopyFrom(source);
			sample.ReleaseClaim();
			sample.SetState(GC_SlotState.FREE);
			sample.SetAdminLocked(false);
			if (row == 0)
				sample.SetAdminLocked(true);
			else if (row == 1)
				sample.MarkKIA();
			else if (row == 2 || row == 3)
			{
				sample.ReserveIdentity("00000000-0000-0000-0000-000000000001", "Reserved Player");
				sample.SetAdminLocked(row == 3);
			}
			else if (row == 4)
			{
				sample.SetPlayer(GetGame().GetPlayerController().GetPlayerId());
				sample.MarkKIA();
				sample.SetAdminLocked(true);
			}
			else if (row == 5)
			{
				sample.SetPlayer(GetGame().GetPlayerController().GetPlayerId());
				sample.MarkDisconnected();
			}
			else if (row == 6)
			{
				sample.SetPlayer(GetGame().GetPlayerController().GetPlayerId());
				sample.SetAdminLocked(true);
			}
			m_aGCStatusGallerySlots.Insert(sample);
			button.SetSlot(sample);
			row++;
		}
		if (restore)
			m_aGCStatusGallerySlots.Clear();
		return string.Format("presentationOnly=1 restored=%1 rows=%2", restore, row);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestSlotStatusSnapshot(bool refreshNames = false)
	{
		array<Widget> widgets = {};
		GC_TestCollect(GC_TestMenuRoot(), widgets);
		string result;
		foreach (Widget widget : widgets)
		{
			GC_SlotButtonComponent button = GC_SlotButtonComponent.Cast(widget.FindHandler(GC_SlotButtonComponent));
			if (!button || !button.GetSlot())
				continue;
			GC_Slot slot = button.GetSlot();
			if (refreshNames)
				button.RefreshAdminNamePresentation();
			TextWidget label = TextWidget.Cast(widget.FindAnyWidget("StatusText"));
			result += string.Format("slot=%1 state=%2 adminLock=%3 icon=%4 nameVisible=%5 text='%6' color=%7\n",
				slot.GetID(), slot.GetState(), slot.IsAdminLocked(), GC_TestVisible(widget.FindAnyWidget("StatusIcon")),
				GC_TestVisible(label), label.GetText(), label.GetColor().PackToInt());
		}
		return result;
	}
}
#endif
