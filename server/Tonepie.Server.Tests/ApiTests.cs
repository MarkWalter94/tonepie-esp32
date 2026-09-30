using System.Net;
using System.Net.Http.Headers;
using System.Net.Http.Json;
using System.Text;
using Microsoft.AspNetCore.Mvc.Testing;
using Microsoft.AspNetCore.TestHost;
using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Time.Testing;
using Tonepie.Server.Api;
using Tonepie.Server.Data;

namespace Tonepie.Server.Tests;

public sealed class ApiTests : IDisposable
{
    const string Key = "test-key-0123456789abcdef";
    readonly List<string> _dbs = [];
    readonly List<IDisposable> _owned = [];
    readonly WebApplicationFactory<Program> _factory;
    readonly HttpClient _client;
    readonly long _now = DateTimeOffset.UtcNow.ToUnixTimeSeconds();

    public ApiTests() => (_factory, _client) = Start();

    /// <summary>A server on its own SQLite file; the client sends the key.</summary>
    (WebApplicationFactory<Program>, HttpClient) Start(TimeProvider? clock = null, string key = Key, string zone = "Europe/Rome")
    {
        var db = Path.Combine(Path.GetTempPath(), $"tonepie-test-{Guid.NewGuid():N}.db");
        _dbs.Add(db);
        var factory = new WebApplicationFactory<Program>().WithWebHostBuilder(b =>
        {
            b.UseSetting("ConnectionStrings:Tonepie", $"Data Source={db};Pooling=False")
                .UseSetting("Tonepie:Provider", "Sqlite")
                .UseSetting("Tonepie:ApiKey", key)
                .UseSetting("Tonepie:TimeZone", zone);
            if (clock is not null) b.ConfigureTestServices(s => s.AddSingleton(clock));
        });
        _owned.Add(factory);
        var client = factory.CreateClient();
        _owned.Add(client);
        client.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue("Bearer", key);
        return (factory, client);
    }

    public void Dispose()
    {
        foreach (var o in Enumerable.Reverse(_owned)) o.Dispose();
        foreach (var db in _dbs) File.Delete(db);
    }

    static readonly List<SnapshotCat> Cats = [new(0, "Ellie", 0, 4200), new(1, "Flipper", 1, 5600)];

    Snapshot Snap(List<SnapshotVisit> visits, List<SnapshotCat>? cats = null, List<VisitRef>? deleted = null, long bagAt = 0) =>
        new("tonepie-abc123", "1.9.0", _now, cats ?? Cats, new SnapshotBin(bagAt, visits.Count, 30), 0, visits, deleted);

    async Task<int> Post(Snapshot s, HttpClient? client = null)
    {
        var r = await (client ?? _client).PostAsJsonAsync("/api/ingest", s);
        Assert.Equal(HttpStatusCode.OK, r.StatusCode);
        return (await r.Content.ReadFromJsonAsync<Dictionary<string, object>>())!["changed"].ToString() is { } c ? int.Parse(c) : -1;
    }

    Task<HttpResponseMessage> PostRaw(string json, HttpClient? client = null) =>
        (client ?? _client).PostAsync("/api/ingest", new StringContent(json, Encoding.UTF8, "application/json"));

    Task<History?> Get(int days = 30, HttpClient? client = null) =>
        (client ?? _client).GetFromJsonAsync<History>($"/api/history?device=tonepie-abc123&days={days}");

    async Task<int> StoredVisits(string device = "tonepie-abc123") =>
        (await _client.GetFromJsonAsync<List<DeviceInfo>>("/api/devices"))!.Single(d => d.Id == device).Visits;

    static int Count(History h, string cat) => h.Days.Sum(d => d.V.GetValueOrDefault(cat));

    static List<SnapshotVisit> Series(IEnumerable<int> ids, long from) =>
        [.. ids.Select((id, i) => new SnapshotVisit(id, from + i * 60, 4200, 40, 0, false))];

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
    public async Task Rejects_invalid_fields()
    {
        var ok = Snap([new(1, _now - 60, 4200, 40, 0, false)]);
        Snapshot[] bad =
        [
            ok with { Device = "bad id!" },
            ok with { Device = new string('a', 41) },
            ok with { Cats = [new(0, "A", 0, 4000), new(0, "B", 1, 5000)] }, // duplicate index
            ok with { Cats = [new(0, "A", 0, -1)] },
            ok with { Visits = [new(1, 1000, 4200, 40, 0, false)] },          // before any real clock
            ok with { Visits = [new(1, _now + 2 * 86400, 4200, 40, 0, false)] },
            ok with { Visits = [new(0, _now - 60, 4200, 40, 0, false)] },
            ok with { Visits = [new(1, _now - 60, -1, 40, 0, false)] },
            ok with { Visits = [new(1, _now - 60, 4200, -1, 0, false)] },
            ok with { Deleted = [new(0, _now - 60)] },
            ok with { Deleted = [new(1, -1)] },
            ok with { Bin = new SnapshotBin(-1, 0, 30) },
            ok with { Bin = new SnapshotBin(0, -1, 30) },
            ok with { Bin = new SnapshotBin(0, 0, -1) },
            ok with { LitterAt = -1 },
        ];
        foreach (var s in bad)
            Assert.Equal(HttpStatusCode.BadRequest, (await _client.PostAsJsonAsync("/api/ingest", s)).StatusCode);
        Assert.Equal(1, await Post(ok));
    }

