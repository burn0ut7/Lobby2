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
	protected ref map<string, IEntity> m_mGC_PhotoSources;
	protected ref map<string, IEntity> m_mGC_PhotoBatch;
	protected ref array<IEntity> m_aGC_PhotoClones;
	protected string m_sGC_PhotoProbe;
	protected bool m_bGC_ObjectExportPending;

	//------------------------------------------------------------------------------------------------
	//! Large worlds exceed the synchronous runtime-call deadline. Publish a receipt.
	string GC_TestBuildingExportStart()
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!Replication.IsServer() || GetManager() != this || !player || !AuthorizeAdminAction(player.GetPlayerId(), "building-export-start", m_sGC_ExportName))
			return "denied";
		if (!GC_MapExportReady() || m_bGC_ObjectExportPending || m_GC_PhotoCamera)
			return "not-ready";
		FileHandle status = FileIO.OpenFile("$profile:GC_MapBuildings_" + m_sGC_ExportName + ".status", FileMode.WRITE);
		if (!status)
			return "open-failed";
		status.WriteLine("running");
		status.Close();
		m_bGC_ObjectExportPending = true;
		GetGame().GetCallqueue().CallLater(GC_TestBuildingExportDeferred, 500, false);
		return "started";
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TestBuildingExportDeferred()
	{
		string result = GC_TestBuildingExport();
		FileHandle status = FileIO.OpenFile("$profile:GC_MapBuildings_" + m_sGC_ExportName + ".status", FileMode.WRITE);
		if (status)
		{
			status.WriteLine(result);
			status.Close();
		}
		m_bGC_ObjectExportPending = false;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestBuildingProbe(vector position)
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!Replication.IsServer() || GetManager() != this || !player || !AuthorizeAdminAction(player.GetPlayerId(), "building-probe", position.ToString()))
			return "denied";
		m_sGC_PhotoProbe = "";
		GetGame().GetWorld().QueryEntitiesByAABB(Vector(position[0] - 5, -200, position[2] - 5), Vector(position[0] + 5, 2000, position[2] + 5), GC_TestBuildingProbeEntity);
		return m_sGC_PhotoProbe;
	}

	//------------------------------------------------------------------------------------------------
	protected bool GC_TestBuildingProbeEntity(IEntity entity)
	{
		string resource;
		if (entity.GetPrefabData())
			resource = entity.GetPrefabData().GetPrefabName();
		if (resource.IsEmpty() && entity.GetVObject())
			resource = entity.GetVObject().GetResourceName();
		if (resource.Contains("Vegetation"))
			return true;
		vector mins, maxs;
		entity.GetWorldBounds(mins, maxs);
		MapDescriptorComponent descriptor = MapDescriptorComponent.Cast(entity.FindComponent(MapDescriptorComponent));
		int type = -1;
		if (descriptor)
			type = descriptor.GetBaseType();
		m_sGC_PhotoProbe += string.Format("%1 | %2 | %3 | %4 | %5\n", entity.Type(), type, mins, maxs, resource);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestBuildingExport()
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!Replication.IsServer() || GetManager() != this || !player || !AuthorizeAdminAction(player.GetPlayerId(), "building-export", m_sGC_ExportName))
			return "denied";
		if (!GC_MapExportReady())
			return "unsupported-world";
		if (m_GC_PhotoCamera)
			return "preview-active";
		m_aGC_BuildingRows = {};
		m_aGC_BuildingResources = {};
		m_mGC_PhotoSources = new map<string, IEntity>();
		vector mins, maxs;
		GetGame().GetWorld().GetBoundBox(mins, maxs);
		if (!GetGame().GetWorld().QueryEntitiesByAABB(mins, maxs, GC_TestBuildingEntity))
			return "incomplete-query";
		FileHandle output = FileIO.OpenFile("$profile:GC_MapBuildings_" + m_sGC_ExportName + ".tsv", FileMode.WRITE);
		if (!output)
			return "open-failed";
		output.WriteLine("minX\tminZ\tmaxX\tmaxZ\tresource");
		foreach (string row : m_aGC_BuildingRows)
			output.WriteLine(row);
		output.Close();
		string result = "exported=" + m_aGC_BuildingRows.Count();
		m_aGC_BuildingRows = null;
		GC_AdminActionLogger.Log(player.GetPlayerId(), "building-export", m_sGC_ExportName, result);
		return result;
	}

	//------------------------------------------------------------------------------------------------
	protected bool GC_TestBuildingEntity(IEntity entity)
	{
		if (!entity)
			return true;
		string resource;
		if (entity.GetPrefabData())
			resource = entity.GetPrefabData().GetPrefabName();
		if (resource.IsEmpty() && entity.GetVObject())
			resource = entity.GetVObject().GetResourceName();
		if (!resource.IsEmpty() && !m_mGC_PhotoSources.Contains(resource))
			m_mGC_PhotoSources.Insert(resource, entity);
		// Native map footprints also include generic meshes without descriptors.
		// Export source identity for every visual candidate; the map owns inclusion.
		if (resource.IsEmpty())
			return true;
		vector mins, maxs;
		entity.GetWorldBounds(mins, maxs);
		m_aGC_BuildingRows.Insert(string.Format("%1\t%2\t%3\t%4\t%5", mins[0], mins[2], maxs[0], maxs[2], resource));
		m_aGC_BuildingResources.Insert(resource);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestBuildingPhoto(int index)
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!Replication.IsServer() || GetManager() != this || !player || !AuthorizeAdminAction(player.GetPlayerId(), "building-photo", index.ToString()))
			return "denied";
		if (!GC_MapExportReady())
			return "unsupported-world";
		if (!m_aGC_BuildingResources || index < 0 || index >= m_aGC_BuildingResources.Count() || m_aGC_BuildingResources[index].IsEmpty())
			return "invalid-index";
		string resource = m_aGC_BuildingResources[index];
		if (!m_mGC_PhotoBatch || !m_mGC_PhotoBatch.Contains(resource))
		{
			string result = GC_TestBuildingPhotoBatch(resource);
			if (result != "prepared=1")
				return result;
		}
		m_GC_PhotoBuilding = m_mGC_PhotoBatch.Get(resource);
		if (!m_GC_PhotoBuilding || !m_GC_PhotoBuilding.GetChildren())
			return "no-visual";
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
		mins = "1e20 1e20 1e20";
		maxs = "-1e20 -1e20 -1e20";
		GC_TestBuildingPhotoBounds(m_GC_PhotoBuilding, mins, maxs);
		vector center = (mins + maxs) * 0.5;
		float distance = Math.Max(4, vector.Distance(mins, maxs) * 1.9);
		vector position = center + Vector(-0.7, 0.3, 0.7) * distance;
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
	//! Select a source resource already resolved by the active world export.
	string GC_TestBuildingPhotoResource(string resource)
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!Replication.IsServer() || GetManager() != this || !player || !AuthorizeAdminAction(player.GetPlayerId(), "building-photo-resource", resource))
			return "denied";
		if (!GC_MapExportReady())
			return "unsupported-world";
		if (!m_aGC_BuildingResources)
			m_aGC_BuildingResources = {};
		int index = m_aGC_BuildingResources.Find(resource);
		if (index < 0)
			index = m_aGC_BuildingResources.Insert(resource);
		string result = GC_TestBuildingPhoto(index);
		if (result != "positioned")
			return result;
		return "index=" + index;
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
		GC_AdminActionLogger.Log(player.GetPlayerId(), "building-photo-end", "preview", "restored");
		return "restored";
	}
	//------------------------------------------------------------------------------------------------
	//! Snapshot only render objects, not simulation scripts or editor-only splines.
	string GC_TestBuildingPhotoBatch(string resources)
	{
		PlayerController player = GetGame().GetPlayerController();
		if (!Replication.IsServer() || GetManager() != this || !player || !AuthorizeAdminAction(player.GetPlayerId(), "building-photo-batch", "preview"))
			return "denied";
		if (!GC_MapExportReady() || !m_mGC_PhotoSources)
			return "export-buildings-first";
		array<string> requested = {};
		resources.Split("\n", requested, true);
		if (requested.IsEmpty() || requested.Count() > 20)
			return "invalid-batch";
		foreach (string resource : requested)
		{
			if (!m_mGC_PhotoSources.Get(resource))
				return "source-unavailable:" + resource;
		}
		GC_TestBuildingPhotoClear();
		m_mGC_PhotoBatch = new map<string, IEntity>();
		m_aGC_PhotoClones = {};
		int slot;
		foreach (string resource : requested)
		{
			IEntity source = m_mGC_PhotoSources.Get(resource);
			EntitySpawnParams parameters = new EntitySpawnParams();
			parameters.TransformMode = ETransformMode.WORLD;
			// A single row perpendicular to the camera: no models behind the subject.
			parameters.Transform[3] = m_vGC_ExportPhoto + Vector(slot * 1024, 0, slot * 1024);
			IEntity root = GetGame().SpawnEntity(GenericEntity, GetGame().GetWorld(), parameters);
			m_aGC_PhotoClones.Insert(root);
			vector original[4];
			source.GetTransform(original);
			GC_TestBuildingPhotoClone(source, root, original);
			m_mGC_PhotoBatch.Insert(resource, root);
			slot++;
		}
		// Size spacing from the largest render bounds, including custom-map models.
		float spacing = 1024;
		foreach (string resource : requested)
		{
			vector low = "1e20 1e20 1e20";
			vector high = "-1e20 -1e20 -1e20";
			IEntity preview = m_mGC_PhotoBatch.Get(resource);
			if (!preview.GetChildren())
				continue;
			GC_TestBuildingPhotoBounds(preview, low, high);
			spacing = Math.Max(spacing, vector.Distance(low, high) * 8);
		}
		slot = 0;
		foreach (string resource : requested)
		{
			IEntity preview = m_mGC_PhotoBatch.Get(resource);
			if (preview.GetChildren())
			{
				vector low = "1e20 1e20 1e20";
				vector high = "-1e20 -1e20 -1e20";
				GC_TestBuildingPhotoBounds(preview, low, high);
				vector center = (low + high) * 0.5;
				vector target = m_vGC_ExportPhoto + Vector(slot * spacing, 0, slot * spacing);
				preview.SetOrigin(preview.GetOrigin() + target - center);
			}
			slot++;
		}
		GC_AdminActionLogger.Log(player.GetPlayerId(), "building-photo-batch", "preview", "prepared=" + slot);
		return "prepared=" + slot;
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TestBuildingPhotoClone(IEntity source, IEntity root, vector original[4])
	{
		VObject visual = source.GetVObject();
		// Runtime render objects can be streamed aggregates or omit the exterior.
		// Resolve the authored mesh, as SCR_PrefabPreviewEntity does.
		Resource meshResource;
		if (source.GetPrefabData())
		{
			Resource prefab = Resource.Load(source.GetPrefabData().GetPrefabName());
			IEntitySource definition = SCR_BaseContainerTools.FindEntitySource(prefab);
			if (definition)
			{
				for (int i; i < definition.GetComponentCount(); i++)
				{
					IEntityComponentSource component = definition.GetComponent(i);
					if (component.GetClassName() != "MeshObject")
						continue;
					ResourceName mesh;
					component.Get("Object", mesh);
					if (mesh.IsEmpty())
						continue;
					meshResource = Resource.Load(mesh);
					if (meshResource && meshResource.GetResource())
						visual = meshResource.GetResource().ToVObject();
				}
			}
		}
		if (visual)
		{
			vector transform[4];
			source.GetTransform(transform);
			EntitySpawnParams parameters = new EntitySpawnParams();
			vector relative[4], rootTransform[4];
			Math3D.MatrixInvMultiply4(original, transform, relative);
			root.GetTransform(rootTransform);
			Math3D.MatrixMultiply4(rootTransform, relative, parameters.Transform);
			parameters.TransformMode = ETransformMode.WORLD;
			IEntity clone = GetGame().SpawnEntity(GenericEntity, GetGame().GetWorld(), parameters);
			root.AddChild(clone, -1, EAddChildFlags.RECALC_LOCAL_TRANSFORM);
			clone.SetObject(visual, "");
			clone.SetFixedLOD(0);
			m_aGC_PhotoClones.Insert(clone);
		}
		IEntity child = source.GetChildren();
		while (child)
		{
			GC_TestBuildingPhotoClone(child, root, original);
			child = child.GetSibling();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TestBuildingPhotoBounds(IEntity root, out vector mins, out vector maxs)
	{
		IEntity child = root.GetChildren();
		while (child)
		{
			vector low, high;
			child.GetWorldBounds(low, high);
			for (int axis; axis < 3; axis++)
			{
				mins[axis] = Math.Min(mins[axis], low[axis]);
				maxs[axis] = Math.Max(maxs[axis], high[axis]);
			}
			child = child.GetSibling();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TestBuildingPhotoClear()
	{
		if (m_aGC_PhotoClones)
		{
			for (int i = m_aGC_PhotoClones.Count() - 1; i >= 0; i--)
			{
				if (m_aGC_PhotoClones[i])
					delete m_aGC_PhotoClones[i];
			}
		}
		m_aGC_PhotoClones = null;
		m_mGC_PhotoBatch = null;
		m_GC_PhotoBuilding = null;
	}

}
#endif
