using Microsoft.EntityFrameworkCore;
using Tonepie.Server.Data;

namespace Tonepie.Server.Api;

public class HistoryService(TonepieDb db, TimeProvider clock, LocalTimeZone local)
{
    public TimeZoneInfo Zone => local.Zone;

    /// <summary>Visits per local day and cat over the last <paramref name="days"/> days (0 = everything).</summary>
    public async Task<History?> GetAsync(string deviceId, int days, CancellationToken ct)
    {
        var device = await db.Devices.AsNoTracking().Include(d => d.Cats).FirstOrDefaultAsync(d => d.Id == deviceId, ct);
        if (device is null) return null;
        long from = 1;
        if (days > 0)
        {
            var today = TimeZoneInfo.ConvertTime(clock.GetUtcNow(), Zone).Date;
            var start = today.AddDays(1 - days);
            from = new DateTimeOffset(start, Zone.GetUtcOffset(start)).ToUnixTimeSeconds();
        }
        var visits = await db.Visits.AsNoTracking()
            .Where(v => v.DeviceId == deviceId && !v.Deleted && v.Epoch >= from)
            .Select(v => new { v.Epoch, v.CatName, v.WeightG })
            .ToListAsync(ct);
        var perDay = visits
            .GroupBy(v => DateOnly.FromDateTime(TimeZoneInfo.ConvertTime(DateTimeOffset.FromUnixTimeSeconds(v.Epoch), Zone).DateTime))
            .OrderBy(g => g.Key)
            .Select(g => new HistoryDay(
                g.Key.ToString("yyyy-MM-dd"),
                g.GroupBy(v => v.CatName ?? "").ToDictionary(c => c.Key, c => c.Count()),
                g.Where(v => v.CatName != null && v.WeightG > 0).GroupBy(v => v.CatName!)
                    .ToDictionary(c => c.Key, c => (int)Math.Round(c.Average(v => v.WeightG)))))
            .ToList();
        var first = await db.Visits.Where(v => v.DeviceId == deviceId && !v.Deleted && v.Epoch > 0)
            .MinAsync(v => (long?)v.Epoch, ct) ?? 0;
        var events = await db.Events.AsNoTracking().Where(e => e.DeviceId == deviceId && e.Epoch >= from)
            .OrderBy(e => e.Epoch).ToListAsync(ct);
        return new History(
            device.Id, Zone.Id, first,
            device.Cats.OrderBy(c => c.Index).Select(c => new HistoryCat(c.Name, c.Color, c.WeightG)).ToList(),
            perDay,
            events.Where(e => e.Kind == MaintenanceKind.BagChange).Select(e => e.Epoch).ToList(),
            events.Where(e => e.Kind == MaintenanceKind.LitterAdded).Select(e => e.Epoch).ToList());
    }

    public async Task<List<DeviceInfo>> DevicesAsync(CancellationToken ct) =>
        await db.Devices.AsNoTracking().OrderBy(d => d.Id)
            .Select(d => new DeviceInfo(d.Id, d.Firmware, d.FirstSeen, d.LastSeen,
                db.Visits.Count(v => v.DeviceId == d.Id && !v.Deleted)))
            .ToListAsync(ct);
}
