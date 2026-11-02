//! Observes native contacts without replacing damage, sound or physics effects.
class GC_TelemetryImpactEffect : BaseProjectileEffect
{
	//------------------------------------------------------------------------------------------------
	override void OnEffect(IEntity pHitEntity, inout vector outMat[3], IEntity damageSource, notnull Instigator instigator, string colliderName, float speed)
	{
		super.OnEffect(pHitEntity, outMat, damageSource, instigator, colliderName, speed);
		if (!Replication.IsServer())
			return;
		GC_SessionLogComponent logs = GC_SessionLogComponent.GetInstance();
		if (logs)
			logs.RecordProjectileImpact(damageSource, pHitEntity, outMat[0], outMat[2]);
	}
}
