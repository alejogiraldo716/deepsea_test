# EV Charger Telemetry Pipeline

A diagnostic telemetry pipeline for Level 3 DC Fast Charger gateways,
developed for DeepSea Developments. Written in pure C, targeting headless
Raspberry Pi Zero W nodes.

---

## Author

| Field       | Detail                          |
|-------------|---------------------------------|
| Author      | Alejandro Giraldo               |
| Email       | alejo.giraldo716@gmail.com      |
| Maintainer  | Alejandro  Giraldo              |
| Project     | DeepSea Developments - Technical Assessment |

---

## Architecture Overview

The system consists of two independent processes communicating over
the D-Bus System Bus:

```
┌──────────────────────────┐      D-Bus System Bus      ┌──────────────────────────┐
│  telemetry_provider      │   ── MetricsBroadcast ──►  │  diagnostic_dashboard    │
│  _daemon                 │       signal broadcast     │  _client                 │
│                          │                            │                          │
│  CPU  → /proc/stat       │                            │  Latency calculation     │
│  RAM  → /proc/meminfo    │                            │  Drop detection          │
│  Temp → thermal_zone0    │                            │  ANSI terminal render    │
└──────────────────────────┘                            └──────────────────────────┘
```

**Component A - telemetry_provider_daemon**: Reads HW metrics from the
Linux pseudo-filesystem and broadcasts them as D-Bus signals at the highest
achievable rate toward the 2 kHz target.

**Component B - diagnostic_dashboard_client**: Subscribes to the signal stream
asynchronously, computes latency and drop statistics, and renders a real-time
terminal dashboard using raw ANSI escape codes.

---

## Repository Structure

```
deepsea-telemetry/
├── docs/
├── src/
│   ├── telemetry_provider_daemon.c
│   ├── diagnostic_dashboard_client.c
│   └── Makefile
├── README.md
└── .gitignore
```

---

## Development Environment

This project was developed and tested on the following environment before
being deployed to the target hardware:

| Field         | Detail                        |
|---------------|-------------------------------|
| Host OS       | Windows 11                    |
| Linux layer   | WSL2 - Ubuntu 24.04           |
| Compiler      | GCC 13.3.0                    |
| Target board  | Raspberry Pi Zero W (ARMv6)   |
| Target OS     | Raspberry Pi OS (Debian-based)|

---

## Dependencies

Install the following packages on the target system before building:

```bash
sudo apt update
sudo apt install -y gcc libglib2.0-dev dbus pkg-config make
```

---

## Build Instructions

### Current state - provider only (no D-Bus yet)

The provider can be compiled and verified independently without GLib,
using only the standard C toolchain:

```bash
cd src/
gcc -std=c11 -Wall -Wextra -o telemetry_provider_daemon telemetry_provider_daemon.c
```

Run it to verify hardware readers and timing:

```bash
./telemetry_provider_daemon
```

Expected output:
```
Telemetry Provider Daemon - starting...
CPU  : 0.47%
RAM  : 10.15%
Temp : -1.00 C
Elapsed : 100.341 ms
```

> Note: Temperature will read `-1.00` on systems without a hardware thermal
> sensor (WSL2, virtual machines). On the Raspberry Pi Zero W the correct
> SoC temperature will be reported.

### Full build (both components - D-Bus required)

Once D-Bus integration is complete, both components are built with:

```bash
cd src/
make
```

This produces two binaries: `telemetry_provider_daemon` and
`diagnostic_dashboard_client`.

To remove compiled binaries:

```bash
make clean
```

---

## Run Instructions

Two separate terminal sessions are required.

**Session 1 - start the provider daemon:**

```bash
./telemetry_provider_daemon
```

**Session 2 - attach the dashboard:**

```bash
./diagnostic_dashboard_client
```

---
*Full build instructions, run guide, and architectural decisions will be
documented as implementation progresses.*