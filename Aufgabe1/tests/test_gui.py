"""Benötigt einen laufenden X-Server (DISPLAY) und bin/gui_probe; siehe README.md."""
from pathlib import Path
import subprocess
import tempfile

with tempfile.TemporaryDirectory() as directory:
    gui = str(Path(directory)/'gui.rle')
    headless = str(Path(directory)/'headless.rle')
    # Normaler Fensterlebenszyklus und Aktualisierung der Generationen.
    subprocess.run(['./bin/main','-i','boards/glider.rle','-r','3','-g','-o',gui],check=True,timeout=15)
    subprocess.run(['./bin/main','-i','boards/glider.rle','-r','3','-o',headless],check=True,timeout=15)
    assert Path(gui).read_text()==Path(headless).read_text()
    # Frühes Schließen beendet das Zeichnen, aber nicht die angeforderten Schritte.
    probe = subprocess.Popen(['./bin/gui_probe','tmp/gui-preview.ppm'])
    try:
        subprocess.run(['./bin/main','-i','boards/glider.rle','-r','20','-g','-o',gui],check=True,timeout=15)
        assert probe.wait(timeout=10)==0
    finally:
        if probe.poll() is None:
            probe.kill()
            probe.wait()
    subprocess.run(['./bin/main','-i','boards/glider.rle','-r','20','-o',headless],check=True,timeout=15)
    assert Path(gui).read_text()==Path(headless).read_text()
print('PASS: GUI output matches headless output, including after early window close')
