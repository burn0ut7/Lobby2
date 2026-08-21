class GC_ContextMenuActionButtonComponent : SCR_ButtonBaseComponent
{
	protected GC_ContextMenuAction m_Action;
	protected ref GC_ContextMenuContext m_Context;

	void Bind(GC_ContextMenuAction action, GC_ContextMenuContext context)
	{
		m_Action = action;
		m_Context = context;

		TextWidget label = TextWidget.Cast(m_wRoot.FindAnyWidget("ContextActionLabel"));
		if (label && m_Action)
			label.SetText(m_Action.GetLabel(m_Context));

		ImageWidget icon = ImageWidget.Cast(m_wRoot.FindAnyWidget("ContextActionIcon"));
		if (icon && m_Action)
		{
			ResourceName iconResource = m_Action.GetIcon(m_Context);
			string iconQuad = m_Action.GetIconQuad(m_Context);
			icon.SetVisible(iconResource != string.Empty);
			if (iconResource != string.Empty)
			{
				if (iconQuad.IsEmpty())
					icon.LoadImageTexture(0, iconResource);
				else
					icon.LoadImageFromSet(0, iconResource, iconQuad);
			}
		}

		SetEnabled(m_Action && m_Action.CanExecute(m_Context));
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button != 0 || !m_Action || !m_Action.CanExecute(m_Context))
			return true;

		m_Action.Execute(m_Context);
		GC_ContextMenuComponent.Close();
		return true;
	}

	override void HandlerDeattached(Widget w)
	{
		m_Action = null;
		m_Context = null;
		super.HandlerDeattached(w);
	}
}

//! Single popup presenter shared by lobby, briefing, and spectator surfaces.
class GC_ContextMenuComponent : SCR_ScriptedWidgetComponent
{
	protected static const ResourceName MENU_LAYOUT = "{6A57000000000001}UI/layouts/Context/Lobby_Context_Menu.layout";
	protected static const ResourceName ACTION_LAYOUT = "{6A57000000000002}UI/layouts/Context/Lobby_Context_Action.layout";
	protected static const float MENU_WIDTH = 280.0;
	protected static const float HEADER_HEIGHT = 34.0;
	protected static const float ACTION_HEIGHT = 36.0;
	protected static const float MENU_PADDING = 8.0;

	protected static Widget s_wActiveMenu;

	protected VerticalLayoutWidget m_wActions;
	protected TextWidget m_wHeader;
	protected ref GC_ContextMenuContext m_Context;
	protected GC_LobbyManager m_LobbyManager;
	protected float m_fMenuHeight;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wActions = VerticalLayoutWidget.Cast(w.FindAnyWidget("ContextActions"));
		m_wHeader = TextWidget.Cast(w.FindAnyWidget("ContextHeaderText"));
		m_LobbyManager = GC_LobbyManager.GetManager();
		if (m_LobbyManager)
			m_LobbyManager.m_OnPlayerRoleChanged.Insert(OnPlayerRoleChanged);

