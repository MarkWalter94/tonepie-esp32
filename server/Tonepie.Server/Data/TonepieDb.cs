using Microsoft.EntityFrameworkCore;

namespace Tonepie.Server.Data;

/// <summary>
/// Provider-neutral model. Each supported database has a small derived context below, so that
/// each one keeps its own migrations (Migrations/Sqlite, Migrations/Postgres, Migrations/SqlServer).
/// </summary>
public abstract class TonepieDb(DbContextOptions options) : DbContext(options)
{
    public DbSet<Device> Devices => Set<Device>();
    public DbSet<Visit> Visits => Set<Visit>();
    public DbSet<MaintenanceEvent> Events => Set<MaintenanceEvent>();

    protected override void OnModelCreating(ModelBuilder b)
    {
        b.Entity<Device>(e =>
        {
            e.HasKey(d => d.Id);
            e.Property(d => d.Id).HasMaxLength(40);
            e.Property(d => d.Firmware).HasMaxLength(40);
            e.HasMany(d => d.Cats).WithOne().HasForeignKey(c => c.DeviceId).OnDelete(DeleteBehavior.Cascade);
        });
        b.Entity<DeviceCat>(e =>
        {
            e.ToTable("DeviceCats");
            e.HasKey(c => new { c.DeviceId, c.Index });
            e.Property(c => c.Name).HasMaxLength(24);
        });
        b.Entity<Visit>(e =>
        {
            e.HasKey(v => new { v.DeviceId, v.EspId, v.Epoch });
            e.Property(v => v.CatName).HasMaxLength(24);
            e.HasIndex(v => new { v.DeviceId, v.Epoch });
            e.HasOne<Device>().WithMany().HasForeignKey(v => v.DeviceId).OnDelete(DeleteBehavior.Cascade);
        });
        b.Entity<MaintenanceEvent>(e =>
        {
            e.HasKey(v => new { v.DeviceId, v.Kind, v.Epoch });
            e.HasOne<Device>().WithMany().HasForeignKey(v => v.DeviceId).OnDelete(DeleteBehavior.Cascade);
        });
    }
}

public class SqliteTonepieDb(DbContextOptions<SqliteTonepieDb> options) : TonepieDb(options);
public class PostgresTonepieDb(DbContextOptions<PostgresTonepieDb> options) : TonepieDb(options);
public class SqlServerTonepieDb(DbContextOptions<SqlServerTonepieDb> options) : TonepieDb(options);
