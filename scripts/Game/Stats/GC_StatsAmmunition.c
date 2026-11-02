//! One ammunition aggregation path for persisted players and transient AI totals.
class GC_StatsAmmunition
{
	static bool Record(GC_MissionStatsComponent owner, GC_StatsAssignment assignment, BaseWeaponComponent weapon, IEntity projectile, bool persistent)
	{
		string projectilePrefab;
		string magazinePrefab;
		string weaponPrefab;
		if (projectile && projectile.GetPrefabData())
			projectilePrefab = projectile.GetPrefabData().GetPrefabName();
		if (weapon)
		{
			if (weapon.GetOwner() && weapon.GetOwner().GetPrefabData())
				weaponPrefab = weapon.GetOwner().GetPrefabData().GetPrefabName();
			BaseMagazineComponent magazine = weapon.GetCurrentMagazine();
			if (magazine && magazine.GetOwner() && magazine.GetOwner().GetPrefabData())
				magazinePrefab = magazine.GetOwner().GetPrefabData().GetPrefabName();
		}
		foreach (GC_StatsAmmo existing : assignment.m_aAmmo)
		{
			if (existing.m_sProjectile == projectilePrefab && existing.m_sMagazine == magazinePrefab && existing.m_sWeapon == weaponPrefab)
			{
				existing.m_iRounds++;
				return true;
			}
		}
		if (assignment.m_aAmmo.Count() >= 128)
		{
			if (persistent)
				owner.MarkIncomplete();
			return false;
		}
		if (persistent && !owner.AllocateAmmoType())
			return false;
		GC_StatsAmmo ammo = new GC_StatsAmmo();
		ammo.m_sProjectile = projectilePrefab;
		ammo.m_sMagazine = magazinePrefab;
		ammo.m_sWeapon = weaponPrefab;
		ammo.m_iRounds = 1;
		assignment.m_aAmmo.Insert(ammo);
		return true;
	}
}
