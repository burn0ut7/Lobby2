#ifdef WORKBENCH
//! Reads faction keys from the current editor world without changing affiliations.
class GC_FactionChoices
{
	//------------------------------------------------------------------------------------------------
	array<string> GetChoices(WorldEditorAPI api)
	{
		array<string> values = {};
		if (!api || !api.GetWorld())
			return values;

		int entityCount = api.GetEditorEntityCount();
		for (int entityIndex; entityIndex < entityCount; entityIndex++)
		{
			IEntitySource source = api.GetEditorEntity(entityIndex);
			if (!source)
				continue;
			IEntity entity = api.SourceToEntity(source);
			if (!entity)
				continue;

			FactionManager manager = FactionManager.Cast(entity);
			if (manager)
			{
				array<Faction> factions = {};
				manager.GetFactionsList(factions);
				foreach (Faction faction : factions)
				{
					if (faction)
						AddKey(values, faction.GetFactionKey());
				}
			}

			SCR_AIGroup group = SCR_AIGroup.Cast(entity);
			if (group)
			{
				// Despite its name, this accessor returns the authored m_faction key.
				AddKey(values, group.GetFactionName());
				Faction groupFaction = group.GetFaction();
				if (groupFaction)
					AddKey(values, groupFaction.GetFactionKey());
			}

			FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
			if (affiliation)
			{
				AddKey(values, affiliation.GetDefaultFactionKey());
				AddKey(values, affiliation.GetAffiliatedFactionKey());
			}
		}

		return values;
	}

	//------------------------------------------------------------------------------------------------
	protected void AddKey(array<string> values, string key)
	{
		// Many entities sharing one key is normal, not a duplicate definition.
		if (!key.IsEmpty() && !values.Contains(key))
			values.Insert(key);
	}
}
#endif
