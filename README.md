# Weather Station Sensor Service (WiringPi Version)

This README documents the **WiringPi-based** implementation of the DHT22 ingestion service. It replaces the pigpio version when `pigpiod` cannot start (e.g., unrecognized hardware revision).

---

## 1. Overview

The service:
1. Talks directly to GPIO via WiringPi (no external daemon).
2. Bit-bangs the DHT22 protocol with microsecond timing loops.
3. Posts each reading to your Next.js ingestion API with an API key.
4. Designed for continuous operation under `systemd`.

---

## 2. Why WiringPi?

| Reason | Explanation |
|--------|-------------|
| pigpiod failed | Your board revision (`e04171`) not recognized by installed pigpio package |
| Simplicity | No background daemon/service prerequisite |
| Quick migration | Only DHT timing logic changed; HTTP + config unchanged |
| Transitional | You can later move to `libgpiod` for long-term maintainability |

> NOTE: WiringPi is considered “legacy”. For future-proofing, consider a libgpiod rewrite when stable.

---

## 3. Trade-offs vs pigpio

| Aspect | pigpio | WiringPi |
|--------|--------|----------|
| Timing accuracy | High (daemon-managed) | Good enough (busy waits) |
| Daemon required | Yes (`pigpiod`) | No |
| Future maintenance | Actively maintained upstream | Community/fork maintenance |
| Advanced features (waveforms) | Yes | Limited |

---

## 4. Requirements

| Component | Notes |
|-----------|-------|
| WiringPi | Install via apt or source (`gpio -v` to confirm) |
| g++ / CMake | As before (g++ 12 OK) |
| libcurl | For HTTP POST |
| DHT22 Sensor | With 10k pull-up resistor |

Install:

```bash
sudo apt update
sudo apt install -y build-essential cmake git libcurl4-openssl-dev wiringpi
gpio -v
gpio readall
```

If outdated / missing:

```bash
cd /tmp
git clone https://github.com/WiringPi/WiringPi.git
cd WiringPi
./build
gpio -v
```

---

## 5. Directory Layout

```
sensor_service/
  src/
    main.cpp
    Dht22Reader.cpp  (WiringPi pulse measurement)
    Dht22Reader.hpp
    HttpClient.cpp
    HttpClient.hpp
    Config.cpp
    Config.hpp
  CMakeLists.txt
  config.example.yaml
```

---

## 6. Configuration

Copy example:

```bash
cp config.example.yaml config.yaml
nano config.yaml
```

Example:

```yaml
gpio_pin: 4
interval_seconds: 5
api_base: http://localhost:3000
api_key: REPLACE_WITH_REAL_INGEST_KEY
source_id: pi-lab
```

| Field | Description |
|-------|-------------|
| gpio_pin | BCM numbering (WiringPi uses `wiringPiSetupGpio()` internally) |
| interval_seconds | Sleep between loops (not counting read time) |
| api_base | URL of Next.js server (use IP if remote) |
| api_key | Must equal `INGEST_API_KEY` in `.env.local` |
| source_id | Sensor identifier stored in DB |

Do **NOT** commit `config.yaml`; commit only `config.example.yaml`.

---

## 7. Build

```bash
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

---

## 8. Run (Manual)

```bash
./sensor_service ../config.yaml
```

Sample output:

```
Starting DHT22 service on GPIO 4 interval=5s source=pi-lab
[WARN] Read failed (1): No initial LOW (sensor not responding)
[OK] 2025-09-27T19:10:32Z T=23.5C H=49.8%
```

Early transient failures are normal (sensor wake-up). Consistent failures → check wiring.

---

## 9. HTTP Ingestion Verification

After a few `[OK]` lines:

```bash
curl -s http://<web-host>:3000/api/readings?limit=3 | jq
```

Should show entries with `source: "pi-lab"`.

---

## 10. Systemd Service

Install:

```bash
sudo cp build/sensor_service /usr/local/bin/
sudo useradd -r -s /usr/sbin/nologin weather 2>/dev/null || true
sudo mkdir -p /etc/weather-sensor
sudo cp config.yaml /etc/weather-sensor/config.yaml
sudo chown -R weather:weather /etc/weather-sensor
```

Unit:

```ini
# /etc/systemd/system/weather-sensor.service
[Unit]
Description=Weather Station DHT22 (WiringPi)
After=network.target

[Service]
ExecStart=/usr/local/bin/sensor_service /etc/weather-sensor/config.yaml
User=weather
Restart=on-failure
RestartSec=5
Environment=TZ=UTC

