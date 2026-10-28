class GC_SpectatorOptionButtonComponent : SCR_ButtonBaseComponent
{
	ref ScriptInvoker m_OnHoverChanged = new ScriptInvoker();

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		m_OnHoverChanged.Invoke(true);
		return super.OnMouseEnter(w, x, y);
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		m_OnHoverChanged.Invoke(false);
		return super.OnMouseLeave(w, enterW, x, y);
	}
}

//! The base manual camera intentionally accepts input only while no menu is
//! open or while the top menu is an EditorMenuUI. Using the camera-compatible
//! base here keeps Lobby2 scoped and avoids a global SCR_ManualCamera override.
class GC_Lobby_SpectatorMenu : EditorMenuUI
{
	protected static const float OPTION_TOOLTIP_GAP = 16;
	protected static const int OPTION_TOOLTIP_DELAY_MS = 1500;
	protected static const string INTERFACE_OPTION_DESCRIPTION = "Hide the spectator interface, chat, notifications, and all other game HUD elements. Press the shown key to restore them.";
	protected static const string NIGHT_MODE_OPTION_DESCRIPTION = "Brighten dark scenes locally using the same exposure behavior as the Game Master local light. This does not change lighting for other players.";
	protected static const string PROJECTILE_TRACERS_OPTION_DESCRIPTION = "Draw client-local paths in the 3D world and on the spectator map for bullets and other projectiles fired by tracked characters and vehicles.";
	protected static const string KILLED_FROM_OPTION_DESCRIPTION = "Show a client-local snapshot of the entity that killed you and trace its firing or explosive origin to the fatal impact point.";

	protected InputManager m_InputManager;
	protected SCR_MapEntity m_MapEntity;
	protected Widget m_wMapFrame;
	protected ref GC_SpectatorMapOverlay m_SpectatorMapOverlay;
	protected GC_SpectatorMarkerOverlay m_SpectatorMarkerOverlay;
	protected bool m_bSpectatorMapOpen;
	protected bool m_bInterfaceVisible = true;
	protected GC_SpectatorOptionButtonComponent m_InterfaceOptionButton;
	protected Widget m_wInterfaceOptionSource;
	protected TextWidget m_wInterfaceOptionState;
	protected Widget m_wInterfaceOptionStateBackground;
	protected GC_SpectatorOptionButtonComponent m_NightModeOptionButton;
	protected Widget m_wNightModeOptionSource;
	protected TextWidget m_wNightModeOptionState;
	protected Widget m_wNightModeOptionStateBackground;
	protected GC_SpectatorOptionButtonComponent m_ProjectileTracersOptionButton;
	protected Widget m_wProjectileTracersOptionSource;
	protected TextWidget m_wProjectileTracersOptionState;
	protected Widget m_wProjectileTracersOptionStateBackground;
	protected GC_SpectatorOptionButtonComponent m_KilledFromOptionButton;
	protected Widget m_wKilledFromOptionSource;
	protected TextWidget m_wKilledFromOptionState;
	protected Widget m_wKilledFromOptionStateBackground;
	protected Widget m_wOptionTooltipLayer;
	protected Widget m_wOptionTooltip;
	protected TextWidget m_wOptionTooltipText;
	protected Widget m_wHoveredOptionSource;
	protected string m_sHoveredOptionDescription;
	protected bool m_bOptionHovered;

	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		WidgetManager.SetCursor(0);

		m_InputManager = GetGame().GetInputManager();
		if (m_InputManager)
		{
			m_InputManager.AddActionListener(UIConstants.MENU_ACTION_BACK, EActionTrigger.DOWN, OnPauseMenuRequested);
			m_InputManager.AddActionListener("GC_ToggleSpectatorInterface", EActionTrigger.DOWN, OnToggleSpectatorInterface);
			m_InputManager.AddActionListener("GC_ToggleSpectatorProjectileTracers", EActionTrigger.DOWN, OnToggleProjectileTracers);
			m_InputManager.AddActionListener("GC_ToggleSpectatorKilledFrom", EActionTrigger.DOWN, OnToggleKilledFrom);
			m_InputManager.AddActionListener("GC_ToggleSpectatorMap", EActionTrigger.DOWN, OnToggleSpectatorMap);
			m_InputManager.AddActionListener("MapSelect", EActionTrigger.DOWN, OnSpectatorMapSelect);
			m_InputManager.AddActionListener("ManualCameraLight", EActionTrigger.DOWN, OnToggleNightMode);
			m_InputManager.AddActionListener("GC_SpectatorTeleport", EActionTrigger.DOWN, OnTeleportToCursor);
		}

