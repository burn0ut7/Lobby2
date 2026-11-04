[ComponentEditorProps(category: "Lobby2/Mission", description: "Legacy metadata component. Use GC > Create Mission Data in World Editor.")]
class GC_MissionMetadataScannerComponentClass : ScriptComponentClass
{
}

//! Retained so existing mission prefabs load without losing serialized data.
//! Metadata collection is owned by the explicit save action, never editor property callbacks.
class GC_MissionMetadataScannerComponent : ScriptComponent
{
	[Attribute("1", UIWidgets.Hidden)]
	protected bool m_bEnabled;
}
