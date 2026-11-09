[EntityEditorProps(category: "Lobby2/Mission", description: "Radius Area: a passive spherical boundary for Area Condition. Set its name and radius. It does not scan entities, set flags or run actions.")]
class GC_RadiusAreaEntityClass : GenericEntityClass
{
}

class GC_RadiusAreaEntity : GenericEntity
{
	[Attribute("10", UIWidgets.SpinBox, "Distance in metres from the area's centre. Includes height: this is a sphere, not an unlimited-height circle. The boundary itself counts as inside. Entity scaling does not change this metre value.", "0.1 100000 0.1")]
	protected float m_fRadius;

	//------------------------------------------------------------------------------------------------
	float GetRadius()
	{
		return m_fRadius;
	}

#ifdef WORKBENCH
	//------------------------------------------------------------------------------------------------
	override int _WB_GetAfterWorldUpdateSpecs(IEntitySource src)
	{
		return EEntityFrameUpdateSpecs.CALL_ALWAYS;
	}

	//------------------------------------------------------------------------------------------------
	override void _WB_AfterWorldUpdate(float timeSlice)
	{
		super._WB_AfterWorldUpdate(timeSlice);
		if (m_fRadius > 0 && SCR_Global.IsEditMode())
			Shape.CreateSphere(0x663399FF, ShapeFlags.TRANSP | ShapeFlags.WIREFRAME | ShapeFlags.NOZWRITE | ShapeFlags.ONCE, GetOrigin(), m_fRadius);
	}
#endif
}
