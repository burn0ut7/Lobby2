//! Boundary conversion for existing string-based runtime/RPC contracts.
//! Authored fields use FactionKey so Workbench can identify their semantics.
class GC_FactionKeys
{
	static array<string> ToStrings(array<FactionKey> keys)
	{
		array<string> values = {};
		if (keys)
		{
			foreach (FactionKey key : keys)
				values.Insert(key);
		}
		return values;
	}

	static array<FactionKey> FromStrings(array<string> values)
	{
		array<FactionKey> keys = {};
		if (values)
		{
			foreach (string value : values)
				keys.Insert(value);
		}
		return keys;
	}
}
