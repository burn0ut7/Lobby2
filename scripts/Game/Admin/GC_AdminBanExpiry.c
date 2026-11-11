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
			if (!value.IsDigitAt(index))
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
		int year;
		if (!ParseNumber(value.Substring(0, 4), year) || year < 1970 || year > 2037)
			return false;

		string iso = value;
		iso.Replace(" ", "T");
		iso += ":00.000Z";
		Timestamp parsed = Timestamp.FromIsoString(iso);
		// Reject invalid or normalized input; the epoch itself remains a valid date.
		if (parsed.ToIsoString() != iso)
			return false;

		timestamp = parsed.ToEpochSeconds();
		return true;
	}
}
