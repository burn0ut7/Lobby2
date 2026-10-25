[ComponentEditorProps(category: "Lobby2/Mission", description: "Local proximity wall for setup/AO polygons. Uses the vanilla virtual-area mesh and texture; no collision. Maximum 60 polygon points.")]
class GC_BoundaryWallComponentClass : ScriptComponentClass
{
}

class GC_BoundaryWallComponent : ScriptComponent
{
	[Attribute("1", UIWidgets.CheckBox, "Show the boundary only to affected players near an edge.", category: "Boundary wall"), RplProp(onRplName: "ClearMesh")]
	protected bool m_bEnabled;
	[Attribute("10", UIWidgets.EditBox, "Fade in within this horizontal distance from the boundary, in meters.", params: "1 100", category: "Boundary wall"), RplProp()]
	protected float m_fRange;
	[Attribute("3", UIWidgets.EditBox, "Wall height above terrain, in meters.", params: "0.5 20", category: "Boundary wall"), RplProp(onRplName: "ClearMesh")]
	protected float m_fHeight;
	[Attribute("0.578 0 0 0.35", UIWidgets.ColorPicker, "Wall color; alpha is maximum opacity at the boundary.", category: "Boundary wall")]
	protected ref Color m_Color;
	[RplProp(onRplName: "ClearMesh")]
	protected int m_iColor;
	[RplProp(onRplName: "ClearMesh")]
	protected ref array<vector> m_aPolygon = {};
	[RplProp()]
	protected ref array<RplId> m_aAudience = {};
	protected GC_SetupZoneEntity m_Setup;
	protected GC_AOLimitEntity m_AO;
	protected IEntity m_Wall;
	protected ref Material m_Material;
	protected bool m_bFailed;
	protected float m_fOpacity = -1;
	protected float m_fMaximumOpacity;
	protected float m_fVisualElapsed;
	protected static int s_iMaterialSerial;

