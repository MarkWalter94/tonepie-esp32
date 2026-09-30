namespace Tonepie.Server.Data;

// Times are Unix seconds (UTC) as sent by the ESP: portable across every database provider.

/// <summary>One ESP (litter box) and the latest state it reported.</summary>
public class Device
{
    public string Id { get; set; } = "";
    public string Firmware { get; set; } = "";
    public long FirstSeen { get; set; }
    public long LastSeen { get; set; }
    /// <summary>Last bag change and visits counted since then, as reported by the ESP.</summary>
    public long BinSince { get; set; }
    public int BinVisits { get; set; }
    public int BinLimitVisits { get; set; }
    public long LitterAt { get; set; }
    public List<DeviceCat> Cats { get; set; } = [];
}

/// <summary>A cat as currently configured on the ESP (index = position in its list).</summary>
public class DeviceCat
{
    public string DeviceId { get; set; } = "";
    public int Index { get; set; }
    public string Name { get; set; } = "";
    public int Color { get; set; }
    public int WeightG { get; set; }
}

/// <summary>
/// A visit. The ESP numbers visits itself; the number alone may repeat if the ESP's
/// storage is erased, so the key also includes the visit time.
/// </summary>
public class Visit
{
    public string DeviceId { get; set; } = "";
    public int EspId { get; set; }
    /// <summary>0 when the ESP had no clock at the time: kept, but not placed on any day.</summary>
    public long Epoch { get; set; }
    public int WeightG { get; set; }
    public int DurationS { get; set; }
    /// <summary>Cat name at the last update; null when not recognised. History is grouped by name,
    /// since the ESP renumbers cats when one is removed.</summary>
    public string? CatName { get; set; }
    public bool Manual { get; set; }
    /// <summary>Deleted on the ESP ("not a visit"): kept for traceability, excluded from history.</summary>
    public bool Deleted { get; set; }
    public long UpdatedAt { get; set; }
}

public enum MaintenanceKind { BagChange = 1, LitterAdded = 2 }

public class MaintenanceEvent
{
    public string DeviceId { get; set; } = "";
    public MaintenanceKind Kind { get; set; }
    public long Epoch { get; set; }
}
