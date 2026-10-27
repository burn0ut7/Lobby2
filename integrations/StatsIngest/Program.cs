using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using Npgsql;
using NpgsqlTypes;

if (args.Length != 2 || args[0] is not ("--once" or "--validate"))
{
    Console.Error.WriteLine("Usage: StatsIngest --once|--validate <Lobby2Stats outbox directory>");
    return 2;
}

var directory = Path.GetFullPath(args[1]);
if (!Directory.Exists(directory))
{
    Console.Error.WriteLine("Outbox directory does not exist.");
    return 2;
}

var connectionString = Environment.GetEnvironmentVariable("LOBBY2_STATS_CONNECTION_STRING");
if (args[0] == "--once" && string.IsNullOrWhiteSpace(connectionString))
{
    Console.Error.WriteLine("Set LOBBY2_STATS_CONNECTION_STRING for the PostgreSQL ingestion account.");
    return 2;
}

// Exclusive sidecar ownership prevents two local uploaders deleting each other's pending files.
using var ownership = new FileStream(Path.Combine(directory, ".ingest.lock"), FileMode.OpenOrCreate,
    FileAccess.ReadWrite, FileShare.None);
await using var database = args[0] == "--once" ? NpgsqlDataSource.Create(connectionString!) : null;
var failures = 0;
foreach (var marker in Directory.EnumerateFiles(directory, "*.json.ready"))
{
    var path = marker[..^".ready".Length];
    try
    {
        if (await File.ReadAllTextAsync(marker) != "ready")
            throw new InvalidDataException("Incomplete ready marker.");
        var info = new FileInfo(path);
        if (info.Length is <= 0 or > 64 * 1024 * 1024)
            throw new InvalidDataException("Batch exceeds the 64 MiB bound or is empty.");
        // Strict UTF-8 preserves identity/name data and rejects incomplete encodings.
        var bytes = await File.ReadAllBytesAsync(path);
        var payload = new UTF8Encoding(false, true).GetString(bytes);
        using var json = JsonDocument.Parse(payload);
        var root = json.RootElement;
        BatchContract.Validate(root);
        if (root.GetProperty("schemaVersion").GetInt32() != 1)
            throw new InvalidDataException("Unsupported schema version.");
        var mission = root.GetProperty("missionId").GetGuid();
        var revision = root.GetProperty("revision").GetInt32();
        if (revision < 0 || Path.GetFileName(path) != $"{mission}-{revision}.json")
            throw new InvalidDataException("Batch identity differs from its filename.");
        if (root.GetProperty("players").GetArrayLength() > 1024 || root.GetProperty("deaths").GetArrayLength() > 256)
            throw new InvalidDataException("Batch exceeds record bounds.");
        var checksum = Convert.ToHexString(SHA256.HashData(bytes));
        if (database is null)
        {
            Console.WriteLine($"VALID {mission} revision={revision}");
            continue;
        }

        await using var connection = await database.OpenConnectionAsync();
        await using var transaction = await connection.BeginTransactionAsync();
        await using (var insert = new NpgsqlCommand("""
            INSERT INTO lobby2.mission_batches(mission_id, revision, checksum, payload)
            VALUES ($1, $2, $3, $4) ON CONFLICT (mission_id, revision) DO NOTHING
            """, connection, transaction))
        {
            insert.Parameters.AddWithValue(mission);
            insert.Parameters.AddWithValue(revision);
            insert.Parameters.AddWithValue(checksum);
            insert.Parameters.AddWithValue(NpgsqlDbType.Jsonb, payload);
            await insert.ExecuteNonQueryAsync();
        }
        await using (var check = new NpgsqlCommand(
            "SELECT checksum FROM lobby2.mission_batches WHERE mission_id=$1 AND revision=$2", connection, transaction))
        {
            check.Parameters.AddWithValue(mission);
            check.Parameters.AddWithValue(revision);
            var existing = (string?)await check.ExecuteScalarAsync();
            if (!string.Equals(existing, checksum, StringComparison.Ordinal))
                throw new InvalidDataException("Conflicting payload for an existing batch identity; local batch retained.");
        }
        await transaction.CommitAsync();
        // A crash before removal simply replays the identical idempotent insert next run.
        File.Delete(marker);
        File.Delete(path);
        Console.WriteLine($"STORED {mission} revision={revision}");
    }
    catch (Exception exception) when (exception is IOException or InvalidDataException or JsonException or InvalidOperationException
        or FormatException or KeyNotFoundException or NpgsqlException or DecoderFallbackException)
    {
        failures++;
        // Never print database connection strings, credentials, or raw player payloads.
        Console.Error.WriteLine($"RETAINED {Path.GetFileName(path)}: {exception.GetType().Name}");
    }
}
return failures == 0 ? 0 : 1;
