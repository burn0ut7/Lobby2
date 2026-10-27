//! Shared verified publication protocol for historical batches.
class GC_StatsOutbox
{
	//------------------------------------------------------------------------------------------------
	static bool WriteReady(string path)
	{
		FileHandle marker = FileIO.OpenFile(path, FileMode.WRITE);
		if (!marker)
			return false;
		int written = marker.Write("ready");
		marker.Close();
		// Once the marker is closed the uploader may consume it immediately; do not read it back.
		return written == 5;
	}

	//------------------------------------------------------------------------------------------------
	static bool WriteVerified(string path, string value)
	{
		FileHandle file = FileIO.OpenFile(path, FileMode.WRITE);
		if (!file)
			return false;
		int written = file.Write(value);
		file.Close();
		if (written != value.Length())
			return false;
		file = FileIO.OpenFile(path, FileMode.READ);
		if (!file)
			return false;
		string actual;
		int read = file.Read(actual, value.Length());
		file.Close();
		return read == value.Length() && actual == value;
	}

}
