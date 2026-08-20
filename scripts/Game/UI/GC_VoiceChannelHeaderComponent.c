//! Server-authoritative interaction for a group voice-channel header.
class GC_VoiceChannelHeaderComponent : SCR_ButtonBaseComponent
{
    protected GC_VoiceChannelWidgetComponent m_Channel;

    override void HandlerAttached(Widget w)
    {
        super.HandlerAttached(w);

        if (SCR_Global.IsEditMode())
            return;

        m_bUseColorization = false;
        SetHoverSound(string.Empty);
        SetClickedSound(string.Empty);
        SetToggleable(false);

        Widget channelRoot = m_wRoot.GetParent();
        if (channelRoot)
            m_Channel = GC_VoiceChannelWidgetComponent.Cast(channelRoot.FindHandler(GC_VoiceChannelWidgetComponent));

    }

    //! The owning channel widget supplies this explicitly because FindHandler
    //! on a parent widget does not reliably resolve the parent's own handler.
    void SetChannel(GC_VoiceChannelWidgetComponent channel)
    {
        m_Channel = channel;
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (button != 0 && button != 1)
        {
            return true;
        }

        if (!m_Channel)
        {
            Print("GC | Voice header click rejected: channel component was not resolved", level: LogLevel.ERROR);
            return true;
        }

		if (button == 1)
		{
			if (m_Channel.GetChannelType() == GC_VoiceChannelType.GROUP && m_Channel.GetGroup())
			{
				GC_ContextMenuComponent.Open(GC_ContextMenuContext.ForGroup(m_Channel.GetGroup()), m_wRoot);
				return true;
			}

			FactionKey contextFactionKey;
			if (m_Channel.GetFaction())
				contextFactionKey = m_Channel.GetFaction().GetFactionKey();

			GC_ContextMenuComponent.Open(
				GC_ContextMenuContext.ForVoiceChannel(
					m_Channel.GetChannelType(),
					contextFactionKey,
					m_Channel.GetGroup(),
					m_Channel.GetPersonalOwnerIdentity(),
					m_Channel.GetPersonalOwnerPlayerId()
				),
				m_wRoot
			);
			return true;
		}

        GC_LobbyPlayerController playerController = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
        if (!playerController)
        {
            Print("GC | Voice header click rejected: local SCR_PlayerController was not found", level: LogLevel.ERROR);
            return true;
        }

        RplId groupRplId = RplId.Invalid();
        if (m_Channel.GetGroup())
            groupRplId = Replication.FindItemId(m_Channel.GetGroup());

        if (m_Channel.GetChannelType() == GC_VoiceChannelType.GROUP && groupRplId == RplId.Invalid())
        {
            PrintFormat("GC | Voice header click rejected: group is not replicated yet group=%1", m_Channel.GetGroup(), level: LogLevel.ERROR);
            return true;
        }

        string factionKey;
        if (m_Channel.GetFaction())
            factionKey = m_Channel.GetFaction().GetFactionKey();

        PlaySound(SCR_SoundEvent.CLICK);
        playerController.RequestVoiceChannel(m_Channel.GetChannelType(), factionKey, groupRplId, m_Channel.GetPersonalOwnerIdentity());
        return true;
    }
}