    [Theory]
    [InlineData("""{"device":"tonepie-abc123","cats":[null]}""")]
    [InlineData("""{"device":"tonepie-abc123","visits":[null]}""")]
    [InlineData("""{"device":"tonepie-abc123","deleted":[null]}""")]
    public async Task Null_list_elements_are_bad_requests(string json) =>
        Assert.Equal(HttpStatusCode.BadRequest, (await PostRaw(json)).StatusCode);

    [Fact]
    public async Task Checks_the_key_before_reading_the_body()
    {
        using var anonymous = _factory.CreateClient();
        Assert.Equal(HttpStatusCode.Unauthorized, (await PostRaw("{not json", anonymous)).StatusCode);
        Assert.Equal(HttpStatusCode.BadRequest, (await PostRaw("{not json")).StatusCode);
        var huge = $$"""{"device":"tonepie-abc123","firmware":"{{new string('x', 70_000)}}"}""";
        Assert.Equal(HttpStatusCode.RequestEntityTooLarge, (await PostRaw(huge)).StatusCode);
    }

    [Fact]
    public async Task Reads_need_the_key()
    {
        await Post(Snap([new(1, _now - 60, 4200, 40, 0, false)]));
        using var anonymous = _factory.CreateClient();
        Assert.Equal(HttpStatusCode.Unauthorized, (await anonymous.GetAsync("/api/history?device=tonepie-abc123")).StatusCode);
        Assert.Equal(HttpStatusCode.Unauthorized, (await anonymous.GetAsync("/api/devices")).StatusCode);
        Assert.Equal(HttpStatusCode.OK, (await anonymous.GetAsync("/health")).StatusCode);
        Assert.False((await anonymous.GetAsync("/api/history?device=tonepie-abc123")).Headers.Contains("Access-Control-Allow-Origin"));
        Assert.Equal(1, await StoredVisits());
    }

