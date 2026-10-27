using System.Text.Json;
using Npgsql;

internal static class TelemetryIngest
{
    // Out-of-order batches are allowed. Indexed boundary reads reject conflicting
    // clocks, sequence ranges and records after closure. Serializable ingestion
    // prevents concurrent directories from validating against incompatible views.
    public static async Task ValidateContinuity(NpgsqlConnection connection, NpgsqlTransaction transaction, JsonElement incoming)
    {
        var recording = incoming.GetProperty("recordingId").GetGuid();
        var revision = incoming.GetProperty("revision").GetInt32();
        await using var query = new NpgsqlCommand("""
            SELECT payload FROM lobby2.telemetry_batches
            WHERE recording_id=$1 AND revision IN (
                (SELECT max(revision) FROM lobby2.telemetry_batches WHERE recording_id=$1 AND revision < $2),
                (SELECT min(revision) FROM lobby2.telemetry_batches WHERE recording_id=$1 AND revision > $2),
                $2)
            """, connection, transaction);
        query.Parameters.AddWithValue(recording);
        query.Parameters.AddWithValue((long)revision);
        await using var reader = await query.ExecuteReaderAsync();
        while (await reader.ReadAsync())
        {
            using var stored = JsonDocument.Parse(reader.GetString(0));
            var other = stored.RootElement;
            foreach (var name in new[] { "missionId", "startedAt", "world", "gameVersion", "assetManifestId", "clock", "coordinates", "sampleHz", "coverage" })
                if (!JsonElement.DeepEquals(incoming.GetProperty(name), other.GetProperty(name)))
                    throw new InvalidDataException("Telemetry recording metadata changed.");
            var otherRevision = other.GetProperty("revision").GetInt32();
            if (otherRevision == revision) continue; // Exact checksum checked by the shared importer.
            var earlier = otherRevision < revision ? other : incoming;
            var later = otherRevision < revision ? incoming : other;
            if (earlier.GetProperty("closed").GetBoolean()
                || earlier.GetProperty("untilMs").GetDouble() > later.GetProperty("fromMs").GetDouble()
                || earlier.GetProperty("droppedRecords").GetInt32() > later.GetProperty("droppedRecords").GetInt32())
                throw new InvalidDataException("Telemetry timeline or closure conflicts.");
            var nextSequence = (long)earlier.GetProperty("firstSequence").GetInt32() + earlier.GetProperty("records").GetArrayLength();
            var laterSequence = later.GetProperty("firstSequence").GetInt32();
            if (nextSequence > laterSequence || (Math.Abs((long)otherRevision - revision) == 1 && nextSequence != laterSequence))
                throw new InvalidDataException("Telemetry sequence range conflicts.");
        }
    }
}
