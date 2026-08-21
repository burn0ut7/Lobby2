modded class SCR_VONController
{
	protected static const int PS_RADIO_RECEIVER_STABILIZATION_DELAY_MS = 3000;
	protected static const int PS_RADIO_RECEIVER_POWER_OFF_MS = 150;

	protected ref array<BaseRadioComponent> m_aPSReinitializedRadios = {};

	//------------------------------------------------------------------------------------------------
	override void AddEntry(SCR_VONEntry entry)
	{
		super.AddEntry(entry);

		SCR_VONEntryRadio radioEntry = SCR_VONEntryRadio.Cast(entry);
		if (!radioEntry)
			return;

		BaseTransceiver transceiver = radioEntry.GetTransceiver();
		if (!transceiver)
			return;

		BaseRadioComponent radio = transceiver.GetRadio();
		if (!radio || radio.IsEditorRadio() || !radio.IsPowered())
			return;

		if (m_aPSReinitializedRadios.Contains(radio))
			return;

		m_aPSReinitializedRadios.Insert(radio);
		GetGame().GetCallqueue().CallLater(
			PS_ReinitializeRadioReceiver,
			PS_RADIO_RECEIVER_STABILIZATION_DELAY_MS,
			false,
			radio);
	}

	//------------------------------------------------------------------------------------------------
	protected void PS_ReinitializeRadioReceiver(BaseRadioComponent radio)
	{
		// Do not override a deliberate power-off made while waiting.
		if (!radio || !radio.IsPowered())
			return;

		radio.SetPower(false);
		PS_SetRadioEntriesUsable(radio, false);

		// Keep the off/on operations on separate call-queue turns so the native
		// radio system has time to unregister the receiver.
		GetGame().GetCallqueue().CallLater(
			PS_RestoreRadioReceiver,
			PS_RADIO_RECEIVER_POWER_OFF_MS,
			false,
			radio);
	}

	//------------------------------------------------------------------------------------------------
	protected void PS_RestoreRadioReceiver(BaseRadioComponent radio)
	{
		if (!radio)
			return;

		radio.SetPower(true);
		PS_SetRadioEntriesUsable(radio, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void PS_SetRadioEntriesUsable(BaseRadioComponent radio, bool usable)
	{
		array<ref SCR_VONEntry> entries = {};
		GetVONEntries(entries);

		foreach (SCR_VONEntry candidate : entries)
		{
			SCR_VONEntryRadio radioEntry = SCR_VONEntryRadio.Cast(candidate);
			if (!radioEntry)
				continue;

			BaseTransceiver transceiver = radioEntry.GetTransceiver();
			if (transceiver && transceiver.GetRadio() == radio)
				candidate.SetUsable(usable);
		}
	}
}