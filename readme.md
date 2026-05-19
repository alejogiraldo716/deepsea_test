# EV Charger Telemetry Pipeline

A diagnostic telemetry pipeline for Level 3 DC Fast Charger gateways,
developed for DeepSea Developments. Written in pure C, targeting headless
Raspberry Pi Zero W nodes.

---

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
2. [Repository Structure](#2-repository-structure)
3. [Development Environment](#3-development-environment)
4. [Dependencies](#4-dependencies)
5. [Build Instructions](#5-build-instructions)
   - 5.1 [Provider - standard build](#51-provider---standard-build)
   - 5.2 [Provider - debug build](#52-provider---debug-build)
   - 5.3 [Full build (both components)](#53-full-build-both-components)
6. [Run Instructions](#6-run-instructions)
   - 6.1 [Session 1 - start the provider daemon](#61-session-1---start-the-provider-daemon)
   - 6.2 [Session 2 - attach the dashboard](#62-session-2---attach-the-dashboard)
7. [Author](#7-author)
---


## 1. Architecture Overview

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

## 2. Repository Structure

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

## 3. Development Environment

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

## 4. Dependencies

Install the following packages on the target system before building:

```bash
sudo apt update
sudo apt install -y gcc libglib2.0-dev dbus pkg-config make
```

---

## 5. Build Instructions

### 5.1 Provider - standard build

```bash
cd src/
make
./telemetry_provider_daemon
```

Expected output:
```bash
Provider running - emitting signals on com.deepsea.Telemetry
```
The provider will run indefinitely, emitting D-Bus signals at the target rate.
Stop it with `Ctrl + C`.

### 5.2 Provider - debug build

To enable verbose signal emission output, compile with the `debug` target:

```bash
make debug
./telemetry_provider_daemon
```

Expected output:
```bash
Provider running - emitting signals on com.deepsea.Telemetry
Emitted 100 signals
Emitted 200 signals
Emitted 300 signals
```
Debug output is printed every 100 signals. This mode is intended for
development verification only and must not be used in production, as the
additional I/O will affect sampling timing.

### 5.3 Full build (both components)

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

## 6. Run Instructions

Two separate terminal sessions are required, one for each component. Both must
share the same D-Bus System Bus on the target machine.

### 6.1 Session 1 - start the provider daemon

```bash
./telemetry_provider_daemon
```

Expected output:
```
Provider running - emitting signals on com.deepsea.Telemetry
```

### 6.2 Session 2 - attach the dashboard

```bash
./diagnostic_dashboard_client
```

The dashboard will render in place and update at 10 Hz with live metrics
streamed from the provider. Example view:
```
╔══════════════════════════════════════════════════════╗
║   DeepSea Developments - EV Charger Telemetry        ║
╚══════════════════════════════════════════════════════╝
CPU Usage   :   2.34%  #---------------------------------------
RAM Usage   :  10.95%  ####------------------------------------
Core Temp   :  47.20°C ###-------------------------------------
──────────────────────────────────────────────────────
Performance Matrix
Current Latency : 200.94 µs
Max Latency     : 1048.14 µs
Messages Rx     : 18420
Dropped/Missed  : 0
──────────────────────────────────────────────────────
```

Stop either component with `Ctrl + C`.

> Note: Temperature will read `-1.00` on systems without a hardware thermal
> sensor (WSL2, virtual machines). On the Raspberry Pi Zero W the correct
> SoC temperature will be reported.

---

## 7. Author

| Field       | Detail                          |
|-------------|---------------------------------|
| Author      | Alejandro Giraldo               |
| Email       | alejo.giraldo716@gmail.com      |
| Maintainer  | Alejandro  Giraldo              |
| Project     | DeepSea Developments - Technical Assessment |

---