//! Native firing callbacks cover AI, unlike the player-only character event.
//! Preserve inherited effects in every mode; the stats owner checks active authority.
modded class SCR_MuzzleEffectComponent
{
	override void OnFired(IEntity effectEntity, BaseMuzzleComponent muzzle, IEntity projectileEntity)
	{
		super.OnFired(effectEntity, muzzle, projectileEntity);
		GC_MissionStatsComponent stats = GC_MissionStatsComponent.GetInstance();
		if (stats)
			stats.RecordAIFired(muzzle, projectileEntity);
	}
}

modded class SCR_OptimizedMuzzleEffectComponent
{
	override void OnFired(IEntity effectEntity, BaseMuzzleComponent muzzle, IEntity projectileEntity)
	{
		super.OnFired(effectEntity, muzzle, projectileEntity);
		GC_MissionStatsComponent stats = GC_MissionStatsComponent.GetInstance();
		if (stats)
			stats.RecordAIFired(muzzle, projectileEntity);
	}
}
