# SmartDial

A small modular app launcher for Seeed XIAO ESP32S3 that runs games, utilities and a BLE launcher. Main entry point: [src/main.cpp](src/main.cpp).

Features
- Menu-driven launcher with games (Pong, Snake, Maze), Torch app and BLE-based Launcher.
- Uses Seeed_GFX / TFT_eSPI for display rendering.
- Haptics and BNO sensor support.
- Modular app interface via the Modulino knob.

Quick links to main code & modules
- [`src/main.cpp`](src/main.cpp)
- UI / menu: [`Menu::init`](lib/Menu/Menu.h)
- Torch app: [`Torch::initOnce`](lib/Apps/Torch.h)
- Pong: [`Pong::init`](lib/Games/Pong.h)
- Snake: [`Snake::init`](lib/Games/Snake.h)
- Maze: [`Maze::init`](lib/Games/Maze.h)
- Launcher (BLE GATT): [`Launcher::init`](lib/Launcher/Launcher.h)
- Lights (BLE UART-like): [`Lights::init`](lib/Lights/Lights.h)
- Storage: [`Storage::loadHighscores`](lib/Storage/Storage.h)
- BLE host helper: [`BLEHost::begin`](lib/BLE/BLEHost.h)
- Haptics: [`Haptics::begin`](lib/Haptics/Haptics.h)
- BNO sensor: [`Sensors::BNO::begin`](lib/Sensors/BNO.h)
- Seeed GFX setup selector: [lib/Seeed_GFX/User_Setup_Select.h](lib/Seeed_GFX/User_Setup_Select.h)

PlatformIO setup

1. Clone repository and open in VS Code + PlatformIO.
2. Platform/board used in this project:
   - CONFIGURATION: https://docs.platformio.org/page/boards/espressif32/seeed_xiao_esp32s3.html
   - PLATFORM: Espressif 32 (6.12.0)
   - BOARD: Seeed Studio XIAO ESP32S3
   - HARDWARE: ESP32S3 240MHz, 320KB RAM, 8MB Flash

3. The provided platformio.ini already includes the project environment:
   - [platformio.ini](platformio.ini)

4. Install libraries (PlatformIO will auto-install lib_deps from platformio.ini). To force-install locally, run:
```sh
pio lib install "arduino-libraries/Arduino_Modulino@^0.6.0" \
                "adafruit/Adafruit BNO08x@^1.2.5" \
                "adafruit/Adafruit DRV2605 Library@^1.2.4" \
                "adafruit/Adafruit GFX Library@^1.12.3" \
                "Seeed-Studio/Seeed_GFX@^2.0.3"
```

Build & flash
- Build (release mode in CI):
```sh
pio run -e seeed_xiao_esp32s3
```
- Build with verbose output:
```sh
pio run -e seeed_xiao_esp32s3 -v
```
- Upload (ensure board connected and correct upload method in platformio.ini):
```sh
pio run -e seeed_xiao_esp32s3 -t upload
```

Debugging
- Debug adapters supported (cmsis-dap, JLink, etc.) — see PlatformIO debug configuration in .vscode/launch.json.
- Toolchain & packages used by the environment (from CI):
  - framework-arduinoespressif32 @ 3.20017.241212+sha.dcc1105b
  - tool-esptoolpy @ 2.40900.250804
  - toolchain-riscv32-esp @ 8.4.0+2021r2-patch5
  - toolchain-xtensa-esp32s3 @ 8.4.0+2021r2-patch5

Notes & configuration hints
- Seeed_GFX is configured via the library User_Setups; if you change hardware or pinout edit:
  - [lib/Seeed_GFX/User_Setup_Select.h](lib/Seeed_GFX/User_Setup_Select.h)
  - See PlatformIO config examples: [lib/Seeed_GFX/docs/PlatformIO/Configuring options.txt](lib/Seeed_GFX/docs/PlatformIO/Configuring options.txt)
- The project relies on the Modulino knob API (Modulino library). The code uses `Modulino.begin()` and `knob.begin()` in [src/main.cpp](src/main.cpp).
- LDF: Library Dependency Finder found many compatible libraries for the selected environment. More on LDF: https://bit.ly/configure-pio-ldf

Troubleshooting
- If the TFT shows garbled output, confirm the active User_Setup file or set USER_SETUP_LOADED via PlatformIO build_flags per Seeed_GFX docs.
- Missing sensors or haptics are treated as non-fatal; core UI still runs.

License & contribution
- See repository LICENSE (if present) and individual library licenses (e.g. Seeed_GFX includes MIT/BSD components). Consult `lib/Seeed_GFX/license.txt` for details.

If you want, this README can be extended with a wiring diagram, per-board build targets, or a developer guide for adding new apps.
