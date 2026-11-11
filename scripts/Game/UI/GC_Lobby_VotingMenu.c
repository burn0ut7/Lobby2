class GC_Lobby_VotingMenu : ChimeraMenuBase
{
	protected GC_LobbyPlayerController m_Controller;
	protected ref GC_LobbyMenuChat m_MenuChat;
	protected ref array<SCR_ButtonBaseComponent> m_aCards = {};
	protected ref array<string> m_aRendered = {};
	protected SCR_ButtonTextComponent m_Continue;

	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		m_Controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		m_MenuChat = new GC_LobbyMenuChat();
		m_MenuChat.Start(this);
		GC_HeaderComponent header = GC_HeaderComponent.Cast(GetRootWidget().FindAnyWidget("MenuBar").FindHandler(GC_HeaderComponent));
		if (header)
			header.SetActiveState(4);
		m_Continue = SCR_ButtonTextComponent.GetButtonText("VotingContinue", GetRootWidget());
		m_Continue.SetText("#GC-Vote_Continue");
		m_Continue.SetToggleable(false);
		m_Continue.m_OnClicked.Insert(OnContinue);
		for (int i; i < 6; i++)
		{
			Widget card = GetGame().GetWorkspace().CreateWidgets("{E37A21004D950002}UI/layouts/Lobby_VoteCard.layout", GetRootWidget().FindAnyWidget("VotingCards"));
			card.SetName("VoteCard" + i);
			int column = i % 3;
			int row = i / 3;
			float x = column / 3.0;
			float y = row * 0.52;
			FrameSlot.SetAnchorMin(card, x, y);
			FrameSlot.SetAnchorMax(card, x + 0.32, y + 0.48);
			FrameSlot.SetOffsets(card, 0, 0, 0, 0);
			SCR_ButtonBaseComponent button = SCR_ButtonBaseComponent.Cast(card.FindHandler(SCR_ButtonBaseComponent));
			button.m_OnClicked.Insert(OnCard);
			m_aCards.Insert(button);
			m_aRendered.Insert("");
		}
		if (m_Controller)
			m_Controller.m_GC_OnVoteChanged.Insert(Refresh);
		Refresh();
		Poll();
		GetGame().GetCallqueue().CallLater(Poll, 1000, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void Poll()
	{
		if (m_Controller)
			m_Controller.GC_RequestVoteState();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnCard(SCR_ButtonBaseComponent button)
	{
		if (m_Controller)
			m_Controller.GC_VoteForMission(m_aCards.Find(button));
	}

	//------------------------------------------------------------------------------------------------
	protected void OnContinue(SCR_ButtonBaseComponent button)
	{
		Close();
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.GC_Lobby_DebriefMenu, 0, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void Refresh()
	{
		if (!m_Controller)
			return;
		int seconds = m_Controller.m_iGC_VoteSeconds;
		int minutes = seconds / 60;
		int remainder = seconds % 60;
		string clock = string.Format(WidgetManager.Translate("#GC-Vote_Clock"), minutes, remainder.ToString(2));
		if (m_Controller.m_bGC_VoteClosed)
			clock = "#GC-Vote_Closed";
		TextWidget.Cast(GetRootWidget().FindAnyWidget("VotingClock")).SetText(clock);
		string status = m_Controller.m_sGC_VoteStatus;
		if (!m_Controller.m_aGC_VoteCards.IsEmpty() && !m_Controller.m_bGC_VoteClosed)
			status = string.Empty;
		TextWidget.Cast(GetRootWidget().FindAnyWidget("VotingStatus")).SetText(status);
		for (int i; i < m_aCards.Count(); i++)
		{
			Widget card = m_aCards[i].GetRootWidget();
			bool visible = i < m_Controller.m_aGC_VoteCards.Count() && i < m_Controller.m_aGC_VoteCounts.Count()
				&& !m_Controller.m_aGC_VoteCards[i].IsEmpty();
			card.SetVisible(visible);
			if (!visible)
				continue;
			string row = m_Controller.m_aGC_VoteCards[i];
			if (row != m_aRendered[i])
			{
				m_aRendered[i] = row;
				RenderCard(card, row);
			}
			bool selected = m_Controller.m_iGC_VoteSelected == i;
			m_aCards[i].SetToggleable(true);
			m_aCards[i].SetToggled(selected, false, false, true);
			m_aCards[i].SetToggleable(false);
			m_aCards[i].SetEnabled(!m_Controller.m_bGC_VoteClosed && seconds > 0);
			string votes = string.Format(WidgetManager.Translate("#GC-Vote_Votes"), m_Controller.m_aGC_VoteCounts[i]);
			if (m_Controller.m_aGC_VoteCounts[i] == 1)
				votes = "#GC-Vote_OneVote";
			if (m_Controller.m_iGC_VoteWinner == i)
				votes = WidgetManager.Translate("#GC-Vote_Winner");
			TextWidget.Cast(card.FindAnyWidget("Votes")).SetText(votes);
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuFocusGained()
	{
		super.OnMenuFocusGained();
		if (m_MenuChat)
			m_MenuChat.Show();
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuFocusLost()
	{
		if (m_MenuChat)
			m_MenuChat.Hide();
		super.OnMenuFocusLost();
	}

	//------------------------------------------------------------------------------------------------
	protected void RenderCard(Widget card, string row)
	{
		JsonLoadContext json = new JsonLoadContext(false);
		if (!json.LoadFromString(row))
			return;
		string name, description, tags, thumbnail, plays, lastPlayed;
		json.ReadValue("name", name);
		json.ReadValue("description", description);
		json.ReadValue("tags", tags);
		json.ReadValue("thumbnail", thumbnail);
		json.ReadValue("plays", plays);
		json.ReadValue("lastPlayed", lastPlayed);
		int maxPlayers, minPlayers;
		if (json.ReadValue("maxPlayers", maxPlayers) && json.ReadValue("minPlayers", minPlayers)
			&& maxPlayers > 0 && minPlayers > 0 && minPlayers <= maxPlayers)
			name = string.Format("%1 (%2-%3)", name, maxPlayers, minPlayers);
		if (lastPlayed == "Never played")
			lastPlayed = WidgetManager.Translate("#GC-Vote_Never");
		TextWidget.Cast(card.FindAnyWidget("MissionName")).SetText(name);
		TextWidget.Cast(card.FindAnyWidget("Description")).SetText(description);
		Widget tagList = card.FindAnyWidget("Tags");
		SCR_WidgetHelper.RemoveAllChildren(tagList);
		array<string> labels = {};
		tags.Split(" · ", labels, true);
		foreach (string label : labels)
		{
			Widget tag = GetGame().GetWorkspace().CreateWidgets("{E37A21004D950003}UI/layouts/Lobby_VoteTag.layout", tagList);
			TextWidget.Cast(tag.FindAnyWidget("Label")).SetText(label);
		}
		TextWidget.Cast(card.FindAnyWidget("History")).SetText(string.Format(WidgetManager.Translate("#GC-Vote_History"), plays) + "\n" + lastPlayed);
		bool hasImage;
		if (!thumbnail.IsEmpty())
			hasImage = ImageWidget.Cast(card.FindAnyWidget("Thumbnail")).LoadImageTexture(0, thumbnail);
		if (hasImage)
			SCR_WidgetHelper.ResizeToImage(ImageWidget.Cast(card.FindAnyWidget("Thumbnail")));
		card.FindAnyWidget("Thumbnail").SetVisible(hasImage);
		card.FindAnyWidget("NoThumbnail").SetVisible(!hasImage);
		GetGame().GetCallqueue().Remove(ResizeDescriptions);
		GetGame().GetCallqueue().CallLater(ResizeDescriptions, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void ResizeDescriptions()
	{
		GetRootWidget().Update();
		foreach (SCR_ButtonBaseComponent button : m_aCards)
		{
			Widget card = button.GetRootWidget();
			float width, height;
			TextWidget.Cast(card.FindAnyWidget("Description")).GetTextSize(width, height);
			SizeLayoutWidget.Cast(card.FindAnyWidget("DescriptionSize")).SetHeightOverride(height + 4);
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		if (m_MenuChat)
			m_MenuChat.Update(tDelta);
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		GetGame().GetCallqueue().Remove(Poll);
		GetGame().GetCallqueue().Remove(ResizeDescriptions);
		if (m_Controller)
			m_Controller.m_GC_OnVoteChanged.Remove(Refresh);
		if (m_MenuChat)
			m_MenuChat.Stop();
		foreach (SCR_ButtonBaseComponent card : m_aCards)
			card.m_OnClicked.Remove(OnCard);
		m_Continue.m_OnClicked.Remove(OnContinue);
		super.OnMenuClose();
	}
}
