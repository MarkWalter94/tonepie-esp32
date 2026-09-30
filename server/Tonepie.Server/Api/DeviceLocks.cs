using System.Collections.Concurrent;

namespace Tonepie.Server.Api;

/// <summary>
/// Serializes ingests per device: two snapshots of the same device stored at once would both insert
/// the same new rows (device, visits, events) and one would fail on the primary key.
/// In-process only: the server runs as a single instance.
/// </summary>
public sealed class DeviceLocks
{
    readonly ConcurrentDictionary<string, SemaphoreSlim> _locks = new(StringComparer.Ordinal);

    public async Task<IDisposable> AcquireAsync(string deviceId, CancellationToken ct)
    {
        // One semaphore per device, never removed: devices are few and authenticated.
        var gate = _locks.GetOrAdd(deviceId, _ => new SemaphoreSlim(1, 1));
        await gate.WaitAsync(ct);
        return new Release(gate);
    }

    sealed class Release(SemaphoreSlim gate) : IDisposable
    {
        int _done;
        public void Dispose()
        {
            if (Interlocked.Exchange(ref _done, 1) == 0) gate.Release();
        }
    }
}

/// <summary>The configured Tonepie:TimeZone, resolved once so that a typo stops the start-up instead of every read.</summary>
public sealed class LocalTimeZone
{
    public TimeZoneInfo Zone { get; }

    public LocalTimeZone(IConfiguration config)
    {
        var id = config["Tonepie:TimeZone"];
        if (string.IsNullOrWhiteSpace(id)) id = "Europe/Rome";
        try
        {
            Zone = TimeZoneInfo.FindSystemTimeZoneById(id.Trim());
        }
        catch (Exception e) when (e is TimeZoneNotFoundException or InvalidTimeZoneException)
        {
            throw new InvalidOperationException($"Invalid Tonepie:TimeZone '{id}': use an IANA id such as Europe/Rome", e);
        }
    }
}
