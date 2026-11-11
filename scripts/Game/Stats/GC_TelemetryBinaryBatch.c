//! L2TB v1: bounded, uncompressed numeric tables plus the existing cold-event JSON.
//! All numeric IO is bulk native array IO. Compression belongs outside the game.
class GC_TelemetryBinaryBatch
{
	protected ref array<int> m_aStateIds = {};
	protected ref array<float> m_aStates = {};
	protected ref array<int> m_aVehicleIds = {};
	protected ref array<float> m_aVehicles = {};
	protected ref FileHandle m_File;
	protected int m_iSaveStep;
	protected int m_iWritten;
	protected int m_iCompared;
	protected ref array<int> m_aReadInts;
	protected ref array<float> m_aReadFloats;
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
		int result;
		while (result == 0)
			result = SaveVerifiedStep(path, header, cold);
		return result > 0;
	}

	//------------------------------------------------------------------------------------------------
	//! One bounded table/string IO step. The caller must keep this batch and its
	//! envelope immutable until completion. 0 = pending, 1 = verified, -1 = failed.
	int SaveVerifiedStep(string path, string header, string cold)
	{
		array<int> prefix = {MAGIC, 1, header.Length(), cold.Length(), m_aStateIds.Count() / 4, m_aVehicleIds.Count() / 3};
		int expected = 24 + header.Length() + cold.Length() + GetNumericBytes();
		bool valid = true;
		int compared = 1;
		string actual;
		switch (m_iSaveStep)
		{
			case 0:
				m_File = FileIO.OpenFile(path, FileMode.WRITE);
				valid = m_File != null;
				m_iWritten = 0;
				break;
			case 1: m_iWritten += m_File.WriteArray(prefix); break;
			case 2: m_iWritten += m_File.Write(header); break;
			case 3: m_iWritten += m_File.Write(cold); break;
			case 4: m_iWritten += m_File.WriteArray(m_aStateIds); break;
			case 5: m_iWritten += m_File.WriteArray(m_aStates); break;
			case 6: m_iWritten += m_File.WriteArray(m_aVehicleIds); break;
			case 7: m_iWritten += m_File.WriteArray(m_aVehicles); break;
			case 8:
				m_File.Close();
				m_File = null;
				valid = m_iWritten == expected;
				break;
			case 9:
				m_File = FileIO.OpenFile(path, FileMode.READ);
				valid = m_File && m_File.GetLength() == expected;
				break;
			case 10: compared = VerifyInts(m_File, prefix); break;
			case 11: valid = m_File.Read(actual, header.Length()) == header.Length() && actual == header; break;
			case 12: valid = m_File.Read(actual, cold.Length()) == cold.Length() && actual == cold; break;
			case 13: compared = VerifyInts(m_File, m_aStateIds); break;
			case 14: compared = VerifyFloats(m_File, m_aStates); break;
			case 15: compared = VerifyInts(m_File, m_aVehicleIds); break;
			case 16: compared = VerifyFloats(m_File, m_aVehicles); break;
			case 17:
				ResetSave();
				return 1;
		}
		if (!valid || compared < 0)
		{
			ResetSave();
			return -1;
		}
		if (compared == 0)
			return 0;
		m_iSaveStep++;
		return 0;
	}

	protected void ResetSave()
	{
		if (m_File)
			m_File.Close();
		m_File = null;
		m_iSaveStep = 0;
		m_iCompared = 0;
		m_aReadInts = null;
		m_aReadFloats = null;
	}

	void ~GC_TelemetryBinaryBatch()
	{
		ResetSave();
	}

	//------------------------------------------------------------------------------------------------
	protected int VerifyInts(FileHandle file, array<int> expected)
	{
		if (!m_aReadInts)
		{
			m_aReadInts = {};
			if (file.ReadArray(m_aReadInts, 4, expected.Count()) != expected.Count() * 4 || m_aReadInts.Count() != expected.Count())
				return -1;
			m_iCompared = 0;
			return 0;
		}
		int end = Math.Min(expected.Count(), m_iCompared + 256);
		for (; m_iCompared < end; m_iCompared++)
		{
			if (m_aReadInts[m_iCompared] != expected[m_iCompared])
				return -1;
		}
		if (m_iCompared < expected.Count())
			return 0;
		m_aReadInts = null;
		return 1;
	}

	//------------------------------------------------------------------------------------------------
	protected int VerifyFloats(FileHandle file, array<float> expected)
	{
		if (!m_aReadFloats)
		{
			m_aReadFloats = {};
			if (file.ReadArray(m_aReadFloats, 4, expected.Count()) != expected.Count() * 4 || m_aReadFloats.Count() != expected.Count())
				return -1;
			m_iCompared = 0;
			return 0;
		}
		int end = Math.Min(expected.Count(), m_iCompared + 256);
		for (; m_iCompared < end; m_iCompared++)
		{
			if (m_aReadFloats[m_iCompared] != expected[m_iCompared])
				return -1;
		}
		if (m_iCompared < expected.Count())
			return 0;
		m_aReadFloats = null;
		return 1;
	}
}
