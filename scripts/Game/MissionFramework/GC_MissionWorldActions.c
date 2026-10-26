[BaseContainerProps(), BaseContainerCustomStringTitleField("Delete Entities")]
class GC_DeleteEntitiesAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Unique entity names to delete, including their children. Never target the game mode.")]
	ref array<string> m_aEntities;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (m_aEntities && !m_aEntities.IsEmpty())
			return true;
		error = "Delete Entities: add entity names.";
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		if (!context.IsActive())
			return false;
		bool applied = true;
		foreach (string name : m_aEntities)
		{
			IEntity entity = GetGame().GetWorld().FindEntityByName(name);
			if (!context.CanDelete(entity))
			{
				applied = false;
				continue;
			}
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
		}
		return applied;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Service Vehicles")]
class GC_ServiceVehiclesAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Unique names of the vehicles to service.")]
	ref array<string> m_aVehicles;
	[Attribute("0", UIWidgets.CheckBox, "Set fuel to the percentage below.")]
	bool m_bSetFuel;
	[Attribute("100", UIWidgets.EditBox, "Fuel percentage.", params: "0 100 1")]
	float m_fFuelPercentage;
	[Attribute("0", UIWidgets.CheckBox, "Set the vehicle's supply amount.")]
	bool m_bSetSupplies;
	[Attribute("0", UIWidgets.EditBox, "Supply amount, limited by the vehicle's capacity.", params: "0 inf 1")]
	int m_iSupplies;
	[Attribute("1", UIWidgets.CheckBox, "Fully repair the vehicle.")]
	bool m_bRepair;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (m_aVehicles && !m_aVehicles.IsEmpty() && (m_bSetFuel || m_bSetSupplies || m_bRepair)
			&& m_fFuelPercentage >= 0 && m_fFuelPercentage <= 100 && m_iSupplies >= 0)
			return true;
		error = "Service Vehicles: choose vehicles and at least one service, with valid fuel/supply values.";
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		if (!context.IsActive())
			return false;
		bool applied = true;
		foreach (string name : m_aVehicles)
		{
			IEntity vehicle = GetGame().GetWorld().FindEntityByName(name);
			if (!vehicle)
			{
				applied = false;
				continue;
			}
			if (m_bSetFuel)
			{
				array<SCR_FuelManagerComponent> managers = {};
				SCR_FuelManagerComponent.GetAllFuelManagers(vehicle, managers);
				if (managers.IsEmpty())
					applied = false;
				else
					SCR_FuelManagerComponent.SetTotalFuelPercentageOfFuelManagers(managers, m_fFuelPercentage * 0.01);
			}
			if (m_bSetSupplies)
			{
				SCR_ResourceComponent resources = SCR_ResourceComponent.FindResourceComponent(vehicle, false);
				SCR_ResourceContainer container;
				if (resources && resources.GetContainer(EResourceType.SUPPLIES, container))
					container.SetResourceValue(m_iSupplies);
				else
					applied = false;
			}
			if (m_bRepair)
			{
				SCR_DamageManagerComponent damage = SCR_DamageManagerComponent.GetDamageManager(vehicle);
				if (damage)
					damage.FullHeal();
				else
					applied = false;
			}
		}
		return applied;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Set Time")]
class GC_SetTimeAction : GC_MissionAction
{
	[Attribute("8", UIWidgets.EditBox, "Hour, 0 to 23. For a pregame change, disable Reset Time On Game Start on the native time/weather component.", params: "0 23 1")]
	int m_iHour;
	[Attribute("0", UIWidgets.EditBox, "Minute, 0 to 59.", params: "0 59 1")]
	int m_iMinute;
	[Attribute("0", UIWidgets.EditBox, "Second, 0 to 59.", params: "0 59 1")]
	int m_iSecond;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (m_iHour >= 0 && m_iHour <= 23 && m_iMinute >= 0 && m_iMinute <= 59 && m_iSecond >= 0 && m_iSecond <= 59)
			return true;
		error = "Set Time: enter a valid hour, minute and second.";
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		if (!context.IsActive())
			return false;
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		TimeAndWeatherManagerEntity weather = world.GetTimeAndWeatherManager();
		return weather && weather.SetHoursMinutesSeconds(m_iHour, m_iMinute, m_iSecond);
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Set Weather")]
class GC_SetWeatherAction : GC_MissionAction
{
	[Attribute("Clear", UIWidgets.EditBox, "Weather preset name from the world's weather states, for example Clear.")]
	string m_sPreset;
	[Attribute("0", UIWidgets.CheckBox, "Allow normal random weather changes afterward.")]
	bool m_bAllowRandomChanges;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		TimeAndWeatherManagerEntity weather = world.GetTimeAndWeatherManager();
		if (weather)
		{
			array<ref WeatherState> states = {};
			weather.GetWeatherStatesList(states);
			foreach (WeatherState state : states)
			{
				if (state.GetStateName() == m_sPreset)
					return true;
			}
		}
		error = "Set Weather: missing weather manager or unknown preset " + m_sPreset;
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		if (!context.IsActive())
			return false;
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		TimeAndWeatherManagerEntity weather = world.GetTimeAndWeatherManager();
		if (!weather || !weather.GetTransitionManager())
			return false;
		weather.ForceWeatherTo(!m_bAllowRandomChanges, m_sPreset);
		return true;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Set Entity Health (%)")]
class GC_SetEntityHealthAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Unique entity name.")]
	string m_sEntity;
	[Attribute("", UIWidgets.EditBox, "Optional hit-zone name. Empty uses the default zone.")]
	string m_sHitZone;
	[Attribute("100", UIWidgets.EditBox, "Health percentage. Zero destroys the hit zone.", params: "0 100 1")]
	float m_fHealthPercentage;

	//------------------------------------------------------------------------------------------------
	override bool Validate(GC_LobbyMissionContext context, out string error)
	{
		if (!m_sEntity.IsEmpty() && m_fHealthPercentage >= 0 && m_fHealthPercentage <= 100)
			return true;
		error = "Set Entity Health: enter an entity name and percentage from 0 to 100.";
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool Execute(GC_LobbyMissionContext context)
	{
		if (!context.IsActive())
			return false;
		IEntity entity = GetGame().GetWorld().FindEntityByName(m_sEntity);
		if (!entity)
			return false;
		SCR_DamageManagerComponent damage = SCR_DamageManagerComponent.GetDamageManager(entity);
		if (!damage)
			return false;
		HitZone zone = damage.GetDefaultHitZone();
		if (!m_sHitZone.IsEmpty())
			zone = damage.GetHitZoneByName(m_sHitZone);
		if (!zone)
			return false;
		zone.SetHealthScaled(m_fHealthPercentage * 0.01);
		return true;
	}
}
