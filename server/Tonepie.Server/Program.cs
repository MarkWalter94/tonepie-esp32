using System.Security.Cryptography;
using System.Text;
using Microsoft.AspNetCore.Http.Features;
using Microsoft.EntityFrameworkCore;
using Tonepie.Server.Api;
using Tonepie.Server.Data;

// Docker HEALTHCHECK: the aspnet image has no curl, so the app probes its own /health.
if (args is ["--healthcheck"])
{
    var port = (Environment.GetEnvironmentVariable("ASPNETCORE_HTTP_PORTS") ?? "8090").Split(',', ';')[0].Trim();
    using var http = new HttpClient { Timeout = TimeSpan.FromSeconds(5) };
    try { return (await http.GetAsync($"http://localhost:{port}/health")).IsSuccessStatusCode ? 0 : 1; }
    catch (Exception) { return 1; }
}

const long MaxIngestBytes = 64 * 1024; // a full snapshot is ~6 KB
const string ExampleKey = "change-me-to-a-long-random-string";

var builder = WebApplication.CreateBuilder(args);
builder.Services.AddTonepieDb(builder.Configuration);
builder.Services.AddSingleton(TimeProvider.System);
builder.Services.AddSingleton<LocalTimeZone>();
builder.Services.AddSingleton<DeviceLocks>();
builder.Services.AddScoped<IngestService>();
builder.Services.AddScoped<HistoryService>();
builder.Services.ConfigureHttpJsonOptions(o => o.SerializerOptions.PropertyNamingPolicy = System.Text.Json.JsonNamingPolicy.CamelCase);

var app = builder.Build();
app.Services.GetRequiredService<LocalTimeZone>(); // fails fast on an invalid Tonepie:TimeZone

using (var scope = app.Services.CreateScope())
    await scope.ServiceProvider.GetRequiredService<TonepieDb>().Database.MigrateAsync();

var apiKey = app.Configuration["Tonepie:ApiKey"] ?? "";
var keyUsable = apiKey.Length >= 16 && apiKey != ExampleKey;
if (!keyUsable)
    app.Logger.LogWarning("Tonepie:ApiKey missing, shorter than 16 characters or still the example value: /api refuses every request");

bool Authorized(HttpRequest request)
{
    var header = request.Headers.Authorization.ToString();
    if (!keyUsable || !header.StartsWith("Bearer ", StringComparison.Ordinal)) return false;
    return CryptographicOperations.FixedTimeEquals(Encoding.UTF8.GetBytes(header[7..].Trim()), Encoding.UTF8.GetBytes(apiKey));
}

// Before routing binds anything: an anonymous caller never gets its body read, and learns nothing from it.
// The ESP relays the history to its home page, so reads need the key too.
app.Use(async (context, next) =>
{
    var request = context.Request;
    if (!request.Path.StartsWithSegments("/api"))
    {
        await next(context);
        return;
    }
    if (!Authorized(request))
    {
        context.Response.StatusCode = StatusCodes.Status401Unauthorized;
        await context.Response.WriteAsJsonAsync(new { message = "invalid API key" });
        return;
    }
    if (request.ContentLength > MaxIngestBytes)
    {
        context.Response.StatusCode = StatusCodes.Status413PayloadTooLarge;
        await context.Response.WriteAsJsonAsync(new { message = $"body larger than {MaxIngestBytes} bytes" });
        return;
    }
    // Chunked bodies have no length up front: Kestrel stops them at the limit while reading.
    if (context.Features.Get<IHttpMaxRequestBodySizeFeature>() is { IsReadOnly: false } limit)
        limit.MaxRequestBodySize = MaxIngestBytes;
    await next(context);
});

app.MapGet("/health", async (TonepieDb db, CancellationToken ct) =>
    await db.Database.CanConnectAsync(ct)
        ? Results.Ok(new { ok = true })
        : Results.Json(new { ok = false, message = "database unreachable" }, statusCode: StatusCodes.Status503ServiceUnavailable));

app.MapPost("/api/ingest", async (Snapshot snapshot, IngestService ingest, CancellationToken ct) =>
    ingest.Validate(snapshot) is { } error
        ? Results.BadRequest(new { message = error })
        : Results.Ok(new { ok = true, changed = await ingest.StoreAsync(snapshot, ct) }));

app.MapGet("/api/history", async (string device, int? days, HistoryService history, CancellationToken ct) =>
    await history.GetAsync(device, Math.Clamp(days ?? 90, 0, 3660), ct) is { } h
        ? Results.Ok(h)
        : Results.NotFound(new { message = "unknown device" }));

app.MapGet("/api/devices", async (HistoryService history, CancellationToken ct) => Results.Ok(await history.DevicesAsync(ct)));

await app.RunAsync();
return 0;

public partial class Program;
