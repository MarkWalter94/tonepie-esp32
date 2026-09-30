FROM mcr.microsoft.com/dotnet/sdk:10.0 AS build
WORKDIR /src
COPY Tonepie.Server/Tonepie.Server.csproj Tonepie.Server/
RUN dotnet restore Tonepie.Server/Tonepie.Server.csproj
COPY Tonepie.Server/ Tonepie.Server/
RUN dotnet publish Tonepie.Server/Tonepie.Server.csproj -c Release -o /app --no-restore

FROM mcr.microsoft.com/dotnet/aspnet:10.0
WORKDIR /app
COPY --from=build /app .
RUN mkdir -p /data && chown $APP_UID /data
USER $APP_UID
ENV ASPNETCORE_HTTP_PORTS=8090 \
    ConnectionStrings__Tonepie="Data Source=/data/tonepie.db" \
    Tonepie__Provider=Sqlite \
    Tonepie__TimeZone=Europe/Rome
VOLUME /data
EXPOSE 8090
# No curl in the aspnet image: the server binary probes its own /health (which also checks the database).
HEALTHCHECK --interval=1m --timeout=10s --start-period=30s --retries=3 CMD ["dotnet", "Tonepie.Server.dll", "--healthcheck"]
ENTRYPOINT ["dotnet", "Tonepie.Server.dll"]
