#ifdef WORKBENCH
modded class GC_LobbyManager
{
	protected ref GC_ItemCatalogue m_GCItemCatalogue;
	protected int m_iGCItemCatalogueFrames;

	//------------------------------------------------------------------------------------------------
	string GC_TestItemCatalogueStart()
	{
		GetGame().GetCallqueue().Remove(GC_TestItemCatalogueFrame);
		m_GCItemCatalogue = new GC_ItemCatalogue();
		m_iGCItemCatalogueFrames = 0;
		m_GCItemCatalogue.Start();
		GetGame().GetCallqueue().CallLater(GC_TestItemCatalogueFrame, 1, true);
		return m_GCItemCatalogue.Status();
	}

	//------------------------------------------------------------------------------------------------
	protected void GC_TestItemCatalogueFrame()
	{
		if (!m_GCItemCatalogue || GC_LobbyManager.GetManager() != this)
		{
			GC_TestItemCatalogueStop();
			return;
		}
		m_iGCItemCatalogueFrames++;
		m_GCItemCatalogue.Tick();
		if (m_GCItemCatalogue.IsFinished())
			GetGame().GetCallqueue().Remove(GC_TestItemCatalogueFrame);
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestItemCatalogueStop()
	{
		GetGame().GetCallqueue().Remove(GC_TestItemCatalogueFrame);
		m_GCItemCatalogue = null;
		return "stopped";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestItemCatalogueStatus()
	{
		if (!m_GCItemCatalogue)
			return "not started";
		return m_GCItemCatalogue.Metrics() + " | frames=" + m_iGCItemCatalogueFrames + "\n" + m_GCItemCatalogue.CoverageReport();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestItemCatalogueSearch(string text, string category, string equipmentSlot, string addon, string faction, string mountType, int offset)
	{
		if (!m_GCItemCatalogue)
			return "not started";
		GC_ItemCatalogueQuery query = new GC_ItemCatalogueQuery();
		query.m_sText = text;
		query.m_sCategory = category;
		query.m_sEquipmentSlot = equipmentSlot;
		query.m_sSourceAddon = addon;
		query.m_sFaction = faction;
		query.m_MountType = mountType.ToType();
		if (!mountType.IsEmpty() && !query.m_MountType)
			return "invalid mount type";
		array<GC_ItemCatalogueEntry> results = {};
		int total = m_GCItemCatalogue.Search(query, offset, 8, results);
		string report = string.Format("matches=%1 finished=%2 offset=%3", total, m_GCItemCatalogue.IsFinished(), offset);
		foreach (GC_ItemCatalogueEntry entry : results)
		{
			report += "\n" + entry.m_sName + " | " + entry.m_sCategory + " | " + entry.m_sEquipmentSlot + " | " + entry.m_sPrefab + " | factions=" + entry.m_sFactions;
			for (int slot; slot < entry.m_aMountNames.Count(); slot++)
				report += "\n  mount=" + entry.m_aMountNames[slot] + " type=" + entry.m_aMountTypes[slot];
		}
		return report;
	}

	//------------------------------------------------------------------------------------------------
	//! Explicit isolated fixture; checks production APIs, then deletes only its own entities.
	string GC_TestItemAttachmentFixture()
	{
		ResourceName weaponPrefab = "{96DFD2E7E63B3386}Prefabs/Weapons/Rifles/AK74/Rifle_AK74N.et";
		ResourceName opticPrefab = "{ACDF49FACD0701A8}Prefabs/Weapons/Attachments/Optics/Optic_1P29/Optic_1P29.et";
		ResourceName helmetPrefab = "{E49D9EE7E2B3016C}Prefabs/Characters/HeadGear/Helmet_ZSh5_01/Helmet_ZSh5_01.et";
		IEntity weaponEntity = GetGame().SpawnEntityPrefab(Resource.Load(weaponPrefab));
		IEntity optic = GetGame().SpawnEntityPrefab(Resource.Load(opticPrefab));
		IEntity helmet = GetGame().SpawnEntityPrefab(Resource.Load(helmetPrefab));
		string report = "fixture unavailable";
		if (weaponEntity && optic && helmet)
		{
			BaseWeaponComponent weapon = BaseWeaponComponent.Cast(weaponEntity.FindComponent(BaseWeaponComponent));
			array<AttachmentSlotComponent> slots = {};
			weapon.GetAttachments(slots);
			foreach (AttachmentSlotComponent slot : slots)
			{
				if (!slot.GetAttachmentSlotType() || slot.GetAttachmentSlotType().Type() != AttachmentOpticsDovetailAK)
					continue;
				IEntity before = slot.GetAttachedEntity();
				GC_EItemCompatibility accepted = GC_ItemCompatibility.CheckAttachment(optic, slot);
				GC_EItemCompatibility rejected = GC_ItemCompatibility.CheckAttachment(helmet, slot);
				GC_EItemCompatibility unknown = GC_ItemCompatibility.CheckAttachment(null, slot);
				bool pass = accepted == GC_EItemCompatibility.ACCEPTED && rejected == GC_EItemCompatibility.REJECTED && unknown == GC_EItemCompatibility.UNVERIFIED && slot.GetAttachedEntity() == before;
				report = string.Format("pass=%1 optic=%2 helmet=%3 missing=%4 unchanged=%5", pass, accepted, rejected, unknown, slot.GetAttachedEntity() == before);
			}
		}
		SCR_EntityHelper.DeleteEntityAndChildren(weaponEntity);
		SCR_EntityHelper.DeleteEntityAndChildren(optic);
		SCR_EntityHelper.DeleteEntityAndChildren(helmet);
		return report;
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestItemInsertionFixture()
	{
		ResourceName characterPrefab = "{26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et";
		ResourceName riflePrefab = "{FA5C25BF66A53DCF}Prefabs/Weapons/Rifles/AK74/Rifle_AK74.et";
		ResourceName helmetPrefab = "{E49D9EE7E2B3016C}Prefabs/Characters/HeadGear/Helmet_ZSh5_01/Helmet_ZSh5_01.et";
		IEntity character = GetGame().SpawnEntityPrefab(Resource.Load(characterPrefab));
		if (!character)
			return "fixture unavailable";
		InventoryStorageManagerComponent manager = InventoryStorageManagerComponent.Cast(character.FindComponent(InventoryStorageManagerComponent));
		string report = "manager unavailable";
		if (manager)
		{
			array<IEntity> before = {};
			manager.GetItems(before);
			array<BaseInventoryStorageComponent> storages = {};
			manager.GetStorages(storages);
			report = "";
			int accepted, rejected, changed;
			foreach (BaseInventoryStorageComponent storage : storages)
			{
				for (int i; i < storage.GetSlotsCount(); i++)
				{
					InventoryStorageSlot slot = storage.GetSlot(i);
					IEntity previous = slot.GetAttachedEntity();
					GC_EItemCompatibility rifle = GC_ItemCompatibility.CheckInsertion(riflePrefab, manager, storage, i);
					GC_EItemCompatibility helmet = GC_ItemCompatibility.CheckInsertion(helmetPrefab, manager, storage, i);
					if (rifle == GC_EItemCompatibility.ACCEPTED || helmet == GC_EItemCompatibility.ACCEPTED)
						accepted++;
					if (rifle == GC_EItemCompatibility.REJECTED || helmet == GC_EItemCompatibility.REJECTED)
						rejected++;
					if (previous != slot.GetAttachedEntity())
						changed++;
					report += string.Format("\n%1[%2] rifle=%3 helmet=%4", storage.Type(), i, rifle, helmet);
				}
			}
			array<IEntity> after = {};
			manager.GetItems(after);
			GC_EItemCompatibility invalid = GC_ItemCompatibility.CheckInsertion(riflePrefab, manager, null, -1);
			bool pass = accepted > 0 && rejected > 0 && changed == 0 && before.Count() == after.Count() && invalid == GC_EItemCompatibility.UNVERIFIED;
			report = string.Format("pass=%1 acceptedSlots=%2 rejectedSlots=%3 changedSlots=%4 itemsBefore=%5 itemsAfter=%6 invalid=%7", pass, accepted, rejected, changed, before.Count(), after.Count(), invalid) + report;
		}
		SCR_EntityHelper.DeleteEntityAndChildren(character);
		return report;
	}
}
#endif
