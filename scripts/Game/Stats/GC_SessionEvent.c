//! Immutable authority record. Damage callbacks are never coalesced.
class GC_SessionEvent
{
	int m_iSequence;
	float m_fSeconds;
	string m_sType;
	string m_sActor;
	string m_sActorName;
	string m_sTarget;
	string m_sTargetName;
	string m_sActorKind;
	string m_sTargetKind;
	string m_sActorFaction;
	string m_sTargetFaction;
	string m_sActorPrefab;
	string m_sTargetPrefab;
	string m_sActorEntity;
	string m_sTargetEntity;
	ref array<float> m_aActorPosition = {};
	ref array<float> m_aTargetPosition = {};
	string m_sWeapon;
	string m_sProjectile;
	string m_sMagazine;
	string m_sDetail;
	float m_fDamage;
	int m_iDamageType = -1;
	int m_iRelation;
	bool m_bFriendly;
	float m_fDistance = -1;
	string m_sDeathCause;
	string m_sCreditSource;
	int m_iDamageEventSequence;
	// Reference to an immutable damage record, never another death.
	ref GC_SessionEvent m_LastDamage;

	//------------------------------------------------------------------------------------------------
	void Write(SaveContext context)
	{
		context.WriteValue("sequence", m_iSequence);
		context.WriteValue("sessionSeconds", m_fSeconds);
		context.WriteValue("type", m_sType);
		context.WriteValue("actor", m_sActor);
		context.WriteValue("actorName", m_sActorName);
		context.WriteValue("target", m_sTarget);
		context.WriteValue("targetName", m_sTargetName);
		context.WriteValue("actorKind", m_sActorKind);
		context.WriteValue("targetKind", m_sTargetKind);
		context.WriteValue("actorFaction", m_sActorFaction);
		context.WriteValue("targetFaction", m_sTargetFaction);
		context.WriteValue("actorPrefab", m_sActorPrefab);
		context.WriteValue("targetPrefab", m_sTargetPrefab);
		context.WriteValue("actorEntity", m_sActorEntity);
		context.WriteValue("targetEntity", m_sTargetEntity);
		context.WriteValue("actorPosition", m_aActorPosition);
		context.WriteValue("targetPosition", m_aTargetPosition);
		context.WriteValue("weapon", m_sWeapon);
		context.WriteValue("projectile", m_sProjectile);
		context.WriteValue("magazine", m_sMagazine);
		context.WriteValue("detail", m_sDetail);
		context.WriteValue("reportedDamage", m_fDamage);
		context.WriteValue("damageType", m_iDamageType);
		context.WriteValue("relation", m_iRelation);
		context.WriteValue("distanceMetres", m_fDistance);
		context.WriteValue("deathCause", m_sDeathCause);
		context.WriteValue("creditSource", m_sCreditSource);
		context.WriteValue("damageEventSequence", m_iDamageEventSequence);
		if (m_LastDamage)
		{
			context.WriteValue("lastDamageSequence", m_LastDamage.m_iSequence);
			context.WriteValue("lastDamageActor", m_LastDamage.m_sActor);
			context.WriteValue("lastDamageActorName", m_LastDamage.m_sActorName);
			context.WriteValue("lastDamageActorKind", m_LastDamage.m_sActorKind);
			context.WriteValue("lastDamageActorPrefab", m_LastDamage.m_sActorPrefab);
			context.WriteValue("lastDamageWeapon", m_LastDamage.m_sWeapon);
			context.WriteValue("lastDamageProjectile", m_LastDamage.m_sProjectile);
			context.WriteValue("lastDamageType", m_LastDamage.m_iDamageType);
		}
	}
}
