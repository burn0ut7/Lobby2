//! Native modal controls; the owning controller serializes separately authorized insertions.
class GC_GearQuantityDialog : SCR_EditboxDialogUi
{
	protected ResourceName m_sPrefab;
	protected string m_sItemName;
	protected int m_iRevision;
	protected bool m_bCrate;
	protected bool m_bSynchronizing;
	protected SCR_SliderComponent m_Slider;

	//------------------------------------------------------------------------------------------------
	void Configure(GC_ItemCatalogueEntry entry)
	{
		m_sPrefab = entry.m_sPrefab;
		m_sItemName = entry.m_sName;
		m_iRevision = GC_GearSession.GetRevision();
		m_bCrate = GC_GearSession.s_bCrate;
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnMenuOpen(SCR_ConfigurableDialogUiPreset preset)
	{
		super.OnMenuOpen(preset);
		SetTitle(string.Format(WidgetManager.Translate("#GC-Gear_AddItem"), m_sItemName));
		string destination = WidgetManager.Translate("#GC-Gear_Inventory");
		if (m_bCrate)
			destination = WidgetManager.Translate("#GC-Gear_Crate");
		SetMessage(string.Format(WidgetManager.Translate("#GC-Gear_Destination"), destination));
		SetWarningMessage(string.Empty);
		m_Editbox.SetValue("1");
		m_Slider = SCR_SliderComponent.Cast(m_wRoot.FindAnyWidget("AmountSlider").FindHandler(SCR_SliderComponent));
		m_Slider.SetSliderSettings(1, 100, 1);
		m_Slider.SetValue(1);
		m_Slider.m_OnChanged.Insert(OnSliderChanged);
		m_OnTextChange.Insert(OnAmountChanged);
		m_OnCloseStart.Insert(OnClosing);
	}

	//------------------------------------------------------------------------------------------------
	//! Parse without truncating decimals, accepting junk suffixes or overflowing a signed int.
	static bool ParseAmount(string text, out int amount)
	{
		amount = 0;
		if (text.IsEmpty())
			return false;
		for (int i; i < text.Length(); i++)
		{
			int digit = "0123456789".IndexOf(text.Substring(i, 1));
			if (digit < 0 || amount > 214748364 || (amount == 214748364 && digit > 7))
				return false;
			amount = amount * 10 + digit;
		}
		return amount > 0;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnAmountChanged(string text)
	{
		if (m_bSynchronizing)
			return;
		int amount;
		bool valid = ParseAmount(text, amount);
		FindButton("confirm").SetEnabled(valid);
		if (!valid)
		{
			SetWarningMessage("#GC-Gear_InvalidAmount");
			return;
		}
		SetWarningMessage(string.Empty);
		m_bSynchronizing = true;
		m_Slider.SetValue(Math.Min(amount, 100));
		m_bSynchronizing = false;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSliderChanged(SCR_SliderComponent slider, float value)
	{
		if (m_bSynchronizing)
			return;
		int amount = Math.Round(value);
		m_bSynchronizing = true;
		m_Editbox.SetValue(amount.ToString());
		m_bSynchronizing = false;
		FindButton("confirm").SetEnabled(true);
		SetWarningMessage(string.Empty);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnConfirm()
	{
		if (m_bIsClosing)
			return;
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		int amount;
		if (!controller || !ParseAmount(m_Editbox.GetValue(), amount))
			return;
		if (!GC_GearSession.IsAdmin(controller.GetControlledEntity()) || m_iRevision != GC_GearSession.GetRevision() || m_bCrate != GC_GearSession.s_bCrate)
		{
			Close();
			return;
		}
		if (!controller.GC_RequestGearAmount(m_sPrefab, amount))
		{
			SetWarningMessage("#GC-Gear_Busy");
			return;
		}
		Close();
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuUpdate(float tDelta)
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!m_bIsClosing && (!controller || !GC_GearSession.IsAdmin(controller.GetControlledEntity()) || m_iRevision != GC_GearSession.GetRevision()))
			Close();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnClosing()
	{
		if (m_Slider)
			m_Slider.m_OnChanged.Remove(OnSliderChanged);
		m_OnTextChange.Remove(OnAmountChanged);
		m_OnCloseStart.Remove(OnClosing);
	}
}
