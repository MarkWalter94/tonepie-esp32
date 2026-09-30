namespace Tonepie.Server.Api;

// What the ESP posts to /api/ingest: its whole current state (the last 64 visits at most).
// Sending a snapshot instead of single events makes retries harmless: the same data twice is one upsert.
// Checked by IngestService.Validate.
public record Snapshot(
    string Device,
    string? Firmware,
    long Now,
    List<SnapshotCat>? Cats,
    SnapshotBin? Bin,
    long LitterAt,
    List<SnapshotVisit>? Visits,
    List<VisitRef>? Deleted);

public record SnapshotCat(int Index, string? Name, int Color, int WeightG);
public record SnapshotBin(long Since, int Visits, int Limit);
public record SnapshotVisit(int Id, long T, int G, int S, int Cat, bool Manual);
public record VisitRef(int Id, long T);

// Answer of GET /api/history, aggregated per local day so the page stays light.
public record HistoryCat(string Name, int Color, int WeightG);
/// <summary>One local day: visits per cat name ("" = not recognised) and average weight in grams.</summary>
public record HistoryDay(string D, Dictionary<string, int> V, Dictionary<string, int> W);
public record History(
    string Device,
    string TimeZone,
    long FirstVisit,
    List<HistoryCat> Cats,
    List<HistoryDay> Days,
    List<long> Bags,
    List<long> Litter);

public record DeviceInfo(string Id, string Firmware, long FirstSeen, long LastSeen, int Visits);
