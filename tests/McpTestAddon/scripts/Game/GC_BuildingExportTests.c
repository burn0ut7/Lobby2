#ifdef WORKBENCH
//! Explicit static building identity export for offline map inspection.
modded class GC_LobbyManager
{
	protected ref array<string> m_aGC_BuildingRows;
	protected ref array<string> m_aGC_BuildingResources;
	protected CameraBase m_GC_PhotoCamera;
	protected CameraBase m_GC_PreviousCamera;
	protected IEntity m_GC_PhotoBuilding;
	protected int m_iGC_PhotoIndex = -1;
	protected ref array<IEntity> m_aGC_PhotoOriginal;
	protected ref array<IEntity> m_aGC_PhotoNearby;

	//------------------------------------------------------------------------------------------------
	string GC_TestBuildingExport()
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!Replication.IsServer() || GetManager() != this || !player || !AuthorizeAdminAction(player.GetPlayerId(), "building-export", "Arland"))
			return "denied";
		string world = GetGame().GetWorldFile();
		world.ToLower();
		if (!world.Contains("worlds/examples/arland/arland_lobby.ent"))
			return "unsupported-world";
		if (m_GC_PhotoCamera)
			return "preview-active";
		m_aGC_BuildingRows = {};
		m_aGC_BuildingResources = {};
		vector mins, maxs;
		GetGame().GetWorld().GetBoundBox(mins, maxs);
		if (!GetGame().GetWorld().QueryEntitiesByAABB(mins, maxs, GC_TestBuildingEntity))
			return "incomplete-query";
		FileHandle output = FileIO.OpenFile("$profile:GC_MapBuildings_Arland.tsv", FileMode.WRITE);
		if (!output)
			return "open-failed";
		output.WriteLine("minX\tminZ\tmaxX\tmaxZ\tresource");
		foreach (string row : m_aGC_BuildingRows)
			output.WriteLine(row);
		output.Close();
		string result = "exported=" + m_aGC_BuildingRows.Count();
		m_aGC_BuildingRows = null;
		GC_AdminActionLogger.Log(player.GetPlayerId(), "building-export", "Arland", result);
		return result;
	}

	//------------------------------------------------------------------------------------------------
	protected bool GC_TestBuildingEntity(IEntity entity)
	{
		if (!entity || !entity.GetPrefabData())
			return true;
		MapDescriptorComponent descriptor = MapDescriptorComponent.Cast(entity.FindComponent(MapDescriptorComponent));
		if (!SCR_DestructibleBuildingEntity.Cast(entity) && (!descriptor || descriptor.GetBaseType() != EMapDescriptorType.MDT_BUILDING))
			return true;
		vector mins, maxs;
		entity.GetWorldBounds(mins, maxs);
		m_aGC_BuildingRows.Insert(string.Format("%1\t%2\t%3\t%4\t%5", mins[0], mins[2], maxs[0], maxs[2], entity.GetPrefabData().GetPrefabName()));
		m_aGC_BuildingResources.Insert(entity.GetPrefabData().GetPrefabName());
		return true;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestBuildingPhoto(int index)
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!Replication.IsServer() || GetManager() != this || !player || !AuthorizeAdminAction(player.GetPlayerId(), "building-photo", index.ToString()))
			return "denied";
		string world = GetGame().GetWorldFile();
		world.ToLower();
		if (!world.Contains("worlds/examples/arland/arland_lobby.ent"))
			return "unsupported-world";
		if (!m_aGC_BuildingResources || index < 0 || index >= m_aGC_BuildingResources.Count() || m_aGC_BuildingResources[index].IsEmpty())
			return "invalid-index";
		GC_TestBuildingPhotoClear();
		EntitySpawnParams parameters = new EntitySpawnParams();
		parameters.TransformMode = ETransformMode.WORLD;
		parameters.Transform[3] = "2048 1000 2048";
		m_GC_PhotoBuilding = GetGame().SpawnEntityPrefab(Resource.Load(m_aGC_BuildingResources[index]), GetGame().GetWorld(), parameters);
		if (!m_GC_PhotoBuilding)
			return "spawn-failed";
		if (!m_GC_PhotoCamera)
		{
			m_GC_PreviousCamera = GetGame().GetCameraManager().CurrentCamera();
			m_GC_PhotoCamera = CameraBase.Cast(GetGame().SpawnEntity(CameraBase, GetGame().GetWorld()));
		}
		if (!m_GC_PhotoCamera)
		{
			GC_TestBuildingPhotoClear();
			GC_AdminActionLogger.Log(player.GetPlayerId(), "building-photo", index.ToString(), "camera-failed");
			return "camera-failed";
		}
		vector mins, maxs;
		m_GC_PhotoBuilding.GetWorldBounds(mins, maxs);
		vector center = (mins + maxs) * 0.5;
		float distance = Math.Max(4, vector.Distance(mins, maxs) * 1.9);
		vector position = center + Vector(0.7, 0.3, -0.7) * distance;
		vector transform[4];
		Math3D.DirectionAndUpMatrix((center - position).Normalized(), "0 1 0", transform);
		transform[3] = position;
		m_GC_PhotoCamera.SetTransform(transform);
		m_GC_PhotoCamera.SetFOVDegree(40);
		GetGame().GetCameraManager().SetCamera(m_GC_PhotoCamera);
		m_GC_PhotoCamera.ApplyTransform(0);
		m_iGC_PhotoIndex = index;
		GC_AdminActionLogger.Log(player.GetPlayerId(), "building-photo", index.ToString(), "positioned");
		return "positioned";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestBuildingPhotoSave(int index)
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!Replication.IsServer() || GetManager() != this || !player || !AuthorizeAdminAction(player.GetPlayerId(), "building-photo-save", index.ToString()))
			return "denied";
		if (index != m_iGC_PhotoIndex || !m_GC_PhotoBuilding || !m_GC_PhotoCamera || GetGame().GetCameraManager().CurrentCamera() != m_GC_PhotoCamera)
			return "no-preview";
		FileHandle provenance = FileIO.OpenFile("$profile:GC_BuildingPhoto_" + index + ".resource", FileMode.WRITE);
		if (!provenance)
			return "provenance-failed";
		provenance.WriteLine(m_aGC_BuildingResources[index]);
		provenance.Close();
		bool saved = System.MakeScreenshot("$profile:GC_BuildingPhoto_" + index);
		GC_AdminActionLogger.Log(player.GetPlayerId(), "building-photo-save", index.ToString(), saved.ToString());
		return "saved=" + saved;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestBuildingPhotoEnd()
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!Replication.IsServer() || GetManager() != this || !player || !AuthorizeAdminAction(player.GetPlayerId(), "building-photo-end", "preview"))
			return "denied";
		if (m_GC_PreviousCamera)
			GetGame().GetCameraManager().SetCamera(m_GC_PreviousCamera);
		GC_TestBuildingPhotoClear();
		if (m_GC_PhotoCamera)
			delete m_GC_PhotoCamera;
		m_GC_PhotoBuilding = null;
		m_GC_PhotoCamera = null;
		m_GC_PreviousCamera = null;
		m_iGC_PhotoIndex = -1;
		m_aGC_PhotoOriginal = null;
		m_aGC_PhotoNearby = null;
		GC_AdminActionLogger.Log(player.GetPlayerId(), "building-photo-end", "preview", "restored");
		return "restored";
	}
	//------------------------------------------------------------------------------------------------
	//! Match the game's image generator: remove all spawned nearby attachments too.
	protected void GC_TestBuildingPhotoClear()
	{
		m_aGC_PhotoNearby = {};
		GetGame().GetWorld().QueryEntitiesByAABB("1792 744 1792", "2304 1256 2304", GC_TestBuildingPhotoCollect);
		if (!m_aGC_PhotoOriginal)
		{
			m_aGC_PhotoOriginal = m_aGC_PhotoNearby;
			return;
		}
		foreach (IEntity entity : m_aGC_PhotoNearby)
		{
			if (entity && entity != m_GC_PhotoCamera && !m_aGC_PhotoOriginal.Contains(entity))
				delete entity;
		}
		m_GC_PhotoBuilding = null;
	}

	//------------------------------------------------------------------------------------------------
	protected bool GC_TestBuildingPhotoCollect(IEntity entity)
	{
		m_aGC_PhotoNearby.Insert(entity);
		return true;
	}

}
#endif
