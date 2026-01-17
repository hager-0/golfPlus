import sys
import signal
from pathlib import Path

from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import QQmlApplicationEngine

from backend import Backend

def main():
    app = QGuiApplication(sys.argv)
    engine = QQmlApplicationEngine()

    backend = Backend()

    # IMPORTANT: expose backend BEFORE loading QML
    engine.rootContext().setContextProperty("backend", backend)

    qml_file = Path(__file__).resolve().parent / "main.qml"
    engine.load(qml_file)

    if not engine.rootObjects():
        # If QML failed, stop backend cleanly
        backend.stop()
        return -1

    # Ensure closing the window triggers cleanup
    app.aboutToQuit.connect(backend.stop)

    # Handle Ctrl+C / SIGTERM: quit Qt event loop cleanly
    def _handle_sig(_sig, _frame):
        app.quit()

    signal.signal(signal.SIGINT, _handle_sig)
    signal.signal(signal.SIGTERM, _handle_sig)

    return app.exec()

if __name__ == "__main__":
    raise SystemExit(main())
