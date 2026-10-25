[ComponentEditorProps(category: "Lobby2/Mission", description: "Local proximity wall for setup/AO polygons. Uses native meshes and the vanilla virtual-area texture; no collision. Maximum 60 polygon points.")]
class GC_BoundaryWallComponentClass : ScriptComponentClass
{
}

class GC_BoundaryWallComponent : ScriptComponent
{
	[Attribute("1", UIWidgets.CheckBox, "Show the boundary only to affected players near an edge.", category: "Boundary wall"), RplProp(onRplName: "ClearMesh")]
	protected bool m_bEnabled;
	[Attribute("10", UIWidgets.EditBox, "Show only wall portions within this horizontal radius of the player, in meters.", params: "1 100", category: "Boundary wall"), RplProp(onRplName: "ClearMesh")]
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
	protected ref Resource m_Mesh;
	protected ref array<vector> m_aGroundPoints = {};
	protected vector m_aVertices[2040]; // 60 edges * 17 columns * 2 vertices
	protected int m_iVisibleColumns;
	protected vector m_vLastVisualPosition;
	protected bool m_bPositionValid;
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
				else if (m_AO && !(freeze && freeze.AffectsPlayer(playerId)))
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
		m_Mesh = null;
		m_aGroundPoints.Clear();
		m_bPositionValid = false;
		m_bFailed = false;
		m_fOpacity = -1;
	}

	//------------------------------------------------------------------------------------------------
	protected bool CreateWall()
	{
		// Match the supported polygon limit with a bounded geometry buffer.
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
		parameters.Set("AlphaMul", m_fMaximumOpacity.ToString());
		parameters.Set("BlendMode", "AlphaBlend");
		parameters.Set("AlbedoMap", "{930D6120C56C73C9}Assets/Editor/VirtualArea/VirtualArea_01.edds");
		parameters.Set("DisablePrexposure", "1");
		s_iMaterialSerial++;
		m_Material = Material.Create("GC_BoundaryWall_" + s_iMaterialSerial.ToString(), "MatCommon", parameters);
		if (!m_Material)
			return false;
		vector origin = m_aPolygon[0];
		BaseWorld world = GetOwner().GetWorld();
		foreach (vector point : m_aPolygon)
		{
			point[1] = world.GetSurfaceY(point[0], point[2]) - 0.05;
			m_aGroundPoints.Insert(point);
		}
		EntitySpawnParams spawn = new EntitySpawnParams();
		spawn.TransformMode = ETransformMode.WORLD;
		spawn.Transform[3] = origin;
		m_Wall = GetGame().SpawnEntity(GenericEntity, world, spawn);
		if (!m_Wall)
			return false;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool UpdateNearbyMesh(vector position)
	{
		float uvs[4080];
		int indices[5760];
		int vertexCount, indexCount;
		vector origin = position;
		origin[1] = m_aPolygon[0][1];
		float radiusSq = m_fRange * m_fRange;
		for (int edge; edge < m_aGroundPoints.Count(); edge++)
		{
			vector first = m_aGroundPoints[edge];
			vector delta = m_aGroundPoints[(edge + 1) % m_aGroundPoints.Count()] - first;
			vector horizontal = delta;
			horizontal[1] = 0;
			vector relative = position - first;
			relative[1] = 0;
			float lengthSq = horizontal.LengthSq();
			if (lengthSq <= 0.0001)
				continue;

			float center = vector.Dot(relative, horizontal) / lengthSq;
			float perpendicularSq = Math.Max(0, relative.LengthSq() - center * center * lengthSq);
			if (perpendicularSq >= radiusSq)
				continue;

			float halfSpan = Math.Sqrt((radiusSq - perpendicularSq) / lengthSq);
			float start = Math.Max(0, center - halfSpan);
			float end = Math.Min(1, center + halfSpan);
			if (end <= start)
				continue;

			int edgeStart = vertexCount;
			for (int column = 0; column <= 16; column++)
			{
				vector point = first + delta * (start + (end - start) * column / 16.0);
				vector distance = point - position;
				distance[1] = 0;
				float fraction = Math.Clamp(1 - distance.Length() / m_fRange, 0, 1);
				float fade = fraction * fraction * (3 - 2 * fraction);
				m_aVertices[vertexCount] = point - origin;
				m_aVertices[vertexCount + 1] = point - origin + vector.Up * m_fHeight;
				// Sample the vanilla vertical gradient, avoiding its final wrap row.
				// The transparent top and distant columns both use V=0.
				uvs[vertexCount * 2] = 0.5;
				uvs[vertexCount * 2 + 1] = fade * 0.9;
				uvs[vertexCount * 2 + 2] = 0.5;
				uvs[vertexCount * 2 + 3] = 0;
				vertexCount += 2;
			}
			for (int strip = 0; strip < 16; strip++)
			{
				int vertex = edgeStart + strip * 2;
				indices[indexCount++] = vertex;
				indices[indexCount++] = vertex + 2;
				indices[indexCount++] = vertex + 1;
				indices[indexCount++] = vertex + 1;
				indices[indexCount++] = vertex + 2;
				indices[indexCount++] = vertex + 3;
			}
		}
		if (vertexCount == 0)
			return false;

		// In-place mesh edits retain stale rendered geometry in Workbench 1.8.
		// Replace only the clipped mesh, keeping the entity and material alive.
		string materialName;
		m_Material.GetName(materialName);
		int vertexCounts[1] = {vertexCount};
		int indexCounts[1] = {indexCount};
		string materials[1] = {materialName};
		Resource resource = MeshObject.Create(1, vertexCounts, indexCounts, materials, 0);
		if (!resource || !resource.GetResource())
		{
			m_bFailed = true;
			PrintFormat("GC | Boundary wall | mesh unavailable: %1", GetOwner().GetName(), level: LogLevel.ERROR);
			return false;
		}
		MeshObject mesh = resource.GetResource().ToMeshObject();
		mesh.UpdateVerts(0, m_aVertices, uvs);
		mesh.UpdateIndices(0, indices);
		m_Wall.SetObject(mesh, "");
		m_Mesh = resource;
		m_Wall.SetOrigin(origin);
		m_Wall.Update();
		m_iVisibleColumns = vertexCount / 2;
		m_vLastVisualPosition = position;
		m_bPositionValid = true;
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
			m_bFailed = !CreateWall();
			if (m_bFailed)
				PrintFormat("GC | Boundary wall | mesh unavailable: %1", owner.GetName(), level: LogLevel.ERROR);
		}
		if (!m_Wall || m_bFailed)
			return;
		float opacity = m_fMaximumOpacity * fraction * fraction * (3 - 2 * fraction);
		m_fOpacity = opacity;
		if (fraction <= 0)
			m_Wall.ClearFlags(EntityFlags.VISIBLE);
		else
		{
			if (!m_bPositionValid || vector.DistanceSq(position, m_vLastVisualPosition) > 0.01)
			{
				if (!UpdateNearbyMesh(position))
				{
					m_Wall.ClearFlags(EntityFlags.VISIBLE);
					return;
				}
			}
			m_Wall.SetFlags(EntityFlags.VISIBLE);
		}
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
