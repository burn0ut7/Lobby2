//! Owns the dynamic Scenario briefing body. The parent briefing view only
//! selects this renderer and includes its fixed row height in body measurement.
class GC_BriefingScenarioComponent : SCR_ScriptedWidgetComponent
{
    protected static const ResourceName ROW_LAYOUT = "{6A4D000000000006}UI/layouts/Briefing/Lobby_Briefing_Scenario_Row.layout";
    protected static const ResourceName FALLBACK_ICON_SET = "{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.imageset";
    protected static const ResourceName TIME_ICON_MIDNIGHT = "{5E793A7B8DA84847}UI/Textures/Editor/Attributes/Time/Attribute_Time_Midnight.edds";
    protected static const ResourceName TIME_ICON_EARLY_MORNING = "{94B936AA2CE13242}UI/Textures/Editor/Attributes/Time/Attribute_Time_EarlyMorning.edds";
    protected static const ResourceName TIME_ICON_MORNING = "{FF96EE66370C0CBB}UI/Textures/Editor/Attributes/Time/Attribute_Time_Morning.edds";
    protected static const ResourceName TIME_ICON_MIDDAY = "{1F170BA4B611B7D6}UI/Textures/Editor/Attributes/Time/Attribute_Time_Midday.edds";
    protected static const ResourceName TIME_ICON_AFTERNOON = "{F63FE753B10DD0F0}UI/Textures/Editor/Attributes/Time/Attribute_Time_Afternoon.edds";
    protected static const ResourceName TIME_ICON_EVENING = "{59ABC4E5F1CC0755}UI/Textures/Editor/Attributes/Time/Attribute_Time_Evening.edds";
    protected static const ResourceName DATE_ICON = "{90DDF7300CC992DC}UI/Textures/Editor/Clock/Clock_Date.edds";
    protected static const ResourceName WIND_ICON_NONE = "{A97717F1D9E0BE0C}UI/Textures/Icons/no_wind_clean_1024.edds";
    protected static const ResourceName WIND_ICON_LOW = "{19467CC797576B18}UI/Textures/Icons/low_wind_clean_1024.edds";
    protected static const ResourceName WIND_ICON_MEDIUM = "{B1F271C0C1BC8EB5}UI/Textures/Icons/medium_wind_clean_1024.edds";
    protected static const ResourceName WIND_ICON_HEAVY = "{68C97BEE966A0505}UI/Textures/Icons/heavy_wind_clean_1024.edds";
    protected static const string FALLBACK_ICON_NAME = "scenarios";
    protected static const int REFRESH_INTERVAL = 30000;
    protected static const int ROW_COUNT = 7;
    protected static const float ROW_HEIGHT = 88;
    protected static const float METERS_PER_SECOND_TO_MILES_PER_HOUR = 2.236936;

    protected TimeAndWeatherManagerEntity m_WeatherManager;
    protected ref array<SizeLayoutWidget> m_aRows = {};
    protected ref array<ImageWidget> m_aIcons = {};
    protected ref array<RichTextWidget> m_aTexts = {};
    protected ref array<string> m_aRowContents = {};
    protected bool m_bShowing;

    override void HandlerAttached(Widget w)
    {
        super.HandlerAttached(w);
        m_wRoot = w;
    }

    override void HandlerDeattached(Widget w)
    {
        Clear();
        m_wRoot = null;
        super.HandlerDeattached(w);
    }

