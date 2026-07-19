# ttl-probe

свой traceroute на C++ через сырые ICMP-сокеты. шлёт echo-запросы с растущим TTL и смотрит кто отвечает "time exceeded" на каждом хопе.

```mermaid
sequenceDiagram
    ttl-probe->>Hop1: ICMP echo, TTL=1
    Hop1-->>ttl-probe: time exceeded
    ttl-probe->>Hop2: ICMP echo, TTL=2
    Hop2-->>ttl-probe: time exceeded
    ttl-probe->>Target: ICMP echo, TTL=N
    Target-->>ttl-probe: echo reply (дошли)
```

изначально планировал сюда честный fake-packet TTL-трюк (как в zapret), но на винде это требует сырых TCP-пакетов, а винда с висты их резать не даёт без отдельного драйвера (WinDivert/Npcap) — это уже не "просто". traceroute зато честно показывает сколько хопов до сервера, полезно прежде чем городить любой TTL-трюк вручную.

## сборка

```
cmake -B build
cmake --build build --config Release
```

## юзать

нужны права администратора (сырые сокеты):

```
build\Release\ttlprobe.exe example.com 30
```
