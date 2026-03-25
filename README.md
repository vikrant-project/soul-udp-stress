# SOUL - UDP Stress Testing Tool ⚡🔥

> A high-performance UDP stress testing tool written in C. Multi-threaded architecture with a dedicated worker/IO thread split, ring buffer pipeline, atomic stats tracking, and configurable payload patterns. Built for Linux.

---

## ✨ What It Does

SOUL sends UDP packets at maximum throughput to a target IP and port for a specified duration. Worker threads generate packets into a shared ring buffer, while dedicated I/O threads drain the buffer and send over non-blocking UDP sockets — all with live real-time statistics.

---

## 🚀 Key Features

| Feature | Description |
|---|---|
| **Multi-threaded Pipeline** | Separate worker threads (packet gen) and I/O threads (sending) |
| **Ring Buffer** | Lock-based ring buffer decouples generation from transmission |
| **3 Payload Patterns** | Random (`/dev/urandom`), Sequential, or Fixed (0xFF) |
| **Atomic Stats** | Lock-free atomic counters for PPS, Mbps, drops, errors |
| **CPU Affinity** | Threads pinned to CPU cores for maximum throughput (Linux) |
| **Rate Limiting** | Optional PPS cap to control send rate |
| **Non-blocking Sockets** | All sockets use `MSG_DONTWAIT` for zero-blocking sends |
| **Local-only Mode** | Safety flag to restrict targets to `127.x.x.x` only |
| **Live Dashboard** | Real-time `\r` stats: PPS, Mbps, sent, drops, errors |
| **Clean Shutdown** | SIGINT/SIGTERM handled gracefully with final stats report |

---

## 📋 Prerequisites

- **Linux** (Ubuntu 20.04+ recommended)
- **GCC** with C11 support
- **make**
- **pthreads** (usually pre-installed)

### Install Build Tools

```bash
sudo apt update
sudo apt install -y gcc make
```

---

## 🛠 Build

### 1. Clone the Repository

```bash
git clone https://soulcrack-spoofs-admin@bitbucket.org/soulcrack-spoofs/soul-udp-stress.git
cd soul-udp-stress
```

### 2. Compile

```bash
make
```

This produces the `soul` binary in the current directory.

### Clean Build

```bash
make clean && make
```

---

## 🎮 Usage

```bash
./soul <ip> <port> <duration> [options]
```

### Required Arguments

| Argument | Description |
|---|---|
| `ip` | Target IPv4 address |
| `port` | Target port (1–65535) |
| `duration` | Test duration in seconds (1–86400) |

### Options

| Flag | Default | Description |
|---|---|---|
| `--rate <pps>` | `0` (unlimited) | Limit packets per second |
| `--size <bytes>` | `1400` | Packet size (64–65507 bytes) |
| `--threads <N>` | `50` | Number of worker threads (1–256) |
| `--pattern <type>` | `random` | Payload pattern: `random`, `sequential`, `fixed` |
| `--local-only` | off | Restrict target to `127.x.x.x` only |
| `--verbose` | off | Enable verbose output |
| `--help` | — | Show usage |

### Examples

```bash
# Basic test — localhost port 9999 for 30 seconds
./soul 127.0.0.1 9999 30

# High throughput — 100 threads, max size packets
./soul 127.0.0.1 9999 60 --threads 100 --size 65507

# Rate-limited test — 10,000 PPS
./soul 127.0.0.1 9999 30 --rate 10000

# Sequential pattern, local only, verbose
./soul 127.0.0.1 9999 10 --pattern sequential --local-only --verbose
```

---

## 📊 Live Output

```
╔═══════════════════════════════════════════════╗
║         SOUL - UDP Stress Testing Tool       ║
║              High Performance Edition         ║
╚═══════════════════════════════════════════════╝

Configuration:
  Target: 127.0.0.1:9999
  Duration: 30 seconds
  Packet size: 1400 bytes
  Threads: 50 workers, 4 I/O
  Pattern: random

[12.4s] PPS: 284931 (avg: 271204) | Mbps: 3194.23 (avg: 3042.18) | Sent: 3362289 | Drops: 0 | Errors: 0
```

**Final stats on completion:**
```
=== Final Statistics ===
Duration: 30.02 seconds
Packets sent: 8123442
Bytes sent: 11372818800 (10843.50 MB)
Average PPS: 270598
Average Mbps: 3031.71
Packets dropped: 0
Send errors: 12
Drop rate: 0.00%
Error rate: 0.00%
```

---

## 🗂 Project Structure

```
soul-udp-stress/
├── main.c              # Entry point — init, main loop, cleanup
├── config.c/h          # CLI parsing, validation, config struct
├── memory.c/h          # Aligned allocator + memory pool
├── packet_builder.c/h  # Packet construction with header + payload
├── ring_buffer.c/h     # Thread-safe ring buffer (producer/consumer)
├── socket_wrapper.c/h  # UDP socket creation, config, send
├── stats.c/h           # Atomic stats counters + live/final print
├── signals.c/h         # SIGINT/SIGTERM graceful shutdown
├── thread_pool.c/h     # Worker + I/O thread management, CPU affinity
├── utils.c/h           # Time helpers (get_time_ms, sleep_ms)
└── Makefile
```

---

## ⚙️ Architecture

```
CLI args
   │
   ▼
config_parse() → config_validate()
   │
   ▼
┌─────────────────────────────────────┐
│         WORKER THREADS (x50)        │
│  packet_build() → ring_buffer_push()│
└──────────────┬──────────────────────┘
               │ ring buffer
┌──────────────▼──────────────────────┐
│          I/O THREADS (x4)           │
│  ring_buffer_pop() → socket_send()  │
└──────────────┬──────────────────────┘
               │
               ▼
        stats_record_packet()
               │
               ▼
        Live stats every 1s
               │
               ▼
        stats_print_final()
```

---

## ⚙️ Key Constants (`config.h`)

| Constant | Value | Description |
|---|---|---|
| `DEFAULT_THREADS` | `50` | Default worker threads |
| `DEFAULT_IO_THREADS` | `4` | Default I/O threads |
| `MAX_THREADS` | `256` | Max worker threads |
| `DEFAULT_PACKET_SIZE` | `1400` | Default packet size (bytes) |
| `MAX_PACKET_SIZE` | `65507` | Max UDP payload |
| `MIN_PACKET_SIZE` | `64` | Min packet size |
| `RING_BUFFER_SIZE` | `8192` | Ring buffer capacity (packets) |
| `TOTAL_BUFFER_SIZE` | `80MB` | Total memory budget |

---

## 🚢 Run in Background

```bash
nohup ./soul 127.0.0.1 9999 3600 --threads 100 > stress.log 2>&1 &
```

Stop anytime with:
```bash
kill %1
# or
Ctrl+C
```

---

## ⚠️ Legal Disclaimer

This tool is intended for **educational purposes and legitimate network stress testing** only — on systems you own or have explicit written permission to test. Unauthorized use against third-party systems is illegal. The authors accept no liability for misuse.

---

## 🤝 Contributing

Pull requests are welcome. For major changes, open an issue first.

---

<p align="center">Built for speed. Engineered in C. ⚡</p>