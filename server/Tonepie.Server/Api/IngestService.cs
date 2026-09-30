using System.Text.RegularExpressions;
using Microsoft.Data.SqlClient;
using Microsoft.Data.Sqlite;
using Microsoft.EntityFrameworkCore;
using Npgsql;
using Tonepie.Server.Data;

namespace Tonepie.Server.Api;

public partial class IngestService(TonepieDb db, TimeProvider clock, DeviceLocks locks)
{
    public const int MaxVisits = 64, MaxCats = 4, MaxName = 24, MaxColor = 5;
    // The ESP numbers visits 1..65535, then starts again from 1.
    const int IdCount = 65535;
    // Earlier than any real ESP clock (2020-09-13): the ESP sends 0 when it had no time.
    const long MinEpoch = 1_600_000_000;

    [GeneratedRegex("^[A-Za-z0-9_-]{1,40}$")]
    private static partial Regex DeviceId();

    /// <summary>Returns the reason the snapshot is refused, or null when it can be stored.</summary>
    public string? Validate(Snapshot s)
    {
        var maxEpoch = clock.GetUtcNow().ToUnixTimeSeconds() + 86400;
        if (s.Device is null || !DeviceId().IsMatch(s.Device)) return "device must be 1-40 characters among A-Z a-z 0-9 _ -";
        if (s.Firmware is { Length: > 40 }) return "firmware too long";
        if (s.LitterAt < 0) return "invalid litterAt";
        if (s.Bin is { Since: < 0 } or { Visits: < 0 } or { Limit: < 0 }) return "invalid bin";
        if (s.Cats is { Count: > MaxCats }) return $"at most {MaxCats} cats";
        if (s.Cats?.Any(c => c is null || c.Index is < 0 or >= MaxCats || c.WeightG < 0) == true) return "invalid cat";
        if (s.Cats is { } cats && cats.DistinctBy(c => c.Index).Count() != cats.Count) return "duplicate cat index";
        if (s.Visits is { Count: > MaxVisits } || s.Deleted is { Count: > MaxVisits }) return $"at most {MaxVisits} visits";
        if (s.Visits?.Any(v => v is null || v.Id is < 1 or > IdCount || v.G < 0 || v.S < 0
                || (v.T != 0 && (v.T < MinEpoch || v.T > maxEpoch))) == true)
            return "invalid visit";
        if (s.Deleted?.Any(d => d is null || d.Id is < 1 or > IdCount || d.T < 0) == true) return "invalid deleted visit";
        return null;
    }

    /// <summary>Stores a validated snapshot; returns how many visits were added or changed.</summary>
    public async Task<int> StoreAsync(Snapshot s, CancellationToken ct)
    {
        using var _ = await locks.AcquireAsync(s.Device, ct);
        try
        {
            return await StoreOnceAsync(s, ct);
        }
        catch (DbUpdateException e) when (IsUniqueViolation(e))
        {
            // The lock only covers this process: if something else inserted the same rows first,
            // start over from what is stored now.
            db.ChangeTracker.Clear();
            return await StoreOnceAsync(s, ct);
        }
    }

    static bool IsUniqueViolation(DbUpdateException e) => e.InnerException switch
    {
        SqliteException x => x.SqliteExtendedErrorCode is 1555 or 2067, // SQLITE_CONSTRAINT_PRIMARYKEY / _UNIQUE
        PostgresException x => x.SqlState == PostgresErrorCodes.UniqueViolation,
        SqlException x => x.Number is 2601 or 2627,
        _ => false,
    };

