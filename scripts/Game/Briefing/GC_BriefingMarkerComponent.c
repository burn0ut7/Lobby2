[ComponentEditorProps(category: "Lobby2/Briefing", description: "Places a replicated Lobby2 briefing-map marker at this entity's transform")]
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

	[Attribute("", UIWidgets.EditBox, "Quad name used only when Image is an .imageset resource. Workbench does not expose image-set quads to scripted dropdowns")]
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

	[Attribute("", UIWidgets.Object, "Authoritative base-game faction keys. Empty shows the marker to everyone")]
	protected ref array<string> m_aFactionKeys;

	[Attribute("0", UIWidgets.ComboBox, "Earliest game-mode phase from which this marker is shown", enumType: SCR_EGameModeState)]
	protected SCR_EGameModeState m_eShowAtGameModeState;

	[Attribute("0", UIWidgets.CheckBox, "Invert the phase threshold. Off: show from the selected phase onward. On: hide from the selected phase onward")]
	protected bool m_bInvertGameModeState;

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

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

	override int _WB_GetAfterWorldUpdateSpecs(IEntity owner, IEntitySource src)
	{
		return EEntityFrameUpdateSpecs.CALL_WHEN_ENTITY_VISIBLE;
	}

	override event void _WB_AfterWorldUpdate(IEntity owner, float timeSlice)
	{
		if (!owner)
			return;

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
		owner.GetTransform(transform);
		if (m_DirectionArrow)
			m_DirectionArrow.SetMatrix(transform);
	}
#endif

}
