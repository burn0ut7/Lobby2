[ComponentEditorProps(category: "GC Lobby/Briefing", description: "Places a replicated GC Lobby briefing-map marker at this entity's transform")]
class GC_BriefingMarkerComponentClass : ScriptComponentClass
{
}

class GC_BriefingMarkerComponent : ScriptComponent
{
	protected const int MAX_REGISTRATION_ATTEMPTS = 20;
	protected const int REGISTRATION_RETRY_MS = 100;
	protected int m_iRegistrationAttempts;
	protected bool m_bRegistered;

	[Attribute("", UIWidgets.ResourcePickerThumbnail, "Texture or image-set resource used by the briefing marker", params: "imageset edds png")]
	protected ResourceName m_sImageSetOrImage;

	[Attribute("", UIWidgets.ResourcePickerThumbnail, "Optional highlight imageset using the same quad as the marker image", params: "imageset")]
	protected ResourceName m_sHighlightImageSet;

	[Attribute("", UIWidgets.EditBox, "Quad name used only when Image is an .imageset resource. Select the entity to preview its actual icon in the 3D viewport")]
	protected string m_sQuadName;

	[Attribute("1", UIWidgets.CheckBox, "Off: Size is screen pixels. On: Size is a world offset and changes with map zoom, matching PS_ManualMarker")]
	protected bool m_bWorldScale;

	[Attribute("5", UIWidgets.SpinBox, "Legacy marker size: pixels when World Scale is off, or the projected X/Z world offset when on", params: "0.01 100000 0.01")]
	protected float m_fSize;

	[Attribute("0", UIWidgets.SpinBox, "Minimum projected width and height in pixels. Zero disables the legacy minimum-size clamp", params: "0 10000 1")]
	protected float m_fMinSize;

	[Attribute("1 1 1 1", UIWidgets.ColorPicker, "Tint applied to the map marker and its Workbench direction arrow")]
	protected ref Color m_Color;

	[Attribute("", UIWidgets.EditBox, "Text displayed underneath the marker")]
	protected string m_sDescription;

	[Attribute("", UIWidgets.EditComboBox, "Authoritative base-game faction keys. Empty shows the marker to everyone", enums: {ParamEnum("", "", "gc-choices:1:GC_FactionChoices")})]
	protected ref array<string> m_aFactionKeys;

	[Attribute("0", UIWidgets.ComboBox, "Earliest game-mode phase from which this marker is shown", enumType: SCR_EGameModeState)]
	protected SCR_EGameModeState m_eShowAtGameModeState;

	[Attribute("0", UIWidgets.CheckBox, "Invert the phase threshold. Off: show from the selected phase onward. On: hide from the selected phase onward")]
	protected bool m_bInvertGameModeState;

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

#ifdef WORKBENCH
		// Embedded Play must not retain visualization state from the editing world.
		if (!SCR_Global.IsEditMode())
		{
			m_EditorPreview = null;
			m_DirectionArrow = null;
		}
#endif

		if (SCR_Global.IsEditMode() || !Replication.IsServer())
			return;

		GetGame().GetCallqueue().CallLater(RegisterWithManager, 0);
	}

	protected void RegisterWithManager()
	{
		m_iRegistrationAttempts++;
		GC_LobbyManager manager = GC_LobbyManager.GetManager();
		if (manager)
		{
			m_bRegistered = manager.RegisterBriefingMarker(this);
			return;
		}

		if (m_iRegistrationAttempts < MAX_REGISTRATION_ATTEMPTS)
			GetGame().GetCallqueue().CallLater(RegisterWithManager, REGISTRATION_RETRY_MS);
	}

	override void OnDelete(IEntity owner)
	{
#ifdef WORKBENCH
		m_EditorPreview = null;
#endif
		GetGame().GetCallqueue().Remove(RegisterWithManager);
		if (m_bRegistered && owner)
		{
			GC_LobbyManager manager = GC_LobbyManager.GetManager();
			if (manager)
				manager.RemoveBriefingMarker(owner.GetName());
		}

		m_bRegistered = false;
		super.OnDelete(owner);
	}

	GC_BriefingMarker CreateMarkerSnapshot()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return null;

		string markerName = owner.GetName();
		if (markerName.IsEmpty())
		{
			Print("GC | Briefing marker registration failed: the owning entity must have a name", LogLevel.WARNING);
			return null;
		}

		GC_BriefingMarker marker = new GC_BriefingMarker();
		int markerColor = Color.WHITE;
		if (m_Color)
			markerColor = m_Color.PackToInt();
		marker.Set(markerName, m_sImageSetOrImage, m_sHighlightImageSet, m_sQuadName, owner.GetOrigin(), owner.GetYawPitchRoll()[0] - 90,
			m_bWorldScale, m_fSize, m_fMinSize, m_sDescription, m_aFactionKeys,
			m_eShowAtGameModeState, m_bInvertGameModeState, true, markerColor);
		return marker;
	}

#ifdef WORKBENCH
	protected ref Shape m_DirectionArrow;
	protected int m_iDirectionArrowColor;
	protected ref GC_MapMarkerPreview m_EditorPreview;

	//------------------------------------------------------------------------------------------------
	override void _WB_SetTransform(IEntity owner, inout vector mat[4], IEntitySource src)
	{
		if (!owner || !SCR_Global.IsEditMode())
			return;

		// Constrain this editor transform without scheduling a second move.
		vector position = mat[3];
		position[1] = owner.GetWorld().GetSurfaceY(position[0], position[2]);
		mat[3] = position;
	}

	override int _WB_GetAfterWorldUpdateSpecs(IEntity owner, IEntitySource src)
	{
		return EEntityFrameUpdateSpecs.CALL_ALWAYS;
	}

	override event void _WB_AfterWorldUpdate(IEntity owner, float timeSlice)
	{
		if (!owner || !SCR_Global.IsEditMode())
		{
			m_DirectionArrow = null;
			m_EditorPreview = null;
			return;
		}

		int arrowColor = Color.WHITE;
		if (m_Color)
			arrowColor = m_Color.PackToInt();

		if (!m_DirectionArrow || arrowColor != m_iDirectionArrowColor)
		{
			m_DirectionArrow = null;
			m_DirectionArrow = Shape.CreateArrow("0 0.15 0", "2 0.15 0", 0.35, arrowColor, 0, 4);
			m_iDirectionArrowColor = arrowColor;
		}

		vector transform[4];
		// Map markers use yaw only, regardless of entity pitch, roll or scale.
		Math3D.AnglesToMatrix(Vector(owner.GetYawPitchRoll()[0], 0, 0), transform);
		transform[3] = owner.GetOrigin();
		if (m_DirectionArrow)
			m_DirectionArrow.SetMatrix(transform);

		GenericEntity entity = GenericEntity.Cast(owner);
		WorldEditorAPI api;
		if (entity)
			api = entity._WB_GetEditorAPI();
		if (!api || !api.IsEntitySelected(api.EntityToSource(owner)))
		{
			m_EditorPreview = null;
			return;
		}

		if (!m_EditorPreview)
			m_EditorPreview = new GC_MapMarkerPreview();
		float previewSize = 5;
		if (m_bWorldScale)
			previewSize = m_fSize;
		m_EditorPreview.Update(owner, m_sImageSetOrImage, m_sQuadName, arrowColor, previewSize);
	}
#endif

}
