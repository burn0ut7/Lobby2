#ifdef WORKBENCH
modded class GC_LobbyManager
{
	protected ChimeraCharacter m_GC_EventActor;
	protected ChimeraCharacter m_GC_EventTarget;

	//------------------------------------------------------------------------------------------------
	//! Read-only authority/cache regression; does not create combat or change player state.
	string GC_TestEventLogCache(int requester)
	{
		GC_SessionLogComponent logs = GC_SessionLogComponent.GetInstance();
		if (!logs)
			return "UNAVAILABLE";
		array<string> rows;
		int next, revision;
		bool unchanged;
		string status;
		logs.ReadSpectatorPage(requester, true, 0, rows, next, status, revision, unchanged);
		string report = string.Format("first rows=%1 revision=%2 unchanged=%3 status=%4", rows.Count() / 7, revision, unchanged, status);
		int known = revision;
		logs.ReadSpectatorPage(requester, true, 0, rows, next, status, revision, unchanged, known);
		report += string.Format(" | repeat rows=%1 revision=%2 unchanged=%3 status=%4", rows.Count() / 7, revision, unchanged, status);
		logs.ReadSpectatorPage(-1, true, 0, rows, next, status, revision, unchanged, known);
		return report + string.Format(" | invalid rows=%1 revision=%2 unchanged=%3 status=%4", rows.Count() / 7, revision, unchanged, status);
	}

	//------------------------------------------------------------------------------------------------
	//! Native pointer hit-testing, separate from direct handler dispatch.
	string GC_TestEventLogPointer(string rowName, string cellName)
	{
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu)
			return "NO MENU";
		Widget row = menu.GetRootWidget().FindAnyWidget(rowName);
		if (!row)
			return "NO ROW";
		Widget cell = row.FindAnyWidget(cellName);
		if (!cell)
			return "NO CELL";
		float x, y, width, height;
		cell.GetScreenPos(x, y);
		cell.GetScreenSize(width, height);
		WidgetManager.ReportMouse(x + width * 0.5, y + height * 0.5, menu.GetRootWidget());
		Widget hit = WidgetManager.GetWidgetUnderCursor();
		if (!hit)
			return "NO HIT";
		return "hit=" + hit.GetName() + " button=" + (ButtonWidget.Cast(hit) != null).ToString();
	}

	//------------------------------------------------------------------------------------------------
	//! Native damage between test AI; production damage callbacks record it.
	string GC_TestEventLogCombat(int count)
	{
		if (!Replication.IsServer() || count < 1 || count > 30)
			return "UNAVAILABLE";
		if (!m_GC_EventActor)
		{
			EntitySpawnParams parameters = new EntitySpawnParams();
			parameters.TransformMode = ETransformMode.WORLD;
			parameters.Transform[3] = "150 1 150";
			Resource prefab = Resource.Load("{BF643BE4ADBDFDD3}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_LAT.et");
			m_GC_EventActor = ChimeraCharacter.Cast(GetGame().SpawnEntityPrefab(prefab, GetGame().GetWorld(), parameters));
			parameters.Transform[3] = "180 1 150";
			m_GC_EventTarget = ChimeraCharacter.Cast(GetGame().SpawnEntityPrefab(prefab, GetGame().GetWorld(), parameters));
		}
		ChimeraCharacter actor = m_GC_EventActor;
		ChimeraCharacter target = m_GC_EventTarget;
		if (!actor || !target || target.GetCharacterController().IsDead())
			return "NO LIVING AI";
		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(target.GetDamageManager());
		vector hit[3];
		hit[0] = target.GetOrigin();
		for (int i; i < count; i++)
		{
			SCR_DamageContext context = new SCR_DamageContext(EDamageType.MELEE, 1, hit, target, damage.GetHeadHitZone(), Instigator.CreateInstigator(actor), null, -1, -1);
			BaseWeaponComponent weapon = actor.GetCharacterController().GetWeaponManagerComponent().GetCurrentWeapon();
			if (weapon)
				context.damageSource = weapon.GetOwner();
			context.damageEffect = new SCR_MeleeDamageEffect();
			damage.HandleDamage(context);
		}
		return "DISPATCHED native AI damage count=" + count.ToString();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestEventLogRead(bool everyone, int before)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		GC_SessionLogComponent logs = GC_SessionLogComponent.GetInstance();
		if (!controller || !logs)
			return "UNAVAILABLE";
		array<string> rows;
		int next;
		string status;
		int revision;
		bool unchanged;
		logs.ReadSpectatorPage(controller.GetPlayerId(), everyone, before, rows, next, status, revision, unchanged);
		string report = string.Format("state=%1 admin=%2 rows=%3 next=%4 status=%5", controller.GetLobbyPlayerState(), SCR_Global.IsAdmin(controller.GetPlayerId()), rows.Count() / 7, next, status);
		for (int i; i < rows.Count(); i += 7)
			report += string.Format("\n%1 | %2 | %3 | %4 | %5 | position=%6", rows[i], rows[i + 1], rows[i + 2], rows[i + 3], rows[i + 4], rows[i + 6]);
		return report;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestEventLogSnapshot()
	{
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu)
			return "NO MENU";
		Widget panel = menu.GetRootWidget().FindAnyWidget("SpectatorEventLog");
		if (!panel)
			return "NO PANEL";
		GC_SpectatorEventLogComponent component = GC_SpectatorEventLogComponent.Cast(panel.FindHandler(GC_SpectatorEventLogComponent));
		return component.GC_TestReport();
	}
}

modded class GC_SpectatorEventLogComponent
{
	//------------------------------------------------------------------------------------------------
	string GC_TestReport()
	{
		string report = string.Format("active=%1 everyone=%2 rows=%3 personal=%4 next=%5", m_bActive, m_bEveryone, m_aRows.Count() / 7, m_Controller.GC_GetCachedSpectatorEvents(false).Count() / 7, m_Controller.GC_GetPersonalEventNext());
		report += " status=" + TextWidget.Cast(m_wRoot.FindAnyWidget("EventStatus")).GetText();
		foreach (SCR_ButtonBaseComponent button, int index : m_mRows)
			report += "\n" + button.GetRootWidget().GetName() + " | " + TextWidget.Cast(button.GetRootWidget().FindAnyWidget("EventSummary")).GetText() + " | " + TextWidget.Cast(button.GetRootWidget().FindAnyWidget("EventInfo")).GetText();
		CameraBase camera = GetGame().GetCameraManager().CurrentCamera();
		if (camera)
			report += "\ncamera=" + camera.GetOrigin().ToString();
		return report;
	}
}
#endif
