using System.Security.Cryptography;
using System.Text;
using Microsoft.EntityFrameworkCore;
using Tonepie.Server.Api;
using Tonepie.Server.Data;

var builder = WebApplication.CreateBuilder(args);
builder.Services.AddTonepieDb(builder.Configuration);
builder.Services.AddSingleton(TimeProvider.System);
builder.Services.AddScoped<IngestService>();
builder.Services.AddScoped<HistoryService>();
// The ESP's home page (served by the ESP itself, another origin) reads the history from the browser.
builder.Services.AddCors(o => o.AddDefaultPolicy(p => p.AllowAnyOrigin().AllowAnyHeader().WithMethods("GET")));
builder.Services.ConfigureHttpJsonOptions(o => o.SerializerOptions.PropertyNamingPolicy = System.Text.Json.JsonNamingPolicy.CamelCase);

var app = builder.Build();
app.UseCors();

using (var scope = app.Services.CreateScope())
    await scope.ServiceProvider.GetRequiredService<TonepieDb>().Database.MigrateAsync();

var apiKey = app.Configuration["Tonepie:ApiKey"] ?? "";
if (apiKey.Length < 16)
    app.Logger.LogWarning("Tonepie:ApiKey missing or shorter than 16 characters: /api/ingest refuses every request");

bool Authorized(HttpRequest request)
{
    var header = request.Headers.Authorization.ToString();
    if (apiKey.Length < 16 || !header.StartsWith("Bearer ", StringComparison.Ordinal)) return false;
    return CryptographicOperations.FixedTimeEquals(Encoding.UTF8.GetBytes(header[7..].Trim()), Encoding.UTF8.GetBytes(apiKey));
}

app.MapGet("/health", () => Results.Ok(new { ok = true }));

app.MapPost("/api/ingest", async (HttpRequest request, Snapshot snapshot, IngestService ingest, CancellationToken ct) =>
{
    if (!Authorized(request)) return Results.Json(new { message = "invalid API key" }, statusCode: 401);
    if (IngestService.Validate(snapshot) is { } error) return Results.BadRequest(new { message = error });
    var changed = await ingest.StoreAsync(snapshot, ct);
    return Results.Ok(new { ok = true, changed });
});

app.MapGet("/api/history", async (string device, int? days, HistoryService history, CancellationToken ct) =>
    await history.GetAsync(device, Math.Clamp(days ?? 90, 0, 3660), ct) is { } h
        ? Results.Ok(h)
        : Results.NotFound(new { message = "unknown device" }));

app.MapGet("/api/devices", async (HistoryService history, CancellationToken ct) => Results.Ok(await history.DevicesAsync(ct)));

app.Run();

public partial class Program;
