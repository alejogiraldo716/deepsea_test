# EV Charger Telemetry Pipeline

A diagnostic telemetry pipeline for Level 3 DC Fast Charger gateways,
developed for DeepSea Developments. Written in pure C, targeting headless
Raspberry Pi Zero W nodes.

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
└── README.md
└── .gitignore
```

---

*Full build instructions, run guide, and architectural decisions will be
documented as implementation progresses.*