	//------------------------------------------------------------------------------------------------
	//! Trusted server configuration. Does not change enforcement or its polygon.
	bool Configure(bool enabled, float range, float height, int color)
	{
		if (!Replication.IsServer() || range < 1 || range > 100 || height < 0.5 || height > 20)
			return false;
		m_bEnabled = enabled;
		m_fRange = range;
		m_fHeight = height;
		m_iColor = color;
		m_Color = Color.FromInt(color);
		ClearMesh();
		Replication.BumpMe();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshAuthority()
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		array<RplId> audience = {};
		array<vector> polygon = {};
		if (m_Setup)
			m_Setup.GetBoundaryPolygon(polygon);
		else if (m_AO)
			m_AO.GetMapPolygon(polygon);
		if (m_bEnabled && mode && mode.GetState() == SCR_EGameModeState.GAME && polygon.Count() >= 3)
		{
			array<int> players = {};
			PlayerManager manager = GetGame().GetPlayerManager();
			manager.GetPlayers(players);
			GC_LobbyFreezeComponent freeze = GC_LobbyFreezeComponent.GetInstance();
			foreach (int playerId : players)
			{
				GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(manager.GetPlayerController(playerId));
				if (!controller || controller.GetLobbyPlayerState() != GC_ELobbyPlayerState.PLAYING)
					continue;
				IEntity character = controller.GetControlledEntity();
				if (!character || !SCR_AIDamageHandling.IsAlive(character))
					continue;
				bool applies;
				if (m_Setup)
					applies = controller.GC_GetSetupZone() == m_Setup;
				else if (m_AO && !controller.GC_SetupActive() && !(freeze && freeze.AffectsPlayer(playerId)))
					applies = m_AO.AppliesTo(playerId, character, CompartmentAccessComponent.GetVehicleIn(character));
				if (applies)
					audience.Insert(Replication.FindItemId(character));
			}
		}
		bool changed = audience.Count() != m_aAudience.Count();
		if (!changed)
		{
			foreach (RplId id : audience)
				changed = changed || !m_aAudience.Contains(id);
		}
		bool geometryChanged = polygon.Count() != m_aPolygon.Count();
		if (!geometryChanged)
		{
			foreach (int index, vector point : polygon)
				geometryChanged = geometryChanged || point != m_aPolygon[index];
		}
		if (geometryChanged)
		{
			m_aPolygon.Copy(polygon);
			ClearMesh();
		}
		if (changed)
			m_aAudience.Copy(audience);
		if (changed || geometryChanged)
			Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearMesh()
	{
		if (m_Wall)
			SCR_EntityHelper.DeleteEntityAndChildren(m_Wall);
		m_Wall = null;
		m_Material = null;
		m_bFailed = false;
		m_fOpacity = -1;
	}

	//------------------------------------------------------------------------------------------------
	protected bool CreateMesh()
	{
		// SCR_Shape's native mesh wrapper has a fixed 60-point buffer.
		if (m_aPolygon.Count() > 60)
		{
			PrintFormat("GC | Boundary wall | maximum 60 points exceeded: %1", GetOwner().GetName(), level: LogLevel.ERROR);
			return false;
		}
		Color color = Color.FromInt(m_iColor);
		m_fMaximumOpacity = color.A();
		map<string, string> parameters = new map<string, string>();
		// Match vanilla VirtualArea.emat, with a private material for color/fading.
		parameters.Set("Color", string.Format("%1 %2 %3 1", color.R(), color.G(), color.B()));
		parameters.Set("Cull", "none");
		parameters.Set("Sort", "translucent");
		parameters.Set("CastShadow", "0");
		parameters.Set("ReceiveShadow", "0");
		parameters.Set("AlphaTest", "0.01");
		parameters.Set("AlphaMul", "0");
		parameters.Set("BlendMode", "AlphaBlend");
		parameters.Set("AlbedoMap", "{930D6120C56C73C9}Assets/Editor/VirtualArea/VirtualArea_01.edds");
		parameters.Set("DisablePrexposure", "1");
		s_iMaterialSerial++;
		m_Material = Material.Create("GC_BoundaryWall_" + s_iMaterialSerial.ToString(), "MatCommon", parameters);
		if (!m_Material)
			return false;
		string materialName;
		m_Material.GetName(materialName);
		vector origin = m_aPolygon[0];
		array<vector> positions = {};
		BaseWorld world = GetOwner().GetWorld();
		foreach (vector point : m_aPolygon)
		{
			point[1] = world.GetSurfaceY(point[0], point[2]) - 0.05;
			positions.Insert(point - origin);
		}
		Resource mesh = SCR_Shape.CreateAreaMesh(positions, m_fHeight, materialName, true);
		if (!mesh || !mesh.GetResource())
			return false;
		EntitySpawnParams spawn = new EntitySpawnParams();
		spawn.TransformMode = ETransformMode.WORLD;
		spawn.Transform[3] = origin;
		m_Wall = GetGame().SpawnEntity(GenericEntity, world, spawn);
		if (!m_Wall)
			return false;
		m_Wall.SetObject(mesh.GetResource().ToMeshObject(), "");
		m_Wall.SetFlags(EntityFlags.VISIBLE);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		// Bound proximity/material work independently of the client's rendering FPS.
		m_fVisualElapsed += timeSlice;
		if (m_fVisualElapsed < 0.033)
			return;
		m_fVisualElapsed = 0;
		if (!m_bEnabled)
		{
			if (m_Wall || m_Material)
				ClearMesh();
			return;
		}
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		IEntity character;
		if (controller)
			character = controller.GetControlledEntity();
		bool visible = m_bEnabled && mode && mode.GetState() == SCR_EGameModeState.GAME
			&& character && SCR_AIDamageHandling.IsAlive(character) && m_aPolygon.Count() >= 3
			&& m_aAudience.Contains(Replication.FindItemId(character));
		if (visible && m_Setup)
			visible = controller.GC_SetupActive();
		if (!visible)
		{
			if (m_Wall || m_Material)
				ClearMesh();
			return;
		}
		vector position = character.GetOrigin();
		position[1] = 0;
		float distanceSq = m_fRange * m_fRange;
		for (int i; i < m_aPolygon.Count(); i++)
		{
			vector first = m_aPolygon[i];
			vector second = m_aPolygon[(i + 1) % m_aPolygon.Count()];
			first[1] = 0;
			second[1] = 0;
			distanceSq = Math.Min(distanceSq, Math3D.PointLineSegmentDistanceSqr(position, first, second));
		}
		float fraction = 1 - Math.Sqrt(distanceSq) / m_fRange;
		if (!m_Wall && fraction > 0 && !m_bFailed)
		{
			m_bFailed = !CreateMesh();
			if (m_bFailed)
				PrintFormat("GC | Boundary wall | mesh unavailable: %1", owner.GetName(), level: LogLevel.ERROR);
		}
		if (!m_Wall)
			return;
		float opacity = m_fMaximumOpacity * fraction * fraction * (3 - 2 * fraction);
		if (Math.AbsFloat(opacity - m_fOpacity) > 0.001)
		{
			m_Material.SetParam("AlphaMul", opacity);
			m_fOpacity = opacity;
		}
		if (fraction <= 0)
			m_Wall.ClearFlags(EntityFlags.VISIBLE);
		else
			m_Wall.SetFlags(EntityFlags.VISIBLE);
	}

	//------------------------------------------------------------------------------------------------
	protected void Initialize()
	{
		if (SCR_Global.IsEditMode() || !GC_LobbyGameMode.GetGameMode())
			return;
		IEntity owner = GetOwner();
		m_Setup = GC_SetupZoneEntity.Cast(owner);
		m_AO = GC_AOLimitEntity.Cast(owner);
		if (!m_Setup && !m_AO)
			return;
		if (Replication.IsServer())
		{
			m_fRange = Math.Clamp(m_fRange, 1, 100);
			m_fHeight = Math.Clamp(m_fHeight, 0.5, 20);
			m_iColor = m_Color.PackToInt();
			Replication.BumpMe();
			GetGame().GetCallqueue().CallLater(RefreshAuthority, 250, true);
		}
		if (!System.IsConsoleApp())
			SetEventMask(owner, EntityEvent.FRAME);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!SCR_Global.IsEditMode())
			GetGame().GetCallqueue().CallLater(Initialize, 0);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (GetGame())
		{
			GetGame().GetCallqueue().Remove(Initialize);
			GetGame().GetCallqueue().Remove(RefreshAuthority);
		}
		ClearMesh();
		super.OnDelete(owner);
	}
}
