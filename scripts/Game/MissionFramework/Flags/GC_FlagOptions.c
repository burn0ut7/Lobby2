[BaseContainerProps(), BaseContainerCustomStringTitleField("Advanced")]
class GC_FlagOptions
{
	[Attribute("0", UIWidgets.SpinBox, "Start with this remembered call count. Flag Action can change it later. Default 0 means false; 1 or more means true. This does not perform a call. A positive count uses a non-repeatable flag's activation; enable Repeatable to accept more calls. Conditions control future calls. Flag Action > Reset to zero clears to zero, not this starting count.", "0 1000000 1")]
	int m_iStartCallCount;
	[Attribute("0", UIWidgets.CheckBox, "Unchecked: accept one successful call until Flag Action > Reset to zero is used. Checked: accept more calls, increasing the count and notifying dependent events each time. A remembered count of zero means false; one or more means true.")]
	bool m_bRepeatable;
}
