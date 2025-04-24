# Low-Latency C++ Trading Bot

Inspired by David Gross Meeting CPP 2022
## Features

- Lock-free MPSC queue for fast hand off between threads
- Non-blocking UDP market data listener (multicast ready)
- Non-blocking TCP order entry sender
- Microsecond sleeps to avoid CPU burn and maintain speed
- Buy if price < 100.0, else sell
- Runs for 30s

## Prerequisites

- C++17–compliant compiler (g++ ≥ 7, clang++ ≥ 6)
- POSIX-compatible OS (Linux/macOS)
- pthreads and BSD sockets support

## Building

```bash
# Clone the repo
git clone https://github.com/your-org/low-latency-trading-bot.git
cd low-latency-trading-bot

# Build with g++
g++ -O3 -std=c++17 -pthread -o trading_bot src/main.cpp
```

Tip: Use `-O3` for maximum optimization and `-march=native` for better CPU speed.

## Running

```bash
# Usage: ./trading_bot

./trading_bot
```

What this does:
1. Listen on UDP `239.0.0.1:5000` for market ticks
2. Connect via TCP to `127.0.0.1:6000` for order entry
3. Run for 30 seconds, then exit gracefully

To adjust addresses or ports, edit the constants in `main.cpp` and rebuild.

## Architecture Overview

```mermaid
flowchart LR
  A[UDP Market Data] -->|tick| B(Market-Data Thread)
  B -->|Order push| C[MPSC Queue]
  C -->|Order pop| D(Order-Entry Thread)
  D -->|send| E[TCP Order Server]
```

## Customization

- **Trading logic**: modify the `if (px < 100.0)` condition in the market data thread
- **Run duration**: change `std::this_thread::sleep_for(std::chrono::seconds(30));`
- **Queue behavior**: adjust the order thread’s sleep or switch to busy wait

## Troubleshooting

- **Socket bind errors**: ensure no other process uses the same UDP port
- **Connect failures**: verify the order-entry server is listening on the TCP port
- **High CPU usage**: increase the sleep duration in the order thread or add backoff logic

## License

MIT License © 2025 Sami Halabieh
