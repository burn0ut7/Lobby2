//! Parses moderation input without widgets. Absolute dates are UTC and checked again by authority.
class GC_AdminBanExpiry
{
	//------------------------------------------------------------------------------------------------
	static bool ParseNumber(string value, out int number)
	{
		if (value.IsEmpty() || value.Length() > 9)
			return false;
		for (int index; index < value.Length(); index++)
		{
			if (!"0123456789".Contains(value.Substring(index, 1)))
				return false;
		}
		number = value.ToInt();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static bool ParseDuration(string value, int unitSeconds, out int duration)
	{
		int amount;
		if (unitSeconds != 60 && unitSeconds != 3600 && unitSeconds != 86400)
			return false;
		if (!ParseNumber(value, amount) || amount < 1 || amount > 315360000 / unitSeconds)
			return false;
		duration = amount * unitSeconds;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! YYYY-MM-DD HH:MM UTC, bounded below the native signed Unix timestamp limit.
	static bool ParseDate(string value, out int timestamp)
	{
		if (value.Length() != 16 || value.Substring(4, 1) != "-" || value.Substring(7, 1) != "-"
			|| value.Substring(10, 1) != " " || value.Substring(13, 1) != ":")
			return false;
		int year, month, day, hour, minute;
		if (!ParseNumber(value.Substring(0, 4), year) || !ParseNumber(value.Substring(5, 2), month)
			|| !ParseNumber(value.Substring(8, 2), day) || !ParseNumber(value.Substring(11, 2), hour)
			|| !ParseNumber(value.Substring(14, 2), minute))
			return false;
		if (year < 1970 || year > 2037 || month < 1 || month > 12 || hour > 23 || minute > 59)
			return false;
		array<int> monthDays = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
		if (SCR_DateTimeHelper.IsLeapYear(year))
			monthDays[1] = 29;
		if (day < 1 || day > monthDays[month - 1])
			return false;
		int days = day - 1;
		for (int priorYear = 1970; priorYear < year; priorYear++)
		{
			days += 365;
			if (SCR_DateTimeHelper.IsLeapYear(priorYear))
				days++;
		}
		for (int priorMonth; priorMonth < month - 1; priorMonth++)
			days += monthDays[priorMonth];
		timestamp = days * 86400 + hour * 3600 + minute * 60;
		return true;
	}
}
