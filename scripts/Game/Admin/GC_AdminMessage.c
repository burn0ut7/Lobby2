//! Owner-local message history. It is never broadcast or included in JIP snapshots.
class GC_AdminMessage
{
	int m_iMessageId;
	int m_iSenderId;
	string m_sSenderName;
	string m_sText;
	string m_sTitle;
	bool m_bPreserveLineBreaks;
	bool m_bWarning;
	bool m_bMission;
	GC_EAdminMessageAudience m_eAudience;
}