    [Fact]
    public async Task Refuses_the_example_key_and_an_invalid_time_zone()
    {
        var (_, client) = Start(key: "change-me-to-a-long-random-string");
        Assert.Equal(HttpStatusCode.Unauthorized, (await client.PostAsJsonAsync("/api/ingest", Snap([]))).StatusCode);
        Assert.ThrowsAny<Exception>(() => Start(zone: "Mars/Olympus_Mons"));
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
    public async Task Blank_cat_names_get_a_default_and_long_ones_are_cut()
    {
        List<SnapshotCat> cats = [new(0, "  ", 9, 4000), new(1, "A name much longer than twenty-four", -3, 5000)];
        await Post(Snap([new(1, _now - 60, 4000, 40, 0, false)], cats));
        var h = (await Get())!;
        Assert.Equal(["Cat 1", "A name much longer than "], h.Cats.Select(c => c.Name));
        Assert.Equal([5, 0], h.Cats.Select(c => c.Color)); // clamped to the ESP's palette
        Assert.Equal(1, Count(h, "Cat 1"));
    }

    [Fact]
    public async Task Missing_bin_keeps_the_stored_one()
    {
        await Post(Snap([], bagAt: _now - 86400));
        await Post(Snap([]) with { Bin = null });
        using var scope = _factory.Services.CreateScope();
        var device = await scope.ServiceProvider.GetRequiredService<TonepieDb>().Devices.SingleAsync();
        Assert.Equal((_now - 86400, 0, 30), (device.BinSince, device.BinVisits, device.BinLimitVisits));
    }

    [Fact]
    public async Task Numbers_restarting_after_an_erased_esp_keep_the_old_visits()
    {
        var old = Series(Enumerable.Range(1, 5), _now - 10 * 86400);
        old.Add(new(6, 0, 4200, 40, 0, false)); // no clock back then
        Assert.Equal(6, await Post(Snap(old)));

        // Erased ESP: numbers 1..8 again, with new times. Nothing stored may be taken for a deletion.
        Assert.Equal(8, await Post(Snap(Series(Enumerable.Range(1, 8), _now - 3600))));
        Assert.Equal(14, await StoredVisits());
        Assert.Equal(13, (await Get(0))!.Days.Sum(d => d.V.Values.Sum()));
    }

    [Fact]
    public async Task Gaps_are_looked_for_in_the_wrapped_id_window_only()
    {
        Assert.Equal(1, await Post(Snap([new(30000, _now - 1000, 4200, 40, 0, false)])));
        int[] ids = [65530, 65531, 65532, 65533, 65534, 65535, 1, 2, 3];
        var wrapped = Series(ids, _now - 7200);
        Assert.Equal(9, await Post(Snap(wrapped)));

        // 65533 and 2 missing from the middle: deleted. 30000 is newer but outside 65530..3: untouched.
        Assert.Equal(2, await Post(Snap([.. wrapped.Where(v => v.Id is not (65533 or 2))])));
        Assert.Equal(8, await StoredVisits());
    }

    [Fact]
    public void Window_follows_the_wrap()
    {
        Assert.Equal((1, 5), IngestService.Window([1, 2, 6]));
        Assert.Equal((65530, 25), IngestService.Window([65530, 65535, 1, 20]));
        Assert.Equal((7, 0), IngestService.Window([7]));
    }

    [Fact]
    public async Task Parallel_identical_posts_all_succeed()
    {
        var snap = Snap(Series(Enumerable.Range(1, 20), _now - 7200));
        var results = await Task.WhenAll(Enumerable.Range(0, 8).Select(_ => _client.PostAsJsonAsync("/api/ingest", snap)));
        Assert.All(results, r => Assert.Equal(HttpStatusCode.OK, r.StatusCode));
        results = await Task.WhenAll(Enumerable.Range(0, 8).Select(_ => _client.PostAsJsonAsync("/api/ingest", snap)));
        Assert.All(results, r => Assert.Equal(HttpStatusCode.OK, r.StatusCode));
        Assert.Equal(20, await StoredVisits());
    }

    [Fact]
    public async Task Parallel_first_posts_of_a_new_device_all_succeed()
    {
        var snap = Snap(Series(Enumerable.Range(1, 20), _now - 7200), bagAt: _now - 86400) with { Device = "tonepie-new", LitterAt = _now - 3600 };
        var changed = await Task.WhenAll(Enumerable.Range(0, 8).Select(_ => Post(snap)));
        Assert.Equal(20, changed.Sum()); // stored once
        Assert.Equal(20, await StoredVisits("tonepie-new"));
    }

    // Europe/Rome switches to summer time on 2026-03-29 (23-hour day) and back on 2026-10-25 (25 hours).
    [Theory]
    [InlineData("2026-03-29T22:00:00+02:00",
        "2026-03-27T23:50:00+01:00 2026-03-28T00:10:00+01:00 2026-03-28T23:30:00+01:00 2026-03-29T00:30:00+01:00 2026-03-29T03:30:00+02:00 2026-03-29T21:00:00+02:00",
        "2026-03-29=3", "2026-03-28=2 2026-03-29=3")]
    [InlineData("2026-10-25T23:30:00+01:00",
        "2026-10-23T23:50:00+02:00 2026-10-24T00:10:00+02:00 2026-10-24T23:50:00+02:00 2026-10-25T00:10:00+02:00 2026-10-25T02:30:00+02:00 2026-10-25T02:30:00+01:00 2026-10-25T23:20:00+01:00",
        "2026-10-25=4", "2026-10-24=2 2026-10-25=4")]
    public async Task Days_are_local_across_daylight_saving_changes(string now, string times, string oneDay, string twoDays)
    {
        var clock = new FakeTimeProvider(DateTimeOffset.Parse(now));
        var (_, client) = Start(clock);
        var visits = times.Split(' ').Select((t, i) => new SnapshotVisit(i + 1, DateTimeOffset.Parse(t).ToUnixTimeSeconds(), 4200, 40, 0, false)).ToList();
        await Post(Snap(visits), client);

        async Task<string> Days(int days) =>
            string.Join(' ', (await Get(days, client))!.Days.Select(d => $"{d.D}={d.V.Values.Sum()}"));
        Assert.Equal(oneDay, await Days(1));
        Assert.Equal(twoDays, await Days(2));
    }

    [Fact]
    public void Migrations_match_the_model()
    {
        TonepieDb[] contexts = [new SqliteFactory().CreateDbContext([]), new PostgresFactory().CreateDbContext([]), new SqlServerFactory().CreateDbContext([])];
        foreach (var db in contexts)
            using (db)
                Assert.False(db.Database.HasPendingModelChanges(), $"{db.GetType().Name}: add a migration");
    }
}
