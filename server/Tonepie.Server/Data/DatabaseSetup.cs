using Microsoft.EntityFrameworkCore;
using Microsoft.EntityFrameworkCore.Design;

namespace Tonepie.Server.Data;

public static class DatabaseSetup
{
    /// <summary>
    /// Registers the context for the configured provider ("Tonepie:Provider": Sqlite, Postgres or SqlServer)
    /// and exposes it as <see cref="TonepieDb"/> to the rest of the application.
    /// </summary>
    public static IServiceCollection AddTonepieDb(this IServiceCollection services, IConfiguration config)
    {
        var provider = config["Tonepie:Provider"] ?? "Sqlite";
        var cs = config.GetConnectionString("Tonepie") ?? "Data Source=tonepie.db";
        switch (provider.ToLowerInvariant())
        {
            case "sqlite":
                services.AddDbContext<SqliteTonepieDb>(o => o.UseSqlite(cs));
                services.AddScoped<TonepieDb>(sp => sp.GetRequiredService<SqliteTonepieDb>());
                break;
            case "postgres":
            case "postgresql":
                services.AddDbContext<PostgresTonepieDb>(o => o.UseNpgsql(cs));
                services.AddScoped<TonepieDb>(sp => sp.GetRequiredService<PostgresTonepieDb>());
                break;
            case "sqlserver":
                services.AddDbContext<SqlServerTonepieDb>(o => o.UseSqlServer(cs));
                services.AddScoped<TonepieDb>(sp => sp.GetRequiredService<SqlServerTonepieDb>());
                break;
            default:
                throw new InvalidOperationException($"Unknown Tonepie:Provider '{provider}': use Sqlite, Postgres or SqlServer");
        }
        return services;
    }
}

// Used only by "dotnet ef migrations add": the connection strings are never opened.
public class SqliteFactory : IDesignTimeDbContextFactory<SqliteTonepieDb>
{
    public SqliteTonepieDb CreateDbContext(string[] args) =>
        new(new DbContextOptionsBuilder<SqliteTonepieDb>().UseSqlite("Data Source=design.db").Options);
}
public class PostgresFactory : IDesignTimeDbContextFactory<PostgresTonepieDb>
{
    public PostgresTonepieDb CreateDbContext(string[] args) =>
        new(new DbContextOptionsBuilder<PostgresTonepieDb>().UseNpgsql("Host=localhost;Database=tonepie").Options);
}
public class SqlServerFactory : IDesignTimeDbContextFactory<SqlServerTonepieDb>
{
    public SqlServerTonepieDb CreateDbContext(string[] args) =>
        new(new DbContextOptionsBuilder<SqlServerTonepieDb>().UseSqlServer("Server=localhost;Database=tonepie").Options);
}
