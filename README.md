GOLF PLUS (Qt/QML + Python + C/libgpiod)
================================================

This repo is a small prototype showing how a Qt Quick (QML) UI can react to an IR obstacle sensor trigger on Raspberry Pi.

Architecture (current)
----------------------
IR Sensor (GPIO) -> C program (libgpiod, event + debounce) -> Python backend (subprocess + parsing) -> QML UI (circle changes color)

- The UI (QML) does **not** talk to GPIO directly.
- The Python backend exposes a `backend.detected` property to QML.
- The backend can run in:
  - Mock mode (no hardware) for development on any Linux PC, or
  - Real mode on Raspberry Pi by reading output from the C program.

Project layout (typical)
------------------------
- ui/
  - main.py         # loads QML + registers backend object
  - backend.py      # provides `detected` + reads IR events
  - main.qml        # UI (circle changes color)
- driver/           # C driver/library (libgpiod)
- logic/            # C executable (ir_event)
- CMakeLists.txt    # root CMake

Prerequisites
-------------
### Linux PC (development)
- Python 3.10+ (recommended 3.11/3.12)
- Qt for Python: PySide6
- Qt Quick runtime (bundled with PySide6)

### Raspberry Pi (deployment)
- Raspberry Pi OS (Bookworm or later recommended)
- Python 3.10+ (system)
- C toolchain + CMake
- libgpiod + headers (for GPIO access)
- PySide6 inside a virtual environment (recommended due to PEP 668)

Qt / PySide6 versions
---------------------
- This project uses **PySide6** (Qt for Python).
- Any PySide6 that provides `QtQuick` / `QtQml` should work (Qt 6.x).
- If you install via pip in a venv, you will get an appropriate Qt 6 runtime for your platform.

Setup: Python venv + PySide6 (Linux PC or Raspberry Pi)
-------------------------------------------------------
From the `ui/` directory (or repo root; choose one place and stick to it):

1) Install venv support (Raspberry Pi OS):
   sudo apt update
   sudo apt install -y python3-full python3-venv

2) Create and activate a virtual environment:
   cd ui
   python3 -m venv venv
   source venv/bin/activate

3) Install Python dependencies:
   pip install --upgrade pip
   pip install PySide6

4) Run the UI:
   python main.py

Note:
- On Raspberry Pi OS you may see "externally-managed-environment" if you try to pip install globally.
  The solution is exactly the venv steps above.

Build: C GPIO app (libgpiod + CMake)
------------------------------------
Install build dependencies on Raspberry Pi (or Linux if you want to build there too):

sudo apt update
sudo apt install -y build-essential cmake pkg-config gpiod libgpiod-dev

Build the C executable:
cd <repo-root>
mkdir -p build
cd build
cmake ..
cmake --build . -j

The executable will be at:
- build/logic/ir_event

Run `ir_event` manually (hardware check):
-----------------------------------------
Example (GPIO17, active-low, 50ms debounce):
./build/logic/ir_event --chip gpiochip0 --line 17 --active-low 1 --debounce-ms 50

If your sensor polarity is inverted, try:
--active-low 0

GPIO wiring notes (typical LM393 IR module)
-------------------------------------------
IR module pins: VCC, GND, OUT
- VCC -> 3.3V on Raspberry Pi (recommended to keep OUT at 3.3V)
- GND -> GND
- OUT -> chosen GPIO input line (default: GPIO17 / physical pin 11)

Run the UI on Raspberry Pi (real sensor mode)
---------------------------------------------
1) Ensure C executable is built:
   cd <repo-root>
   mkdir -p build && cd build
   cmake .. && cmake --build . -j

2) Activate the UI venv:
   cd <repo-root>/ui
   source venv/bin/activate

3) Run the UI:
   python main.py

Config via environment variables
--------------------------------
The backend reads these environment variables (optional):

- IR_CHIP         (default: gpiochip0)
- IR_LINE         (default: 17)
- IR_ACTIVE_LOW   (default: 1)
- IR_DEBOUNCE_MS  (default: 50)

Examples:
IR_LINE=23 IR_ACTIVE_LOW=1 python main.py

Permissions
-----------
If GPIO access fails due to permissions, try running the app with sudo (temporary workaround):
sudo -E ./venv/bin/python main.py

A cleaner solution (later) is to configure udev rules / groups for gpio access.

Troubleshooting
---------------
### QThread: Destroyed while thread is still running
This means the UI closed before the worker thread stopped. Ensure:
- `app.aboutToQuit.connect(backend.stop)` is present in `main.py`,
- backend `stop()` calls thread termination + `wait()`.

### No display over SSH
Qt apps need a display server. Options:
- Run on the Pi with a monitor (local desktop),
- SSH with X11 forwarding: `ssh -X user@pi`, or
- Use VNC / remote desktop.

Next steps (optional improvements)
----------------------------------
- Replace stdout parsing with a proper IPC (UNIX domain sockets) for bidirectional control (e.g., lock control).
- Add systemd services for automatic startup.
- Add a mock provider for development on PC without hardware.
