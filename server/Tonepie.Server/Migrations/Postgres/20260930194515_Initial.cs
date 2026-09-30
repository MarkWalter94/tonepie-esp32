using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace Tonepie.Server.Migrations.Postgres
{
    /// <inheritdoc />
    public partial class Initial : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.CreateTable(
                name: "Devices",
                columns: table => new
                {
                    Id = table.Column<string>(type: "character varying(40)", maxLength: 40, nullable: false),
                    Firmware = table.Column<string>(type: "character varying(40)", maxLength: 40, nullable: false),
                    FirstSeen = table.Column<long>(type: "bigint", nullable: false),
                    LastSeen = table.Column<long>(type: "bigint", nullable: false),
                    BinSince = table.Column<long>(type: "bigint", nullable: false),
                    BinVisits = table.Column<int>(type: "integer", nullable: false),
                    BinLimitVisits = table.Column<int>(type: "integer", nullable: false),
                    LitterAt = table.Column<long>(type: "bigint", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_Devices", x => x.Id);
                });

            migrationBuilder.CreateTable(
                name: "DeviceCats",
                columns: table => new
                {
                    DeviceId = table.Column<string>(type: "character varying(40)", nullable: false),
                    Index = table.Column<int>(type: "integer", nullable: false),
                    Name = table.Column<string>(type: "character varying(24)", maxLength: 24, nullable: false),
                    Color = table.Column<int>(type: "integer", nullable: false),
                    WeightG = table.Column<int>(type: "integer", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_DeviceCats", x => new { x.DeviceId, x.Index });
                    table.ForeignKey(
                        name: "FK_DeviceCats_Devices_DeviceId",
                        column: x => x.DeviceId,
                        principalTable: "Devices",
                        principalColumn: "Id",
                        onDelete: ReferentialAction.Cascade);
                });

            migrationBuilder.CreateTable(
                name: "Events",
                columns: table => new
                {
                    DeviceId = table.Column<string>(type: "character varying(40)", nullable: false),
                    Kind = table.Column<int>(type: "integer", nullable: false),
                    Epoch = table.Column<long>(type: "bigint", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_Events", x => new { x.DeviceId, x.Kind, x.Epoch });
                    table.ForeignKey(
                        name: "FK_Events_Devices_DeviceId",
                        column: x => x.DeviceId,
                        principalTable: "Devices",
                        principalColumn: "Id",
                        onDelete: ReferentialAction.Cascade);
                });

            migrationBuilder.CreateTable(
                name: "Visits",
                columns: table => new
                {
                    DeviceId = table.Column<string>(type: "character varying(40)", nullable: false),
                    EspId = table.Column<int>(type: "integer", nullable: false),
                    Epoch = table.Column<long>(type: "bigint", nullable: false),
                    WeightG = table.Column<int>(type: "integer", nullable: false),
                    DurationS = table.Column<int>(type: "integer", nullable: false),
                    CatName = table.Column<string>(type: "character varying(24)", maxLength: 24, nullable: true),
                    Manual = table.Column<bool>(type: "boolean", nullable: false),
                    Deleted = table.Column<bool>(type: "boolean", nullable: false),
                    UpdatedAt = table.Column<long>(type: "bigint", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_Visits", x => new { x.DeviceId, x.EspId, x.Epoch });
                    table.ForeignKey(
                        name: "FK_Visits_Devices_DeviceId",
                        column: x => x.DeviceId,
                        principalTable: "Devices",
                        principalColumn: "Id",
                        onDelete: ReferentialAction.Cascade);
                });

            migrationBuilder.CreateIndex(
                name: "IX_Visits_DeviceId_Epoch",
                table: "Visits",
                columns: new[] { "DeviceId", "Epoch" });
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropTable(
                name: "DeviceCats");

            migrationBuilder.DropTable(
                name: "Events");

            migrationBuilder.DropTable(
                name: "Visits");

            migrationBuilder.DropTable(
                name: "Devices");
        }
    }
}
