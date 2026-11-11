#ifdef WORKBENCH
//! Union of the vehicle action's supported operations, including non-vehicle repair.
class GC_VehicleTargetChoices : GC_EntityReferenceChoices
{
	//------------------------------------------------------------------------------------------------
	override protected bool AcceptEntity(IEntity entity)
	{
		if (entity.FindComponent(SCR_DamageManagerComponent))
			return true;

		array<SCR_FuelManagerComponent> fuelManagers = {};
		SCR_FuelManagerComponent.GetAllFuelManagers(entity, fuelManagers);
		if (!fuelManagers.IsEmpty())
			return true;

		SCR_ResourceComponent resources = SCR_ResourceComponent.FindResourceComponent(entity, false);
		SCR_ResourceContainer container;
		return resources && resources.GetContainer(EResourceType.SUPPLIES, container);
	}
}
#endif