[Install]
WantedBy=multi-user.target
```

Enable:

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now weather-sensor.service
journalctl -u weather-sensor.service -f
```

---

## 11. Code Highlights (DHT22 Timing)

Key logic (simplified):

1. Pull data pin LOW for ≥1 ms (using `delay(2)`).
2. Brief HIGH for ~30 µs.
3. Switch to input; wait for low/high preamble.
4. For each bit:
   - Wait for rising edge.
   - Measure HIGH pulse length (`micros()`).
   - Threshold (≈50 µs): shorter = 0, longer = 1.
5. Validate checksum.
6. Convert raw humidity & temperature.

Tuning points:
- Increase start signal to 3–4 ms if sensor unreliable.
- Adjust bit threshold (50 → 55/60 µs) if misclassification occurs.

---

## 12. Troubleshooting

| Symptom | Possible Cause | Fix |
|---------|----------------|-----|
| Continuous `No initial LOW` | Sensor not powered / wrong pin / missing pull-up | Check 3.3V, ground, data resistor |
| Continuous checksum mismatch | Threshold off / noise | Shorter wires, adjust threshold in code |
| Always same temp/humidity | Stuck sensor or repeating cached value | Power cycle sensor; check code path for `valid` flag |
| HTTP `Couldn't connect to server` | `api_base` incorrect or server unreachable | Use `curl <api_base>/api/health` to test |
| 401 Unauthorized | Key mismatch | Sync `api_key` with web `.env.local` |
| Works manually, fails under systemd | Wrong config path | Use absolute path in unit file |

---

## 13. Security

- Secret in plain text (`api_key`)—restrict `config.yaml` perms: `chmod 600`.
- Keep ingestion endpoint non-public or behind reverse proxy if exposed externally.
- Rotate key periodically (manually for now).

---

## 14. Enhancements (Next Steps)

| Category | Ideas |
|----------|-------|
| Data Quality | Add dew point / heat index calculation |
| Reliability | Exponential backoff after N failures |
| Observability | Push a heartbeat metric or log consecutive failures |
| Multi-Sensor | Multiple pins → multiple binaries or multiplex logic |
| Real-Time UI | SSE/WebSocket to push new readings instantly |

---

## 15. Migration From pigpio

| Task | Action |
|------|--------|
| Remove pigpio dependency | Stop `pigpiod`, remove pigpio libs (optional) |
| Replace build flags | Link `wiringPi` instead of `pigpio` |
| Swap reader | Use `Dht22Reader (WiringPi)` implementation |
| Systemd unit | Drop `Requires=pigpiod.service` |
| Timing tests | Validate stability over 24h run |

---

## 16. Testing Script (Simulated Posts)

While diagnosing sensor hardware, you can simulate ingestion:

```bash
for i in $(seq 1 5); do
  curl -X POST "$API_BASE/api/ingest" \
    -H "X-API-Key: $INGEST_API_KEY" \
    -H "Content-Type: application/json" \
    -d "{\"temperature_c\":$((20 + i)).0,\"humidity_rel\":$((50 + i)).0,\"timestamp_utc\":\"$(date -u +%FT%TZ)\",\"source\":\"sim\"}"
  sleep 2
done
```

---

## 17. Maintenance

| Task | Frequency | Command |
|------|-----------|---------|
| Rebuild after code change | As needed | `make -j$(nproc)` |
| Update WiringPi (if fork changes) | Rare | Re-run `./build` from repo |
| Log review | Weekly | `journalctl -u weather-sensor.service --since "1 day ago"` |
| Key rotation | Manual | Generate new hex → update config & web env |

---

## 18. License

Match top-level project license (e.g., MIT).

---

## 19. Cheat Sheet

```bash
# Build
cd sensor_service && mkdir -p build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(nproc)

# Run
./sensor_service ../config.yaml

# Systemd
sudo systemctl restart weather-sensor.service
journalctl -u weather-sensor.service -f

# Read latest ingestion
curl -s http://<web>:3000/api/readings?limit=1
```

---

## 20. When to Move to pigpio

Consider migrating if:
- You need long-term maintenance.
- You add more complex GPIO interactions.
- You deploy on mixed SBC hardware brands.
- Using a Pi from 4 downwards

---

Happy building! Open an issue (or note in your project tracker) if you later want a libgpiod rewrite template or pulse logging diagnostics.
