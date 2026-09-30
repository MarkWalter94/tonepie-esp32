using Microsoft.EntityFrameworkCore;
using Tonepie.Server.Data;

namespace Tonepie.Server.Api;

public class IngestService(TonepieDb db, TimeProvider clock)
{
    public const int MaxVisits = 64, MaxCats = 4;

    public static string? Validate(Snapshot s)
    {
        if (string.IsNullOrWhiteSpace(s.Device) || s.Device.Length > 40) return "device must be 1-40 characters";
        if (s.Firmware is { Length: > 40 }) return "firmware too long";
        if (s.Cats is { Count: > MaxCats }) return $"at most {MaxCats} cats";
        if (s.Cats?.Any(c => string.IsNullOrWhiteSpace(c.Name) || c.Name.Length > 24 || c.Index is < 0 or >= MaxCats) == true)
            return "invalid cat";
        if (s.Visits is { Count: > MaxVisits } || s.Deleted is { Count: > MaxVisits }) return $"at most {MaxVisits} visits";
        if (s.Visits?.Any(v => v.Id is < 0 or > 65535 || v.T < 0) == true) return "invalid visit";
        return null;
    }

    /// <summary>Stores a snapshot; returns how many visits were added or changed.</summary>
    public async Task<int> StoreAsync(Snapshot s, CancellationToken ct)
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
        device.BinSince = s.Bin?.Since ?? 0;
        device.BinVisits = s.Bin?.Visits ?? 0;
        device.BinLimitVisits = s.Bin?.Limit ?? 0;
        device.LitterAt = s.LitterAt;
        var cats = s.Cats ?? [];
        device.Cats.RemoveAll(c => cats.All(n => n.Index != c.Index));
        foreach (var c in cats)
        {
            var row = device.Cats.FirstOrDefault(x => x.Index == c.Index);
            if (row is null) device.Cats.Add(row = new DeviceCat { DeviceId = s.Device, Index = c.Index });
            row.Name = c.Name.Trim();
            row.Color = c.Color;
            row.WeightG = c.WeightG;
        }
        string? NameOf(int index) => cats.FirstOrDefault(c => c.Index == index)?.Name.Trim();

        var visits = (s.Visits ?? []).DistinctBy(v => (v.Id, v.T)).ToList();
        var changed = 0;
        if (visits.Count > 0 || s.Deleted is { Count: > 0 })
        {
            // Every stored visit the ESP could still be holding: ids within the snapshot's range.
            var ids = visits.Select(v => v.Id).Concat(s.Deleted?.Select(d => d.Id) ?? []).ToList();
            int lo = ids.Min(), hi = ids.Max();
            var stored = await db.Visits.Where(v => v.DeviceId == s.Device && v.EspId >= lo && v.EspId <= hi)
                .ToDictionaryAsync(v => (v.EspId, v.Epoch), ct);
            foreach (var v in visits)
            {
                var name = v.Cat >= 0 ? NameOf(v.Cat) : null;
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
            // than the snapshot simply left the ESP's memory and stay as they are.
            var present = visits.Select(v => (v.Id, v.T)).ToHashSet();
            var explicitly = s.Deleted?.Select(d => (d.Id, d.T)).ToHashSet() ?? [];
            var oldestTimed = visits.Where(v => v.T > 0).Select(v => v.T).DefaultIfEmpty(long.MaxValue).Min();
            var lowestShown = visits.Count > 0 ? visits.Min(v => v.Id) : int.MaxValue;
            foreach (var row in stored.Values.Where(r => !r.Deleted && !present.Contains((r.EspId, r.Epoch))))
            {
                var gap = row.EspId > lowestShown && (row.Epoch == 0 || row.Epoch >= oldestTimed);
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
}