    bool Show()
    {
        Clear();
        if (!m_wRoot)
            return false;

        WorkspaceWidget workspace = GetGame().GetWorkspace();
        if (!workspace)
            return false;

        for (int index = 0; index < ROW_COUNT; index++)
        {
            Widget rowRoot = workspace.CreateWidgets(ROW_LAYOUT, m_wRoot);
            SizeLayoutWidget rowSize = SizeLayoutWidget.Cast(rowRoot);
            ImageWidget rowIcon;
            RichTextWidget rowText;
            if (rowRoot)
            {
                rowIcon = ImageWidget.Cast(rowRoot.FindAnyWidget("BriefingScenarioIcon"));
                rowText = RichTextWidget.Cast(rowRoot.FindAnyWidget("BriefingScenarioText"));
            }

            if (!rowSize || !rowIcon || !rowText)
            {
                if (rowRoot)
                    rowRoot.RemoveFromHierarchy();

                Clear();
                return false;
            }

            m_aRows.Insert(rowSize);
            m_aIcons.Insert(rowIcon);
            m_aTexts.Insert(rowText);
            m_aRowContents.Insert(string.Empty);
            SetFallbackIcon(index);
        }

        m_bShowing = true;
        Refresh();
        return true;
    }

    void Clear()
    {
        GetGame().GetCallqueue().Remove(Refresh);
        UnbindWeatherManager();
        m_bShowing = false;

        foreach (SizeLayoutWidget row : m_aRows)
        {
            if (row)
                row.RemoveFromHierarchy();
        }

        m_aRows.Clear();
        m_aIcons.Clear();
        m_aTexts.Clear();
        m_aRowContents.Clear();
    }

    float GetContentHeight()
    {
        return m_aRows.Count() * ROW_HEIGHT;
    }

    protected void Refresh()
    {
        GetGame().GetCallqueue().Remove(Refresh);
        if (!m_bShowing || m_aTexts.Count() != ROW_COUNT)
            return;

        ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
        TimeAndWeatherManagerEntity weatherManager;
        if (world)
            weatherManager = world.GetTimeAndWeatherManager();

        BindWeatherManager(weatherManager);
        if (!weatherManager)
        {
            for (int unavailableIndex = 0; unavailableIndex < ROW_COUNT; unavailableIndex++)
                SetRowText(unavailableIndex, "SCENARIO", "Environment information is unavailable.");

            ScheduleFallbackRefresh();
            return;
        }

        WeatherState weatherState = weatherManager.GetCurrentWeatherState();
        if (weatherState)
        {
            string weatherName = weatherState.GetLocalizedName();
            string weatherDescription = weatherState.GetLocalizedDescription();
            if (weatherName.IsEmpty())
                weatherName = "UNSPECIFIED";

            if (SetRowText(0, "WEATHER", weatherName, weatherDescription))
            {
                ResourceName weatherIcon = weatherState.GetIconPath();
                if (weatherIcon.IsEmpty() || !m_aIcons[0].LoadImageTexture(0, weatherIcon))
                    SetFallbackIcon(0);
            }
        }
        else if (SetRowText(0, "WEATHER", "UNAVAILABLE"))
        {
            SetFallbackIcon(0);
        }

        float windDirection = weatherManager.GetWindDirection();
        float windSpeedMph = weatherManager.GetWindSpeed() * METERS_PER_SECOND_TO_MILES_PER_HOUR;
        int roundedWindSpeedMph = windSpeedMph + 0.5;
        int roundedWindDirection = windDirection + 0.5;
        if (roundedWindDirection >= 360)
            roundedWindDirection -= 360;

        string windDirectionName = GetCardinalDirection(windDirection);
        int windInfoIndex;
        SCR_WindDirectionInfo windInfo;
        SCR_UIInfo windUIInfo;
        if (weatherManager.GetWindDirectionInfoFromFloat(windDirection, windInfoIndex, windInfo) && windInfo)
        {
            windUIInfo = windInfo.GetUIInfo();
            if (windUIInfo)
            {
                string authoredWindName = windUIInfo.GetName();
                if (!authoredWindName.IsEmpty())
                    windDirectionName = authoredWindName;
            }
        }

        SetRowText(1, "WIND", string.Format("%1 MPH FROM %2 (%3 DEG)",
            roundedWindSpeedMph, windDirectionName, roundedWindDirection.ToString(3)));
        SetTextureIcon(1, GetWindIcon(windSpeedMph));

        int year;
        int month;
        int day;
        weatherManager.GetDate(year, month, day);
        if (SetRowText(2, "DATE", string.Format("%1 %2 %3", day.ToString(2), GetMonthAbbreviation(month), year)))
            SetTextureIcon(2, DATE_ICON);

        int hours;
        int minutes;
        int seconds;
        weatherManager.GetHoursMinutesSeconds(hours, minutes, seconds);
        bool dayTimeChanged = SetRowText(3, "TIME OF DAY", string.Format("%1%2 HOURS", hours.ToString(2), minutes.ToString(2)));
        if (dayTimeChanged)
            SetTextureIcon(3, GetTimeIcon(hours + minutes / 60.0));

        float sunriseHour;
        bool sunriseChanged;
        if (weatherManager.GetSunriseHour(sunriseHour))
            sunriseChanged = SetRowText(4, "SUNRISE", FormatMilitaryTime(sunriseHour) + " HOURS");
        else
            sunriseChanged = SetRowText(4, "SUNRISE", "NOT AVAILABLE");
        if (sunriseChanged)
            SetTextureIcon(4, TIME_ICON_MORNING);

        float sunsetHour;
        bool sunsetChanged;
        if (weatherManager.GetSunsetHour(sunsetHour))
            sunsetChanged = SetRowText(5, "SUNSET", FormatMilitaryTime(sunsetHour) + " HOURS");
        else
            sunsetChanged = SetRowText(5, "SUNSET", "NOT AVAILABLE");
        if (sunsetChanged)
            SetTextureIcon(5, TIME_ICON_AFTERNOON);

        SCR_MoonPhaseUIInfo moonPhase = weatherManager.GetCurrentMoonPhaseInfoForDate();
        if (moonPhase)
        {
            string moonName = moonPhase.GetName();
            if (moonName.IsEmpty())
                moonName = moonPhase.GetSimplifiedName();

            if (SetRowText(6, "MOON PHASE", moonName))
                moonPhase.SetIconTo(m_aIcons[6]);
        }
        else if (SetRowText(6, "MOON PHASE", "NOT AVAILABLE"))
        {
            SetFallbackIcon(6);
        }

        ScheduleFallbackRefresh();
    }

