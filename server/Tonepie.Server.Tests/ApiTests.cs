using System.Net;
using System.Net.Http.Headers;
using System.Net.Http.Json;
using Microsoft.AspNetCore.Mvc.Testing;
using Tonepie.Server.Api;

namespace Tonepie.Server.Tests;

public sealed class ApiTests : IDisposable
{
    const string Key = "test-key-0123456789abcdef";
    readonly string _db = Path.Combine(Path.GetTempPath(), $"tonepie-test-{Guid.NewGuid():N}.db");
    readonly WebApplicationFactory<Program> _factory;
    readonly HttpClient _client;
    readonly long _now = DateTimeOffset.UtcNow.ToUnixTimeSeconds();

    public ApiTests()
    {
        _factory = new WebApplicationFactory<Program>().WithWebHostBuilder(b => b
            .UseSetting("ConnectionStrings:Tonepie", $"Data Source={_db};Pooling=False")
            .UseSetting("Tonepie:Provider", "Sqlite")
            .UseSetting("Tonepie:ApiKey", Key)
            .UseSetting("Tonepie:TimeZone", "Europe/Rome"));
        _client = _factory.CreateClient();
        _client.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue("Bearer", Key);
    }

    public void Dispose()
    {
        _client.Dispose();
        _factory.Dispose();
        File.Delete(_db);
    }

    static readonly List<SnapshotCat> Cats = [new(0, "Ellie", 0, 4200), new(1, "Flipper", 1, 5600)];

    Snapshot Snap(List<SnapshotVisit> visits, List<SnapshotCat>? cats = null, List<VisitRef>? deleted = null, long bagAt = 0) =>
        new("tonepie-abc123", "1.9.0", _now, cats ?? Cats, new SnapshotBin(bagAt, visits.Count, 30), 0, visits, deleted);

    async Task<int> Post(Snapshot s)
    {
        var r = await _client.PostAsJsonAsync("/api/ingest", s);
        Assert.Equal(HttpStatusCode.OK, r.StatusCode);
        return (await r.Content.ReadFromJsonAsync<Dictionary<string, object>>())!["changed"].ToString() is { } c ? int.Parse(c) : -1;
    }

    Task<History?> Get(int days = 30) => _client.GetFromJsonAsync<History>($"/api/history?device=tonepie-abc123&days={days}");

    static int Count(History h, string cat) => h.Days.Sum(d => d.V.GetValueOrDefault(cat));

    [Fact]
    public async Task Rejects_missing_or_wrong_key_and_invalid_data()
    {
        using var anonymous = _factory.CreateClient();
        Assert.Equal(HttpStatusCode.Unauthorized, (await anonymous.PostAsJsonAsync("/api/ingest", Snap([]))).StatusCode);
        anonymous.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue("Bearer", Key + "x");
        Assert.Equal(HttpStatusCode.Unauthorized, (await anonymous.PostAsJsonAsync("/api/ingest", Snap([]))).StatusCode);
        var five = Enumerable.Range(0, 5).Select(i => new SnapshotCat(i % 4, "C" + i, 0, 4000)).ToList();
        Assert.Equal(HttpStatusCode.BadRequest, (await _client.PostAsJsonAsync("/api/ingest", Snap([], five))).StatusCode);
        Assert.Equal(HttpStatusCode.NotFound, (await _client.GetAsync("/api/history?device=nobody")).StatusCode);
    }

    [Fact]
    public async Task Stores_visits_per_day_and_cat_idempotently()
    {
        List<SnapshotVisit> visits =
        [
            new(1, _now - 3 * 86400, 4100, 60, 0, false),
            new(2, _now - 3600, 4300, 50, 0, false),
            new(3, _now - 600, 5600, 70, 1, false),
            new(4, _now - 300, 0, 0, -1, false),
            new(5, 0, 0, 0, -1, false), // no clock: stored, never on a day
        ];
        Assert.Equal(5, await Post(Snap(visits, bagAt: _now - 5 * 86400)));
        Assert.Equal(0, await Post(Snap(visits, bagAt: _now - 5 * 86400))); // a retry changes nothing

        var h = (await Get())!;
        Assert.Equal(["Ellie", "Flipper"], h.Cats.Select(c => c.Name));
        Assert.Equal(2, Count(h, "Ellie"));
        Assert.Equal(1, Count(h, "Flipper"));
        Assert.Equal(1, Count(h, ""));
        Assert.Contains(h.Days, d => d.W.GetValueOrDefault("Ellie") == 4100);
        Assert.Single(h.Bags);
        Assert.Equal(_now - 3 * 86400, h.FirstVisit);
        var rome = TimeZoneInfo.FindSystemTimeZoneById("Europe/Rome");
        var today = TimeZoneInfo.ConvertTime(DateTimeOffset.UtcNow, rome).ToString("yyyy-MM-dd");
        var expected = visits.Count(v => v.T > 0 && TimeZoneInfo.ConvertTime(DateTimeOffset.FromUnixTimeSeconds(v.T), rome).ToString("yyyy-MM-dd") == today);
        var h1 = (await Get(1))!;
        Assert.All(h1.Days, d => Assert.Equal(today, d.D)); // days=1: today only, in the configured time zone
        Assert.Equal(expected, h1.Days.Sum(d => d.V.Values.Sum()));
    }

    [Fact]
    public async Task Follows_corrections_deletions_and_renumbered_cats()
    {
        List<SnapshotVisit> visits = [.. Enumerable.Range(1, 6).Select(i => new SnapshotVisit(i, _now - 7200 + i * 60, 4200, 40, 0, false))];
        await Post(Snap(visits));

        // Visit 3 reassigned by hand, visit 4 deleted (gap), visit 1 deleted (explicit: it was the oldest one).
        visits[2] = visits[2] with { Cat = 1, Manual = true };
        var later = visits.Where(v => v.Id != 4 && v.Id != 1).ToList();
        Assert.Equal(3, await Post(Snap(later, deleted: [new(1, visits[0].T)])));
        var h = (await Get())!;
        Assert.Equal(3, Count(h, "Ellie"));
        Assert.Equal(1, Count(h, "Flipper"));

        // Ellie removed on the ESP: Flipper becomes index 0. Visits keep their names, old ones leave the ESP's memory.
        var renumbered = later.Where(v => v.Id >= 5).Select(v => v with { Cat = -1 }).Append(new SnapshotVisit(7, _now - 60, 5600, 50, 0, false)).ToList();
        await Post(Snap(renumbered, [new(0, "Flipper", 1, 5600)]));
        h = (await Get())!;
        Assert.Equal(["Flipper"], h.Cats.Select(c => c.Name));
        Assert.Equal(2, Count(h, "Flipper")); // visit 3 (kept from before) and visit 7
        Assert.Equal(1, Count(h, "Ellie"));   // visit 2 left the ESP's memory: history keeps it
        Assert.Equal(2, Count(h, ""));        // visits 5 and 6 unassigned on the ESP now
    }

    [Fact]
    public async Task History_is_readable_from_other_origins()
    {
        await Post(Snap([new(1, _now - 60, 4200, 40, 0, false)]));
        var request = new HttpRequestMessage(HttpMethod.Get, "/api/history?device=tonepie-abc123");
        request.Headers.Add("Origin", "http://192.168.178.94");
        var r = await _factory.CreateClient().SendAsync(request);
        Assert.Equal("*", r.Headers.GetValues("Access-Control-Allow-Origin").Single());
        var devices = await _client.GetFromJsonAsync<List<DeviceInfo>>("/api/devices");
        Assert.Equal(1, devices!.Single().Visits);
    }
}
