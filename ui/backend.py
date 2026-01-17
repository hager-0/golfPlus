import os
import subprocess
from PySide6.QtCore import QObject, Signal, Property, QThread

class IRReaderThread(QThread):
    lineReceived = Signal(str)

    def __init__(self, cmd, cwd=None):
        super().__init__()
        self.cmd = cmd
        self.cwd = cwd
        self.proc = None

    def run(self):
        try:
            self.proc = subprocess.Popen(
                self.cmd,
                cwd=self.cwd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1
            )
            for line in self.proc.stdout:
                if line:
                    self.lineReceived.emit(line.strip())
        except Exception as e:
            self.lineReceived.emit(f"ERROR: {e}")

    def stop(self):
        if self.proc and self.proc.poll() is None:
            self.proc.terminate()


class Backend(QObject):
    detectedChanged = Signal()
    statusTextChanged = Signal()

    def __init__(self):
        super().__init__()
        self._detected = False
        self._status_text = "Status: starting..."

        # ---- Config (???? ??????? ??? ??????) ----
        chip = os.getenv("IR_CHIP", "gpiochip0")
        line = os.getenv("IR_LINE", "17")
        active_low = os.getenv("IR_ACTIVE_LOW", "1")
        debounce_ms = os.getenv("IR_DEBOUNCE_MS", "50")

        # ???? ir_event (????? ?? ????? ?????)
        # ????? UI ?? ~/texi/ui ? ir_event ?? ~/texi/build/logic/ir_event
        base = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
        exe = os.path.join(base, "build", "logic", "ir_event")

        self._status_text = f"Status: launching {exe}"
        self.statusTextChanged.emit()

        cmd = [exe, "--chip", chip, "--line", line, "--active-low", active_low, "--debounce-ms", debounce_ms]
        self.reader = IRReaderThread(cmd, cwd=base)
        self.reader.lineReceived.connect(self.on_line)
        self.reader.start()

    def getDetected(self) -> bool:
        return self._detected

    def setDetected(self, value: bool):
        if self._detected == value:
            return
        self._detected = value
        self.detectedChanged.emit()

    detected = Property(bool, getDetected, setDetected, notify=detectedChanged)

    def getStatusText(self) -> str:
        return self._status_text

    def setStatusText(self, text: str):
        if self._status_text == text:
            return
        self._status_text = text
        self.statusTextChanged.emit()

    statusText = Property(str, getStatusText, notify=statusTextChanged)

    def on_line(self, line: str):
        # ???? ??? ??? ?????
        self.setStatusText(line)

        # ??? strings ???? ir_event ???????
        if "OBJECT DETECTED" in line:
            self.setDetected(True)
        elif "NO OBJECT" in line:
            self.setDetected(False)

    def stop(self):
        if self.reader:
            self.reader.stop()
            self.reader.quit()
            self.reader.wait(1000)