		Widget root = GetRootWidget();
		Widget optionButton;
		if (root)
			optionButton = root.FindAnyWidget("SpectatorInterfaceOptionButton");
		if (optionButton)
		{
			m_wInterfaceOptionSource = optionButton;
			m_InterfaceOptionButton = GC_SpectatorOptionButtonComponent.Cast(optionButton.FindHandler(GC_SpectatorOptionButtonComponent));
		}
		if (root)
		{
			m_wMapFrame = root.FindAnyWidget("SpectatorMapFrame");
			CanvasWidget mapCanvas = CanvasWidget.Cast(root.FindAnyWidget("LobbySpectatorMapCharacters"));
			Widget mapTooltip = root.FindAnyWidget("LobbySpectatorMapTooltip");
			TextWidget mapTooltipText = TextWidget.Cast(root.FindAnyWidget("LobbySpectatorMapTooltipText"));
			m_MapEntity = SCR_MapEntity.GetMapInstance();
			m_SpectatorMapOverlay = new GC_SpectatorMapOverlay();
			if (!m_SpectatorMapOverlay.Start(mapCanvas, mapTooltip, mapTooltipText))
			{
				Print("GC | Spectator Map | overlay initialization failed", LogLevel.WARNING);
				m_SpectatorMapOverlay = null;
			}

			Widget projectileTracersOptionButton = root.FindAnyWidget("SpectatorProjectileTracersOptionButton");
			if (projectileTracersOptionButton)
			{
				m_wProjectileTracersOptionSource = projectileTracersOptionButton;
				m_ProjectileTracersOptionButton = GC_SpectatorOptionButtonComponent.Cast(projectileTracersOptionButton.FindHandler(GC_SpectatorOptionButtonComponent));
			}
			m_wInterfaceOptionState = TextWidget.Cast(root.FindAnyWidget("SpectatorInterfaceOptionState"));
			m_wInterfaceOptionStateBackground = root.FindAnyWidget("SpectatorInterfaceOptionStateBackground");
			Widget nightModeOptionButton = root.FindAnyWidget("SpectatorNightModeOptionButton");
			if (nightModeOptionButton)
			{
				m_wNightModeOptionSource = nightModeOptionButton;
				m_NightModeOptionButton = GC_SpectatorOptionButtonComponent.Cast(nightModeOptionButton.FindHandler(GC_SpectatorOptionButtonComponent));
			}
			m_wNightModeOptionState = TextWidget.Cast(root.FindAnyWidget("SpectatorNightModeOptionState"));
			m_wNightModeOptionStateBackground = root.FindAnyWidget("SpectatorNightModeOptionStateBackground");
			m_wProjectileTracersOptionState = TextWidget.Cast(root.FindAnyWidget("SpectatorProjectileTracersOptionState"));
			m_wProjectileTracersOptionStateBackground = root.FindAnyWidget("SpectatorProjectileTracersOptionStateBackground");
			Widget killedFromOptionButton = root.FindAnyWidget("SpectatorKilledFromOptionButton");
			if (killedFromOptionButton)
			{
				m_wKilledFromOptionSource = killedFromOptionButton;
				m_KilledFromOptionButton = GC_SpectatorOptionButtonComponent.Cast(killedFromOptionButton.FindHandler(GC_SpectatorOptionButtonComponent));
			}
			m_wKilledFromOptionState = TextWidget.Cast(root.FindAnyWidget("SpectatorKilledFromOptionState"));
			m_wKilledFromOptionStateBackground = root.FindAnyWidget("SpectatorKilledFromOptionStateBackground");
			m_wOptionTooltipLayer = root.FindAnyWidget("SpectatorOptionTooltipLayer");
			m_wOptionTooltip = root.FindAnyWidget("SpectatorOptionTooltip");
			m_wOptionTooltipText = TextWidget.Cast(root.FindAnyWidget("SpectatorOptionTooltipText"));
		}
		if (m_InterfaceOptionButton)
		{
			m_InterfaceOptionButton.m_OnClicked.Insert(OnToggleSpectatorInterface);
			m_InterfaceOptionButton.m_OnHoverChanged.Insert(OnInterfaceOptionHoverChanged);
		}
		if (m_ProjectileTracersOptionButton)
		{
			m_ProjectileTracersOptionButton.m_OnClicked.Insert(OnToggleProjectileTracers);
			m_ProjectileTracersOptionButton.m_OnHoverChanged.Insert(OnProjectileTracersOptionHoverChanged);
		}
		if (m_NightModeOptionButton)
		{
			m_NightModeOptionButton.m_OnClicked.Insert(OnToggleNightMode);
			m_NightModeOptionButton.m_OnHoverChanged.Insert(OnNightModeOptionHoverChanged);
		}
		if (m_KilledFromOptionButton)
		{
			m_KilledFromOptionButton.m_OnClicked.Insert(OnToggleKilledFrom);
			m_KilledFromOptionButton.m_OnHoverChanged.Insert(OnKilledFromOptionHoverChanged);
		}
		HideOptionTooltip();