		InputManager inputManager = GetGame().GetInputManager();
		if (inputManager)
		{
			inputManager.AddActionListener("MouseLeft", EActionTrigger.UP, RequestClose);
			inputManager.AddActionListener("MouseRight", EActionTrigger.DOWN, RequestClose);
			inputManager.AddActionListener("MenuBack", EActionTrigger.DOWN, RequestClose);
			inputManager.AddActionListener("MenuOpen", EActionTrigger.DOWN, RequestClose);
		#ifdef WORKBENCH
			inputManager.AddActionListener("MenuBackWB", EActionTrigger.DOWN, RequestClose);
			inputManager.AddActionListener("MenuOpenWB", EActionTrigger.DOWN, RequestClose);
		#endif
		}
	}

	override void HandlerDeattached(Widget w)
	{
		InputManager inputManager = GetGame().GetInputManager();
		if (inputManager)
		{
			inputManager.RemoveActionListener("MouseLeft", EActionTrigger.UP, RequestClose);
			inputManager.RemoveActionListener("MouseRight", EActionTrigger.DOWN, RequestClose);
			inputManager.RemoveActionListener("MenuBack", EActionTrigger.DOWN, RequestClose);
			inputManager.RemoveActionListener("MenuOpen", EActionTrigger.DOWN, RequestClose);
		#ifdef WORKBENCH
			inputManager.RemoveActionListener("MenuBackWB", EActionTrigger.DOWN, RequestClose);
			inputManager.RemoveActionListener("MenuOpenWB", EActionTrigger.DOWN, RequestClose);
		#endif
		}

		if (s_wActiveMenu == m_wRoot)
			s_wActiveMenu = null;
		if (m_LobbyManager)
			m_LobbyManager.m_OnPlayerRoleChanged.Remove(OnPlayerRoleChanged);

		m_Context = null;
		m_LobbyManager = null;
		m_wHeader = null;
		m_wActions = null;
		super.HandlerDeattached(w);
	}

	static bool Open(GC_ContextMenuContext context, Widget sourceWidget)
	{
		Close();
		if (!context || !sourceWidget || !GC_LobbyManager.GetManager())
			return false;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return false;

		Widget screenRoot = sourceWidget;
		while (screenRoot.GetParent())
			screenRoot = screenRoot.GetParent();

		s_wActiveMenu = workspace.CreateWidgets(MENU_LAYOUT, screenRoot);
		if (!s_wActiveMenu)
			return false;

		GC_ContextMenuComponent component = GC_ContextMenuComponent.Cast(s_wActiveMenu.FindHandler(GC_ContextMenuComponent));
		if (!component || !component.Build(context))
		{
			Close();
			return false;
		}

		int mouseX;
		int mouseY;
		WidgetManager.GetMousePos(mouseX, mouseY);
		FrameSlot.SetPos(s_wActiveMenu, workspace.DPIUnscale(mouseX), workspace.DPIUnscale(mouseY));
		component.MoveInsideScreen();
		return true;
	}

	static void Close()
	{
		if (!s_wActiveMenu)
			return;

		Widget menu = s_wActiveMenu;
		s_wActiveMenu = null;
		menu.RemoveFromHierarchy();
	}

	protected bool Build(GC_ContextMenuContext context)
	{
		if (!context || !m_wActions || !m_wHeader)
			return false;

		m_Context = context;
		m_wHeader.SetTextFormat("%1  /  %2", context.GetTargetTypeName(), context.GetTargetLabel());
		GC_PlayerNamePresentation.Apply(m_wHeader, context.GetLabelPlayerId());

		array<ref GC_ContextMenuAction> actions = {};
		GC_ContextMenuActionRegistry.CollectVisible(context, actions);
		foreach (GC_ContextMenuAction action : actions)
		{
			Widget actionWidget = GetGame().GetWorkspace().CreateWidgets(ACTION_LAYOUT, m_wActions);
			if (!actionWidget)
				continue;

			GC_ContextMenuActionButtonComponent button = GC_ContextMenuActionButtonComponent.Cast(actionWidget.FindHandler(GC_ContextMenuActionButtonComponent));
			if (button)
				button.Bind(action, context);
		}

		if (!m_wActions.GetChildren())
			return false;

		m_fMenuHeight = HEADER_HEIGHT + actions.Count() * ACTION_HEIGHT + MENU_PADDING;
		return true;
	}

	protected void MoveInsideScreen()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		float screenWidth = workspace.DPIUnscale(workspace.GetWidth());
		float screenHeight = workspace.DPIUnscale(workspace.GetHeight());
		float x = Math.Clamp(FrameSlot.GetPosX(m_wRoot), 0.0, Math.Max(0.0, screenWidth - MENU_WIDTH));
		float y = Math.Clamp(FrameSlot.GetPosY(m_wRoot), 0.0, Math.Max(0.0, screenHeight - m_fMenuHeight));
		FrameSlot.SetPos(m_wRoot, x, y);
	}

	protected void OnPlayerRoleChanged(int playerId)
	{
		if (!m_Context)
			return;

		PlayerController playerController = GetGame().GetPlayerController();
		if (playerController && playerController.GetPlayerId() == playerId)
		{
			Close();
			return;
		}

		if (m_Context.GetLabelPlayerId() == playerId)
			GC_PlayerNamePresentation.Apply(m_wHeader, playerId);
	}

	protected void RequestClose()
	{
		GetGame().GetCallqueue().Call(Close);
	}
}
