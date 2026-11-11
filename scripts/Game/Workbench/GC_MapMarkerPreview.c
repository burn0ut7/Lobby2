#ifdef WORKBENCH
//! Selected-marker preview. Temporary geometry is never added to the editor source.
class GC_MapMarkerPreview
{
	protected IEntity m_Entity;
	protected ref Resource m_Mesh;
	protected ref Material m_Material;
	protected ResourceName m_sImage;
	protected string m_sQuad;
	protected int m_iColor;
	protected bool m_bAttempted;
	protected float m_fAspect = 1;
	protected bool m_bTransformSet;
	protected vector m_vPosition;
	protected float m_fYaw;
	protected float m_fSize;
	protected const float PREVIEW_OFFSET = 0.15;
	protected static int s_iMaterialId;

	//------------------------------------------------------------------------------------------------
	void Update(IEntity owner, ResourceName image, string quad, int color, float size)
	{
		if (!SCR_Global.IsEditMode())
		{
			Clear();
			return;
		}
		if (!m_bAttempted || image != m_sImage || quad != m_sQuad || color != m_iColor)
		{
			Clear();
			m_sImage = image;
			m_sQuad = quad;
			m_iColor = color;
			m_bAttempted = true;
			if (!Create(owner))
				PrintFormat("GC | Cannot preview map marker: image=%1 quad=%2", image, quad, level: LogLevel.WARNING);
		}
		if (!m_Entity)
			return;

		vector position = owner.GetOrigin();
		float yaw = owner.GetYawPitchRoll()[0] - 90;
		size = Math.Max(0.01, size);
		if (m_bTransformSet && position == m_vPosition && yaw == m_fYaw && size == m_fSize)
			return;
		m_vPosition = position;
		m_fYaw = yaw;
		m_fSize = size;
		m_bTransformSet = true;

		vector transform[4];
		Math3D.AnglesToMatrix(Vector(yaw, 0, 0), transform);
		transform[3] = position + Vector(0, PREVIEW_OFFSET, 0);
		transform[0] = transform[0] * size;
		transform[1] = transform[1] * size;
		transform[2] = transform[2] * size;
		m_Entity.SetTransform(transform);
		m_Entity.Update();
	}

	//------------------------------------------------------------------------------------------------
	protected bool Create(IEntity owner)
	{
		if (m_sImage.IsEmpty())
			return false;
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return false;

		ResourceName texture = m_sImage;
		float uv[8] = {0, 0, 1, 0, 1, 1, 0, 1};
		if (m_sImage.EndsWith(".imageset"))
		{
			// Let the engine resolve the named quad instead of duplicating atlas coordinates.
			CanvasWidget canvas = CanvasWidget.Cast(workspace.CreateWidget(WidgetType.CanvasWidgetTypeID, 0, null, 0));
			if (!canvas)
				return false;
			ImageDrawCommand command = canvas.CreateCommandFromImageSet(m_sImage, m_sQuad, "1 1 0");
			canvas.RemoveFromHierarchy();
			if (!command || !command.m_pTexture || !command.m_pTexture.IsValid())
				return false;
			uv[0] = command.m_fUV[0];
			uv[1] = command.m_fUV[1];
			uv[2] = command.m_fUV[2];
			uv[3] = command.m_fUV[1];
			uv[4] = command.m_fUV[2];
			uv[5] = command.m_fUV[3];
			uv[6] = command.m_fUV[0];
			uv[7] = command.m_fUV[3];

			Resource imageSet = BaseContainerTools.LoadContainer(m_sImage);
			if (!imageSet || !imageSet.GetResource())
				return false;
			BaseContainer container = imageSet.GetResource().ToBaseContainer();
			if (!container)
				return false;
			BaseContainerList textures = container.GetObjectArray("Textures");
			if (!textures || textures.Count() == 0 || !textures.Get(0).Get("path", texture))
				return false;
		}

		ImageWidget image = ImageWidget.Cast(workspace.CreateWidget(WidgetType.ImageWidgetTypeID, 0, null, 0));
		if (!image)
			return false;
		bool loaded;
		if (m_sImage.EndsWith(".imageset"))
			loaded = image.LoadImageFromSet(0, m_sImage, m_sQuad);
		else
			loaded = image.LoadImageTexture(0, m_sImage);
		int width, height;
		image.GetImageSize(0, width, height);
		image.RemoveFromHierarchy();
		if (!loaded || width <= 0 || height <= 0)
			return false;
		m_fAspect = (float)height / width;

		map<string, string> parameters = new map<string, string>();
		parameters.Insert("AlbedoMap", texture);
		Color tint = Color.FromInt(m_iColor);
		parameters.Insert("Color", string.Format("%1 %2 %3 %4", tint.R(), tint.G(), tint.B(), tint.A()));
		parameters.Insert("Cull", "none");
		parameters.Insert("CastShadow", "0");
		parameters.Insert("ReceiveShadow", "0");
		parameters.Insert("BlendMode", "AlphaBlend");
		parameters.Insert("ZWrite", "0");
		// Match the editor direction indicator: sorting alone still tests terrain depth.
		parameters.Insert("ZEnable", "0");
		parameters.Insert("Sort", "overall");
		parameters.Insert("DisablePrexposure", "1");
		s_iMaterialId++;
		string materialName = string.Format("GC_MapMarkerPreview_%1.emat", s_iMaterialId);
		m_Material = Material.Create(materialName, "MatCommon", parameters);
		if (!m_Material)
			return false;
		m_Material.GetName(materialName);

		int vertexCounts[] = {4};
		int indexCounts[] = {6};
		string materials[] = {materialName};
		m_Mesh = MeshObject.Create(1, vertexCounts, indexCounts, materials, 0);
		if (!m_Mesh || !m_Mesh.GetResource())
			return false;
		MeshObject mesh = m_Mesh.GetResource().ToMeshObject();
		if (!mesh)
			return false;
		float halfHeight = m_fAspect * 0.5;
		vector vertices[4] = {Vector(-0.5, 0, halfHeight), Vector(0.5, 0, halfHeight), Vector(0.5, 0, -halfHeight), Vector(-0.5, 0, -halfHeight)};
		int indices[6] = {0, 1, 2, 0, 2, 3};
		mesh.UpdateVerts(0, vertices, uv);
		mesh.UpdateIndices(0, indices);
		m_Entity = GetGame().SpawnEntity(GenericEntity, owner.GetWorld());
		if (!m_Entity)
			return false;
		m_Entity.SetObject(mesh, "");
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void Clear()
	{
		if (m_Entity && !m_Entity.IsDeleted())
			delete m_Entity;
		m_Entity = null;
		m_Mesh = null;
		m_Material = null;
		m_bTransformSet = false;
	}

	//------------------------------------------------------------------------------------------------
	void ~GC_MapMarkerPreview()
	{
		Clear();
	}
}
#endif
