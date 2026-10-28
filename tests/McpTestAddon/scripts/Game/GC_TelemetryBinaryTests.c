#ifdef WORKBENCH
//! Opt-in serializer benchmark, not fabricated gameplay/mission evidence.
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestTelemetryBinaryBenchmark(bool binary, int count)
	{
		if (!Replication.IsServer() || count < 1 || count > 512)
			return "REJECTED authority/count";
		string header = "{\"kind\":\"telemetry\",\"schemaVersion\":2,\"missionId\":\"00000000-0000-0000-0000-000000000001\",\"recordingId\":\"00000000-0000-0000-0000-000000000002\",\"revision\":0,\"startedAt\":0,\"world\":\"synthetic-benchmark\",\"gameVersion\":\"1.8.0.13\",\"assetManifestId\":\"synthetic\",\"clock\":\"session-world-ms\",\"coordinates\":\"enfusion-world-metres\",\"sampleHz\":5,\"firstSequence\":1,\"fromMs\":1000,\"untilMs\":2000,\"closed\":true,\"stopReason\":\"disabled\",\"droppedRecords\":0,\"coverage\":{\"actors\":\"players\",\"pose\":\"inferred\",\"shots\":false,\"impacts\":false,\"damage\":false,\"grenades\":false,\"vehicles\":false,\"gear\":false}}";
		GC_TelemetryBinaryBatch packed = new GC_TelemetryBinaryBatch();
		string records;
		GC_SpectatorSample sample = new GC_SpectatorSample();
		sample.m_iCommand = 21;
		sample.m_iFlags = 3;
		sample.m_fLean = 0.6;
		sample.m_fDynamicStance = 0.75;
		sample.m_fMovement = 1;
		sample.m_vAimAngles = "0.1 0.2 0.3";
		sample.m_vLookAngles = "0.2 0.3 0.4";
		int started = System.GetTickCount();
		for (int i; i < count; i++)
		{
			int entity = (i % 128) + 1;
			int frame = i / 128;
			float timeMs = 1000 + frame * 200;
			vector position = Vector(1200 + i * 0.125, 39, 2700 + i * 0.25);
			vector angles = Vector(i * 0.25, 0, 0);
			vector velocity = "1.25 0 2.5";
			if (binary)
			{
				packed.Add(i + 1, timeMs, entity, position, angles, velocity, sample, false, timeMs - 200);
				continue;
			}
			JsonSaveContext context = new JsonSaveContext();
			context.SetMaxDecimalPlaces(3);
			context.WriteValue("entity", "entity-" + entity);
			GC_TelemetryRecorder.WriteVector(context, "position", position);
			GC_TelemetryRecorder.WriteVector(context, "anglesDegrees", angles);
			GC_TelemetryRecorder.WriteVector(context, "velocity", velocity);
			bool discontinuity = false;
			context.WriteValue("discontinuity", discontinuity);
			GC_TelemetryRecorder.WriteVector(context, "aimRadians", sample.m_vAimAngles);
			GC_TelemetryRecorder.WriteVector(context, "lookRadians", sample.m_vLookAngles);
			context.WriteValue("stance", sample.m_iStance);
			context.WriteValue("command", sample.m_iCommand);
			context.WriteValue("flags", sample.m_iFlags);
			context.WriteValue("lean", sample.m_fLean);
			context.WriteValue("dynamicStance", sample.m_fDynamicStance);
			context.WriteValue("movement", sample.m_fMovement);
			string state = context.SaveToString();
			JsonSaveContext window = new JsonSaveContext();
			window.SetMaxDecimalPlaces(6);
			window.WriteValue("previousSampleMs", timeMs - 200);
			string timing = window.SaveToString();
			state = state.Substring(0, state.Length() - 1) + "," + timing.Substring(1, timing.Length() - 1);
			JsonSaveContext envelope = new JsonSaveContext();
			envelope.SetMaxDecimalPlaces(6);
			envelope.WriteValue("sequence", i + 1);
			envelope.WriteValue("timeMs", timeMs);
			envelope.WriteValue("type", "state");
			string encoded = envelope.SaveToString();
			if (i > 0)
				records += ",";
			records += encoded.Substring(0, encoded.Length() - 1) + ",\"data\":" + state + "}";
		}
		int encodeMs = System.GetTickCount(started);
		FileIO.MakeDirectory("$profile:TelemetryBinaryBench");
		started = System.GetTickCount();
		bool saved;
		int bytes;
		if (binary)
		{
			saved = packed.SaveVerified("$profile:TelemetryBinaryBench/native.l2t", header, "[]");
			bytes = 24 + header.Length() + 2 + packed.GetNumericBytes();
		}
		else
		{
			string json = header.Substring(0, header.Length() - 1) + ",\"records\":[" + records + "]}";
			saved = GC_StatsOutbox.WriteVerified("$profile:TelemetryBinaryBench/native.json", json);
			bytes = json.Length();
		}
		return string.Format("binary=%1 count=%2 encodeMs=%3 publishVerifyMs=%4 bytes=%5 verified=%6", binary, count, encodeMs, System.GetTickCount(started), bytes, saved);
	}
}
#endif