		GC_LobbyPlayerController playerController = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		SetInterfaceVisible(!playerController || playerController.IsSpectatorInterfaceVisible());
		SetNightModeEnabled(playerController && playerController.IsSpectatorNightModeEnabled());
		SetProjectileTracersEnabled(playerController && playerController.AreSpectatorProjectileTracersEnabled());
		SetKilledFromEnabled(playerController && playerController.IsSpectatorKilledFromEnabled());
	}

	override void OnMenuFocusGained()
	{
		super.OnMenuFocusGained();
		GC_LobbyPlayerController playerController = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		SetInterfaceVisible(!playerController || playerController.IsSpectatorInterfaceVisible());
		if (playerController)
			playerController.SetSpectatorMenuFocused(true);
		SetNightModeEnabled(playerController && playerController.IsSpectatorNightModeEnabled());
		SetProjectileTracersEnabled(playerController && playerController.AreSpectatorProjectileTracersEnabled());
		SetKilledFromEnabled(playerController && playerController.IsSpectatorKilledFromEnabled());
	}

	override void OnMenuFocusLost()
	{
		GetGame().GetCallqueue().Remove(OpenNativeChat);
		super.OnMenuFocusLost();
		if (m_ChatPanel && m_ChatPanel.IsOpen())
			SCR_ChatPanelManager.GetInstance().CloseChatPanel(m_ChatPanel);
		CloseSpectatorMap();
		HideOptionTooltip();
		GC_LobbyPlayerController playerController = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (playerController)
			playerController.SetSpectatorMenuFocused(false);
		Widget root = GetRootWidget();
		if (root)
			root.SetVisible(false);
	}

	override void OnMenuClose()
	{
		GetGame().GetCallqueue().Remove(OpenNativeChat);
		if (m_ChatPanel && m_ChatPanel.IsOpen())
			SCR_ChatPanelManager.GetInstance().CloseChatPanel(m_ChatPanel);
		CloseSpectatorMap();
		GC_LobbyPlayerController playerController = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (playerController)
			playerController.SetSpectatorMenuFocused(false);
		if (m_SpectatorMapOverlay)
			m_SpectatorMapOverlay.Destroy();

		GetGame().GetCallqueue().Remove(OpenPauseMenu);
		if (m_InterfaceOptionButton)
		{
			m_InterfaceOptionButton.m_OnClicked.Remove(OnToggleSpectatorInterface);
			m_InterfaceOptionButton.m_OnHoverChanged.Remove(OnInterfaceOptionHoverChanged);
		}
		if (m_ProjectileTracersOptionButton)
		{
			m_ProjectileTracersOptionButton.m_OnClicked.Remove(OnToggleProjectileTracers);
			m_ProjectileTracersOptionButton.m_OnHoverChanged.Remove(OnProjectileTracersOptionHoverChanged);
		}
		if (m_NightModeOptionButton)
		{
			m_NightModeOptionButton.m_OnClicked.Remove(OnToggleNightMode);
			m_NightModeOptionButton.m_OnHoverChanged.Remove(OnNightModeOptionHoverChanged);
		}
		if (m_KilledFromOptionButton)
		{
			m_KilledFromOptionButton.m_OnClicked.Remove(OnToggleKilledFrom);
			m_KilledFromOptionButton.m_OnHoverChanged.Remove(OnKilledFromOptionHoverChanged);
		}
		if (m_InputManager)
		{
			m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_BACK, EActionTrigger.DOWN, OnPauseMenuRequested);
			m_InputManager.RemoveActionListener("GC_ToggleSpectatorInterface", EActionTrigger.DOWN, OnToggleSpectatorInterface);
			m_InputManager.RemoveActionListener("GC_ToggleSpectatorProjectileTracers", EActionTrigger.DOWN, OnToggleProjectileTracers);
			m_InputManager.RemoveActionListener("GC_ToggleSpectatorKilledFrom", EActionTrigger.DOWN, OnToggleKilledFrom);
			m_InputManager.RemoveActionListener("GC_ToggleSpectatorMap", EActionTrigger.DOWN, OnToggleSpectatorMap);
			m_InputManager.RemoveActionListener("MapSelect", EActionTrigger.DOWN, OnSpectatorMapSelect);
			m_InputManager.RemoveActionListener("ManualCameraLight", EActionTrigger.DOWN, OnToggleNightMode);
			m_InputManager.RemoveActionListener("GC_SpectatorTeleport", EActionTrigger.DOWN, OnTeleportToCursor);
		}

		HideOptionTooltip();
		m_wOptionTooltipText = null;
		m_wOptionTooltip = null;
		m_wOptionTooltipLayer = null;
		m_wHoveredOptionSource = null;
		m_wProjectileTracersOptionStateBackground = null;
		m_wProjectileTracersOptionState = null;
		m_wProjectileTracersOptionSource = null;
		m_ProjectileTracersOptionButton = null;
		m_wNightModeOptionStateBackground = null;
		m_wNightModeOptionState = null;
		m_wNightModeOptionSource = null;
		m_NightModeOptionButton = null;
		m_wKilledFromOptionStateBackground = null;
		m_wKilledFromOptionState = null;
		m_wKilledFromOptionSource = null;
		m_KilledFromOptionButton = null;
		m_wInterfaceOptionStateBackground = null;
		m_wInterfaceOptionState = null;
		m_wInterfaceOptionSource = null;
		m_InterfaceOptionButton = null;
		m_SpectatorMarkerOverlay = null;
		m_SpectatorMapOverlay = null;
		m_wMapFrame = null;
		m_MapEntity = null;
		m_InputManager = null;
		super.OnMenuClose();
	}

	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		if (!m_bInterfaceVisible)
			return;

		if (m_SpectatorMarkerOverlay)
			m_SpectatorMarkerOverlay.Update();
		if (!m_bSpectatorMapOpen || !IsFocused())
			return;

		WidgetManager.SetCursor(0);
		if (m_InputManager)
			m_InputManager.ActivateContext("GC_SpectatorMap");
		if (m_SpectatorMapOverlay)
			m_SpectatorMapOverlay.Update(tDelta);
	}

	void SetMarkerOverlay(GC_SpectatorMarkerOverlay markerOverlay)
	{
		m_SpectatorMarkerOverlay = markerOverlay;
	}

	void SetInterfaceVisible(bool visible)
	{
		m_bInterfaceVisible = visible;
		if (!visible)
		{
			GetGame().GetCallqueue().Remove(OpenNativeChat);
			if (m_ChatPanel && m_ChatPanel.IsOpen())
				SCR_ChatPanelManager.GetInstance().CloseChatPanel(m_ChatPanel);
			CloseSpectatorMap();
			HideOptionTooltip();
		}

		if (visible)
			WidgetManager.SetCursor(0);
		else
			WidgetManager.SetCursor(-1);

		UpdateBooleanOption(m_wInterfaceOptionState, m_wInterfaceOptionStateBackground, visible);

		Widget root = GetRootWidget();
		if (root)
			root.SetVisible(visible);
	}

	//------------------------------------------------------------------------------------------------
	override void ChatToggle()
	{
		// Native gameplay HUD also handles this action. Select our visible panel
		// after its callback, as the other lobby menus do.
		GetGame().GetCallqueue().Remove(OpenNativeChat);
		GetGame().GetCallqueue().CallLater(OpenNativeChat, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void OpenNativeChat()
	{
		if (!IsFocused() || !m_bInterfaceVisible || !GC_LobbyManager.GetManager())
			return;
		ShowChat();
		super.ChatToggle();
	}

	protected void OnToggleSpectatorInterface()
	{
		GC_LobbyPlayerController playerController = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (playerController)
			playerController.ToggleSpectatorInterface();
	}

	protected void OnToggleSpectatorMap()
	{
		if (!m_bInterfaceVisible || !IsFocused())
			return;

		if (m_bSpectatorMapOpen)
			CloseSpectatorMap();
		else
			OpenSpectatorMap();
	}

	protected void OpenSpectatorMap()
	{
		if (!m_wMapFrame || !m_MapEntity)
			return;
		if (m_MapEntity.IsOpen())
		{
			Print("GC | Spectator Map | another map owns the active view", LogLevel.WARNING);
			return;
		}
		if (!m_wMapFrame.FindAnyWidget("MapWidget"))
		{
			Print("GC | Spectator Map | map layout has no MapWidget", LogLevel.ERROR);
			return;
		}
		GC_LobbyGameMode gameMode = GC_LobbyGameMode.GetGameMode();
		if (!gameMode)
		{
			Print("GC | Spectator Map | active game mode is unavailable", LogLevel.ERROR);
			return;
		}

		ResourceName mapConfigPath = "{6A75000000000003}Configs/Map/MapSpectator.conf";
		MapConfiguration mapConfig = m_MapEntity.SetupMapConfig(EMapEntityMode.GC_SPECTATOR, mapConfigPath, m_wMapFrame);
		if (!mapConfig)
		{
			PrintFormat("GC | Spectator Map | map configuration failed config=%1", mapConfigPath, level: LogLevel.ERROR);
			return;
		}

		m_wMapFrame.SetVisible(true);
		m_MapEntity.OpenMap(mapConfig);
		m_bSpectatorMapOpen = true;
		SCR_ManualCamera camera = SCR_ManualCamera.Cast(GetGame().GetCameraManager().CurrentCamera());
		if (camera)
			camera.SetInputEnabled(false);
		if (m_SpectatorMapOverlay)
			m_SpectatorMapOverlay.Update(0.0, true);
		PrintFormat("GC | Spectator Map | opened config=%1 markers=%2 drawings=%3", mapConfigPath, m_MapEntity.GetMapUIComponent(SCR_MapMarkersUI) != null, m_MapEntity.GetMapUIComponent(SCR_MapDrawingUI) != null);
	}

	protected void CloseSpectatorMap()
	{
		if (!m_bSpectatorMapOpen)
			return;

		m_bSpectatorMapOpen = false;
		if (m_SpectatorMapOverlay)
			m_SpectatorMapOverlay.Clear();
		if (m_MapEntity && m_MapEntity.IsOpen() && m_MapEntity.GetMapConfig().RootWidgetRef == m_wMapFrame)
			m_MapEntity.CloseMap();
		if (m_wMapFrame)
			m_wMapFrame.SetVisible(false);

		SCR_ManualCamera camera = SCR_ManualCamera.Cast(GetGame().GetCameraManager().CurrentCamera());
		if (camera)
			camera.SetInputEnabled(true);
		Print("GC | Spectator Map | closed");
	}

	protected void OnSpectatorMapSelect(float value, EActionTrigger reason)
	{
		if (!m_bSpectatorMapOpen || !m_SpectatorMapOverlay)
			return;

		// Refresh the hit test during the input callback so a fast mouse move and
		// click cannot select the marker hovered on the preceding frame.
		m_SpectatorMapOverlay.Update(0.0);
		IEntity target = m_SpectatorMapOverlay.GetHoveredEntity();
		if (!target)
			return;

		CameraManager cameraManager = GetGame().GetCameraManager();
		if (!cameraManager)
			return;

		GC_SpectatorManualCamera spectatorCamera = GC_SpectatorManualCamera.Cast(cameraManager.CurrentCamera());
		if (!spectatorCamera || !spectatorCamera.TeleportToEntityEye(target))
			return;

		CloseSpectatorMap();
		PrintFormat("GC | Spectator Map | eye teleport selected entity=%1", target);
	}

	void SetProjectileTracersEnabled(bool enabled)
	{
		UpdateBooleanOption(m_wProjectileTracersOptionState, m_wProjectileTracersOptionStateBackground, enabled);
	}

	void SetMapProjectileTracer(GC_SpectatorProjectileTracer projectileTracer)
	{
		if (m_SpectatorMapOverlay)
			m_SpectatorMapOverlay.SetProjectileTracer(projectileTracer);
	}

	void SetNightModeEnabled(bool enabled)
	{
		UpdateBooleanOption(m_wNightModeOptionState, m_wNightModeOptionStateBackground, enabled);
	}

	void SetKilledFromEnabled(bool enabled)
	{
		UpdateBooleanOption(m_wKilledFromOptionState, m_wKilledFromOptionStateBackground, enabled);
	}

	protected void UpdateBooleanOption(TextWidget state, Widget background, bool enabled)
	{
		if (state)
		{
			if (enabled)
				state.SetText("ON");
			else
				state.SetText("OFF");
		}
		if (background)
		{
			if (enabled)
				background.SetColor(Color.FromRGBA(48, 148, 112, 255));
			else
				background.SetColor(Color.FromRGBA(82, 88, 90, 255));
		}
	}

	protected void OnToggleProjectileTracers()
	{
		GC_LobbyPlayerController playerController = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (playerController)
			playerController.ToggleSpectatorProjectileTracers();
	}

	protected void OnToggleNightMode()
	{
		GC_LobbyPlayerController playerController = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (playerController)
			playerController.ToggleSpectatorNightMode();
	}

	protected void OnToggleKilledFrom()
	{
		GC_LobbyPlayerController playerController = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (playerController)
			playerController.ToggleSpectatorKilledFrom();
	}

	protected void OnTeleportToCursor()
	{
		BaseWorld world = GetGame().GetWorld();
		CameraManager cameraManager = GetGame().GetCameraManager();
		if (!world || !cameraManager)
			return;

		GC_SpectatorManualCamera spectatorCamera = GC_SpectatorManualCamera.Cast(cameraManager.CurrentCamera());
		if (!spectatorCamera)
			return;

		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (mapEntity && mapEntity.IsOpen())
		{
			int mouseX;
			int mouseY;
			WidgetManager.GetMousePos(mouseX, mouseY);

			float worldX;
			float worldZ;
			mapEntity.ScreenToWorld(mouseX, mouseY, worldX, worldZ);

			vector mapWorldPosition = {worldX, world.GetSurfaceY(worldX, worldZ), worldZ};
			spectatorCamera.TeleportToWorldPosition(mapWorldPosition);
			return;
		}

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		int cursorX;
		int cursorY;
		WidgetManager.GetMousePos(cursorX, cursorY);

		vector traceDirection;
		vector traceStart = workspace.ProjScreenToWorld(workspace.DPIUnscale(cursorX), workspace.DPIUnscale(cursorY), traceDirection, world, -1);
		traceDirection *= 1000.0;

		autoptr TraceParam trace = new TraceParam();
		trace.Start = traceStart;
		trace.End = traceStart + traceDirection;
		trace.Flags = TraceFlags.ANY_CONTACT | TraceFlags.WORLD | TraceFlags.ENTS | TraceFlags.OCEAN;
		trace.LayerMask = EPhysicsLayerPresets.Projectile;

		float traceDistance = world.TraceMove(trace, null);
		vector cursorWorldPosition = traceStart + traceDirection * traceDistance;
		spectatorCamera.TeleportToWorldPosition(cursorWorldPosition, true);
	}

	protected void OnInterfaceOptionHoverChanged(bool hovered)
	{
		HandleOptionHover(m_wInterfaceOptionSource, INTERFACE_OPTION_DESCRIPTION, hovered);
	}

	protected void OnProjectileTracersOptionHoverChanged(bool hovered)
	{
		HandleOptionHover(m_wProjectileTracersOptionSource, PROJECTILE_TRACERS_OPTION_DESCRIPTION, hovered);
	}

	protected void OnNightModeOptionHoverChanged(bool hovered)
	{
		HandleOptionHover(m_wNightModeOptionSource, NIGHT_MODE_OPTION_DESCRIPTION, hovered);
	}

	protected void OnKilledFromOptionHoverChanged(bool hovered)
	{
		HandleOptionHover(m_wKilledFromOptionSource, KILLED_FROM_OPTION_DESCRIPTION, hovered);
	}

	protected void HandleOptionHover(Widget source, string description, bool hovered)
	{
		m_bOptionHovered = hovered;
		GetGame().GetCallqueue().Remove(ShowOptionTooltip);
		if (hovered)
		{
			m_wHoveredOptionSource = source;
			m_sHoveredOptionDescription = description;
			GetGame().GetCallqueue().CallLater(ShowOptionTooltip, OPTION_TOOLTIP_DELAY_MS);
		}
		else
			HideOptionTooltip();
	}

	protected void ShowOptionTooltip()
	{
		if (!m_bOptionHovered)
			return;

		if (!m_wHoveredOptionSource || !m_wOptionTooltipLayer || !m_wOptionTooltip || !m_wOptionTooltipText)
			return;

		m_wOptionTooltipText.SetText(m_sHoveredOptionDescription);
		m_wOptionTooltip.SetVisible(true);
		GetGame().GetCallqueue().Remove(PositionOptionTooltip);
		GetGame().GetCallqueue().CallLater(PositionOptionTooltip, 0);
	}

	protected void PositionOptionTooltip()
	{
		if (!m_bOptionHovered || !m_wHoveredOptionSource || !m_wOptionTooltipLayer || !m_wOptionTooltip)
			return;

		float layerX;
		float layerY;
		float layerWidth;
		float layerHeight;
		float tooltipWidth;
		float tooltipHeight;
		m_wOptionTooltipLayer.GetScreenPos(layerX, layerY);
		m_wOptionTooltipLayer.GetScreenSize(layerWidth, layerHeight);
		m_wOptionTooltip.GetScreenSize(tooltipWidth, tooltipHeight);
		int mouseX;
		int mouseY;
		WidgetManager.GetMousePos(mouseX, mouseY);

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		float localX = workspace.DPIUnscale(mouseX - layerX) + OPTION_TOOLTIP_GAP;
		float localY = workspace.DPIUnscale(mouseY - layerY) + OPTION_TOOLTIP_GAP;
		float localTooltipWidth = workspace.DPIUnscale(tooltipWidth);
		float localTooltipHeight = workspace.DPIUnscale(tooltipHeight);
		float availableWidth = workspace.DPIUnscale(layerWidth);
		float availableHeight = workspace.DPIUnscale(layerHeight);
		if (localX + localTooltipWidth > availableWidth)
			localX = workspace.DPIUnscale(mouseX - layerX) - localTooltipWidth - OPTION_TOOLTIP_GAP;
		if (localY + localTooltipHeight > availableHeight)
			localY = workspace.DPIUnscale(mouseY - layerY) - localTooltipHeight - OPTION_TOOLTIP_GAP;
		if (localX + localTooltipWidth > availableWidth)
			localX = availableWidth - localTooltipWidth;
		if (localX < 0)
			localX = 0;
		if (localY + localTooltipHeight > availableHeight)
			localY = availableHeight - localTooltipHeight;
		if (localY < 0)
			localY = 0;

		FrameSlot.SetPos(m_wOptionTooltip, localX, localY);
	}

	protected void HideOptionTooltip()
	{
		m_bOptionHovered = false;
		m_wHoveredOptionSource = null;
		m_sHoveredOptionDescription = string.Empty;
		GetGame().GetCallqueue().Remove(ShowOptionTooltip);
		GetGame().GetCallqueue().Remove(PositionOptionTooltip);
		if (m_wOptionTooltip)
			m_wOptionTooltip.SetVisible(false);
	}

	protected void OnPauseMenuRequested()
	{
		HideOptionTooltip();
		GetGame().GetCallqueue().Remove(OpenPauseMenu);
		GetGame().GetCallqueue().CallLater(OpenPauseMenu, 0);
	}

	override protected void OpenPauseMenu()
	{
		if (!GetGame().GetMenuManager().IsAnyDialogOpen() && IsFocused())
			ArmaReforgerScripted.OpenPauseMenu(false, true);
	}
}