    protected void ScheduleFallbackRefresh()
    {
        if (m_bShowing)
            GetGame().GetCallqueue().CallLater(Refresh, REFRESH_INTERVAL);
    }

    protected void RequestRefresh()
    {
        if (!m_bShowing)
            return;

        GetGame().GetCallqueue().Remove(Refresh);
        GetGame().GetCallqueue().CallLater(Refresh, 0);
    }

    protected void BindWeatherManager(TimeAndWeatherManagerEntity weatherManager)
    {
        if (m_WeatherManager == weatherManager)
            return;

        UnbindWeatherManager();
        m_WeatherManager = weatherManager;
        if (!m_WeatherManager)
            return;

        m_WeatherManager.GetOnWeatherChanged().Insert(OnWeatherChanged);
        m_WeatherManager.GetOnWindOverrideDataChanged().Insert(OnWindOverrideChanged);
        m_WeatherManager.GetOnWeatherStatePreview().Insert(OnWeatherPreviewChanged);
        m_WeatherManager.GetOnWindPreview().Insert(OnWindPreviewChanged);
        m_WeatherManager.GetOnDateTimePreview().Insert(OnDateTimePreviewChanged);
    }

    protected void UnbindWeatherManager()
    {
        if (!m_WeatherManager)
            return;

        m_WeatherManager.GetOnWeatherChanged().Remove(OnWeatherChanged);
        m_WeatherManager.GetOnWindOverrideDataChanged().Remove(OnWindOverrideChanged);
        m_WeatherManager.GetOnWeatherStatePreview().Remove(OnWeatherPreviewChanged);
        m_WeatherManager.GetOnWindPreview().Remove(OnWindPreviewChanged);
        m_WeatherManager.GetOnDateTimePreview().Remove(OnDateTimePreviewChanged);
        m_WeatherManager = null;
    }

    protected void OnWeatherChanged(notnull TimeAndWeatherManagerEntity manager, int currentStateId, int nextStateId, bool transitioning)
    {
        RequestRefresh();
    }

