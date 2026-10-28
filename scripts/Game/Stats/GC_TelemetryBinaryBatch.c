//! L2TB v1: bounded, uncompressed numeric tables plus the existing cold-event JSON.
//! All numeric IO is bulk native array IO. Compression belongs outside the game.
class GC_TelemetryBinaryBatch
{
	protected ref array<int> m_aStateIds = {};
	protected ref array<float> m_aStates = {};
	protected ref array<int> m_aVehicleIds = {};
	protected ref array<float> m_aVehicles = {};
	static const int MAGIC = 1112814156;

	//------------------------------------------------------------------------------------------------
	void Add(int sequence, float timeMs, int entity, vector position, vector angles, vector velocity, GC_SpectatorSample sample, bool discontinuity, float previousSampleMs)
	{
		array<int> ids = m_aVehicleIds;
		array<float> values = m_aVehicles;
		int bits = discontinuity;
		if (sample)
		{
			ids = m_aStateIds;
			values = m_aStates;
			bits = sample.m_iStance | (sample.m_iFlags << 8) | (bits << 16);
		}
		ids.Insert(sequence);
		ids.Insert(entity);
		ids.Insert(bits);
		if (sample)
			ids.Insert(sample.m_iCommand);
		values.Insert(timeMs);
		values.Insert(previousSampleMs);
		for (int axis; axis < 3; axis++)
			values.Insert(position[axis]);
		for (int axis; axis < 3; axis++)
			values.Insert(angles[axis]);
		for (int axis; axis < 3; axis++)
			values.Insert(velocity[axis]);
		if (!sample)
			return;
		for (int axis; axis < 3; axis++)
			values.Insert(sample.m_vAimAngles[axis]);
		for (int axis; axis < 3; axis++)
			values.Insert(sample.m_vLookAngles[axis]);
		values.Insert(sample.m_fLean);
		values.Insert(sample.m_fDynamicStance);
		values.Insert(sample.m_fMovement);
	}

	//------------------------------------------------------------------------------------------------
	int GetNumericBytes()
	{
		return (m_aStateIds.Count() + m_aStates.Count() + m_aVehicleIds.Count() + m_aVehicles.Count()) * 4;
	}

	//------------------------------------------------------------------------------------------------
	//! A ready marker may only follow this successful, byte-counted readback.
	bool SaveVerified(string path, string header, string cold)
	{
		array<int> prefix = {MAGIC, 1, header.Length(), cold.Length(), m_aStateIds.Count() / 4, m_aVehicleIds.Count() / 3};
		int expected = 24 + header.Length() + cold.Length() + GetNumericBytes();
		FileHandle file = FileIO.OpenFile(path, FileMode.WRITE);
		if (!file)
			return false;
		int written = file.WriteArray(prefix);
		written += file.Write(header);
		written += file.Write(cold);
		written += file.WriteArray(m_aStateIds);
		written += file.WriteArray(m_aStates);
		written += file.WriteArray(m_aVehicleIds);
		written += file.WriteArray(m_aVehicles);
		file.Close();
		if (written != expected)
			return false;
		file = FileIO.OpenFile(path, FileMode.READ);
		if (!file)
			return false;
		bool valid = file.GetLength() == expected && VerifyInts(file, prefix);
		string actual;
		valid = valid && file.Read(actual, header.Length()) == header.Length() && actual == header;
		valid = valid && file.Read(actual, cold.Length()) == cold.Length() && actual == cold;
		valid = valid && VerifyInts(file, m_aStateIds) && VerifyFloats(file, m_aStates);
		valid = valid && VerifyInts(file, m_aVehicleIds) && VerifyFloats(file, m_aVehicles);
		file.Close();
		return valid;
	}

	//------------------------------------------------------------------------------------------------
	protected bool VerifyInts(FileHandle file, array<int> expected)
	{
		array<int> actual = {};
		if (file.ReadArray(actual, 4, expected.Count()) != expected.Count() * 4 || actual.Count() != expected.Count())
			return false;
		for (int i; i < expected.Count(); i++)
		{
			if (actual[i] != expected[i])
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool VerifyFloats(FileHandle file, array<float> expected)
	{
		array<float> actual = {};
		if (file.ReadArray(actual, 4, expected.Count()) != expected.Count() * 4 || actual.Count() != expected.Count())
			return false;
		for (int i; i < expected.Count(); i++)
		{
			if (actual[i] != expected[i])
				return false;
		}
		return true;
	}
}