    async Task<int> StoreOnceAsync(Snapshot s, CancellationToken ct)
    {
        var now = clock.GetUtcNow().ToUnixTimeSeconds();
        var device = await db.Devices.Include(d => d.Cats).FirstOrDefaultAsync(d => d.Id == s.Device, ct);
        if (device is null)
        {
            device = new Device { Id = s.Device, FirstSeen = now };
            db.Devices.Add(device);
        }
        device.Firmware = s.Firmware ?? "";
        device.LastSeen = now;
        // Snapshots without a bin (older firmware, partial state) keep the last known one.
        if (s.Bin is { } bin) (device.BinSince, device.BinVisits, device.BinLimitVisits) = (bin.Since, bin.Visits, bin.Limit);
        device.LitterAt = s.LitterAt;

        // A blank name must not block the sync forever: it gets the name the ESP shows for an unnamed cat.
        var cats = (s.Cats ?? []).ToDictionary(c => c.Index, c => c.Name?.Trim() is { Length: > 0 } n
            ? (n.Length > MaxName ? n[..MaxName] : n)
            : $"Cat {c.Index + 1}");
        device.Cats.RemoveAll(c => !cats.ContainsKey(c.Index));
        foreach (var c in s.Cats ?? [])
        {
            var row = device.Cats.FirstOrDefault(x => x.Index == c.Index);
            if (row is null) device.Cats.Add(row = new DeviceCat { DeviceId = s.Device, Index = c.Index });
            row.Name = cats[c.Index];
            row.Color = Math.Clamp(c.Color, 0, MaxColor);
            row.WeightG = c.WeightG;
        }

        var visits = (s.Visits ?? []).DistinctBy(v => (v.Id, v.T)).ToList();
        var changed = 0;
        if (visits.Count > 0 || s.Deleted is { Count: > 0 })
        {
            // Every stored visit the ESP could still be holding: ids within the snapshot's window, which
            // may wrap (65530..65535, 1..20). Only those rows are loaded.
            var (start, length) = Window(visits.Select(v => v.Id).Concat(s.Deleted?.Select(d => d.Id) ?? []));
            int end = start + length, hi1 = Math.Min(end, IdCount), hi2 = end - IdCount;
            var stored = await db.Visits
                .Where(v => v.DeviceId == s.Device && ((v.EspId >= start && v.EspId <= hi1) || (v.EspId >= 1 && v.EspId <= hi2)))
                .ToDictionaryAsync(v => (v.EspId, v.Epoch), ct);
            foreach (var v in visits)
            {
                var name = cats.GetValueOrDefault(v.Cat);
                if (!stored.TryGetValue((v.Id, v.T), out var row))
                {
                    db.Visits.Add(new Visit { DeviceId = s.Device, EspId = v.Id, Epoch = v.T, WeightG = v.G, DurationS = v.S,
                        CatName = name, Manual = v.Manual, UpdatedAt = now });
                    changed++;
                }
                else if (row.WeightG != v.G || row.DurationS != v.S || row.CatName != name || row.Manual != v.Manual || row.Deleted)
                {
                    (row.WeightG, row.DurationS, row.CatName, row.Manual, row.Deleted, row.UpdatedAt) = (v.G, v.S, name, v.Manual, false, now);
                    changed++;
                }
            }
            // Deleted on the ESP: listed explicitly, or missing from the middle of the snapshot. Visits older
            // than the snapshot simply left the ESP's memory and stay as they are; so do rows older than its
            // oldest timed visit (same numbers before an erased ESP restarted from 1) and rows without a time,
            // which cannot be placed and are removed only when listed explicitly.
            var present = visits.Select(v => (v.Id, v.T)).ToHashSet();
            var explicitly = s.Deleted?.Select(d => (d.Id, d.T)).ToHashSet() ?? [];
            var oldestTimed = visits.Where(v => v.T > 0).Select(v => v.T).DefaultIfEmpty(long.MaxValue).Min();
            var firstShown = visits.Count > 0 ? visits.Min(v => Offset(v.Id, start)) : int.MaxValue;
            foreach (var row in stored.Values.Where(r => !r.Deleted && !present.Contains((r.EspId, r.Epoch))))
            {
                var gap = Offset(row.EspId, start) > firstShown && row.Epoch > 0 && row.Epoch >= oldestTimed;
                if (gap || explicitly.Contains((row.EspId, row.Epoch)))
                {
                    row.Deleted = true;
                    row.UpdatedAt = now;
                    changed++;
                }
            }
        }

        async Task Event(MaintenanceKind kind, long epoch)
        {
            if (epoch > 0 && !await db.Events.AnyAsync(e => e.DeviceId == s.Device && e.Kind == kind && e.Epoch == epoch, ct))
                db.Events.Add(new MaintenanceEvent { DeviceId = s.Device, Kind = kind, Epoch = epoch });
        }
        await Event(MaintenanceKind.BagChange, device.BinSince);
        await Event(MaintenanceKind.LitterAdded, s.LitterAt);
        await db.SaveChangesAsync(ct);
        return changed;
    }

    /// <summary>
    /// The ids as a circular window: first id and distance to the last one. The window is the circle
    /// minus its biggest hole, so 65530..65535 + 1..20 gives (65530, 25), not 1..65535.
    /// </summary>
    public static (int Start, int Length) Window(IEnumerable<int> ids)
    {
        var sorted = ids.Distinct().Order().ToList();
        int start = sorted[0], length = sorted[^1] - sorted[0], hole = IdCount - length; // hole across the wrap
        for (var i = 1; i < sorted.Count; i++)
            if (sorted[i] - sorted[i - 1] > hole)
                (hole, start, length) = (sorted[i] - sorted[i - 1], sorted[i], IdCount - (sorted[i] - sorted[i - 1]));
        return (start, length);
    }

    static int Offset(int id, int start) => ((id - start) % IdCount + IdCount) % IdCount;
}