    protected void OnWindOverrideChanged(notnull TimeAndWeatherManagerEntity manager, bool currentState, float currentSpeed, float currentDirection)
    {
        RequestRefresh();
    }

    protected void OnWeatherPreviewChanged(bool previewEnabled, string weatherState)
    {
        RequestRefresh();
    }

    protected void OnWindPreviewChanged(bool previewEnabled, float windSpeed, float windDirectionDegrees)
    {
        RequestRefresh();
    }

    protected void OnDateTimePreviewChanged(bool previewEnabled, int year, int month, int day, float timeOfTheDay)
    {
        RequestRefresh();
    }

    protected bool SetRowText(int index, string label, string value, string description = "")
    {
        if (index < 0 || index >= m_aTexts.Count() || index >= m_aRowContents.Count() || !m_aTexts[index])
            return false;

        string content = "<color name=\"orange\"><b>" + label + "</b></color><br />" + value;
        if (!description.IsEmpty())
            content += "  <color rgba=\"150,160,164,255\">" + description + "</color>";

        if (m_aRowContents[index] == content)
            return false;

        m_aRowContents[index] = content;
        m_aTexts[index].SetText(content);
        return true;
    }

    protected void SetFallbackIcon(int index)
    {
        if (index < 0 || index >= m_aIcons.Count() || !m_aIcons[index])
            return;

        m_aIcons[index].LoadImageFromSet(0, FALLBACK_ICON_SET, FALLBACK_ICON_NAME);
    }

    protected void SetTextureIcon(int index, ResourceName texture)
    {
        if (index < 0 || index >= m_aIcons.Count() || !m_aIcons[index])
            return;

        if (texture.IsEmpty() || !m_aIcons[index].LoadImageTexture(0, texture))
            SetFallbackIcon(index);
    }

    protected ResourceName GetTimeIcon(float hour24)
    {
        int segment = hour24 / 3;
        switch (segment)
        {
            case 1: return TIME_ICON_EARLY_MORNING;
            case 2: return TIME_ICON_MORNING;
            case 3:
            case 4: return TIME_ICON_MIDDAY;
            case 5: return TIME_ICON_AFTERNOON;
            case 6: return TIME_ICON_EVENING;
        }

        return TIME_ICON_MIDNIGHT;
    }

    protected ResourceName GetWindIcon(float windSpeedMph)
    {
        if (windSpeedMph < 1.0)
            return WIND_ICON_NONE;

        if (windSpeedMph <= 5.0)
            return WIND_ICON_LOW;

        if (windSpeedMph <= 10.0)
            return WIND_ICON_MEDIUM;

        return WIND_ICON_HEAVY;
    }

    protected string FormatMilitaryTime(float hour24)
    {
        int totalMinutes = hour24 * 60 + 0.5;
        totalMinutes = totalMinutes % 1440;
        if (totalMinutes < 0)
            totalMinutes += 1440;

        int hours = totalMinutes / 60;
        int minutes = totalMinutes % 60;
        return hours.ToString(2) + minutes.ToString(2);
    }

    protected string GetCardinalDirection(float degrees)
    {
        int directionIndex = (degrees + 22.5) / 45;
        directionIndex = directionIndex % 8;
        switch (directionIndex)
        {
            case 0: return "N";
            case 1: return "NE";
            case 2: return "E";
            case 3: return "SE";
            case 4: return "S";
            case 5: return "SW";
            case 6: return "W";
        }

        return "NW";
    }

    protected string GetMonthAbbreviation(int month)
    {
        switch (month)
        {
            case 1: return "JAN";
            case 2: return "FEB";
            case 3: return "MAR";
            case 4: return "APR";
            case 5: return "MAY";
            case 6: return "JUN";
            case 7: return "JUL";
            case 8: return "AUG";
            case 9: return "SEP";
            case 10: return "OCT";
            case 11: return "NOV";
            case 12: return "DEC";
        }

        return "UNK";
    }
}
