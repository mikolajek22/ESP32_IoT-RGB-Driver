# ESP32 IoT RGB Driver

ESP32-based IoT controller for 12V RGB LED strips with a built-in HTTP server, real-time web interface, LittleFS file system, OLED display and hardware keyboard.

## Features

- PWM control of 12V RGB strip via 3 channels
- HTTP server with web UI (color picker, sequences, configuration)
- Real-time log streaming over WebSocket
- LittleFS flash file system for HTML pages and configuration
- SNTP-synchronized clock on OLED display
- Configurable WiFi via `settings.json` (upload over HTTP or UART)
- JTAG debugging support via ESP-PROG

## Hardware

| Component | Description |
|---|---|
| ESP32 WROOM32U DevKit C | Main controller |
| 12V RGB LED strip | Output device, controlled via 3 PWM channels |
| 60W 12V DC power supply | Powers the RGB strip |
| Step-down converter (up to 36V → 3.3V) | Powers the ESP32 |
| OLED display | Shows IP address, clock |
| 5 buttons | Navigation: IP, netmask, reboot |
| Green LED | Heartbeat — blinks every second |
| Red LED | Button press indicator |

Electrical schematic: [Google Drive](https://drive.google.com/drive/folders/1yBjRe1mOAP0S1CEnbVP0L3ItTyetpFwV?usp=sharing)

## Requirements

- [ESP-IDF v5.3.1](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/)
- [esp_littlefs](https://github.com/joltwallet/esp_littlefs) (included as managed component)

## Build & Flash

```bash
# 1. Build
idf.py build

# 2. Flash file system (HTML pages + settings.json)
idf.py partition_table littlefs-flash

# 3. Flash firmware (replace X with your port number, e.g. COM3)
idf.py -p COMX flash

# 4. Monitor logs (optional)
idf.py -p COMX monitor
```

## Configuration

Network settings are stored in `settings.json` on the device. There are three ways to update them:

1. **Web UI** — go to the *Configuration* page, edit settings directly in the browser and click *Save settings*, then reboot.
2. **File upload** — download `settings.json` from the *Home* page, edit it, and re-upload on the *Configuration* page.
3. **UART flash** — place the updated file in `pages_html/` and run `idf.py partition_table littlefs-flash`.

Default settings are hardcoded as a fallback in case the configuration file is missing or corrupted. After any network change, a reboot is required (available via the web UI).

The device's IP address is shown on the OLED display after boot.

## Web Interface

Access the UI by entering the device IP in a browser.

| Page | Description |
|---|---|
| Home | Device info, download `settings.json` |
| Configuration | Upload config file, edit settings, reboot |
| RGB Controller | Set colors and sequences |
| Logs | Real-time log stream over WebSocket, download log file (max 512 kB) |

### RGB Controller modes

**Manual** — select a color from the RGB picker, five preset buttons, or a color cursor.

**Sequence** — four built-in sequences defined in `rgbController.cpp`.

**Original** — define a custom 3-point sequence via the web UI. The controller fades from base 1 → base 2 → base 3 → base 1 in a loop.

**Period time** — sets the delay between each iteration. With period = 100 ms and 8-bit color depth (0–255), a full single-channel transition takes 255 × 100 ms = 25.5 s.

**Speed** (original sequence only) — defines the increment per iteration (1–100%). At 100% speed and 100 ms period, a full 255→0 transition takes 100 ms.

## Logging

Logs are available over UART:

```bash
idf.py -p COMx monitor
```

They are also streamed in real time to the *Logs* page via WebSocket. Every 5 seconds the device sends a diagnostic message with RAM usage, date and time.

## Debugging

The board exposes JTAG pins compatible with ESP-PROG. A ready-to-use VS Code launch configuration is included in `.vscode/`.

Setup guide: [YouTube tutorial](https://www.youtube.com/watch?app=desktop&v=uq93H7T7cOQ)

## Project Structure

```
main/           # Application source (C and C++)
pages_html/     # Web UI files and settings.json (flashed to LittleFS)
managed_components/joltwallet__littlefs/   # LittleFS library
.vscode/        # VS Code + ESP-PROG debug configuration
CMakeLists.txt
partition.csv   # Custom partition table
```

## Roadmap

- [ ] BLE server for WiFi configuration (fallback when no AP available)
- [ ] BLE control of RGB strip
- [ ] Configuration file upload over UART
- [ ] Access Point mode as default network configuration