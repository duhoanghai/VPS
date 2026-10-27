"""CLI-Tests von außen; benötigt nur die Python-Standardbibliothek."""
import os
from pathlib import Path
import subprocess
import tempfile

EXE = str(Path('bin/main').resolve())
def run(*args, ok=True):
    env = dict(os.environ, DISPLAY='')  # Läufe ohne GUI dürfen keinen X-Server benötigen.
    result = subprocess.run([EXE, *args], capture_output=True, text=True, env=env, timeout=30)
    assert (result.returncode == 0) == ok, (args, result.stdout, result.stderr)
    return result

assert '100 steps' in run().stdout
assert 'Usage:' in run('--help').stdout
for args in [('-r',), ('-i',), ('-o',), ('-r','-1'), ('-r','2.5'), ('-r','2x'),
             ('-r',''), ('-r','99999999999999999999999999'), ('--wrong',), ('-i','missing.rle')]:
    run(*args, ok=False)
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    zero, result, repeated = (str(root / name) for name in ['zero.rle','out.rle','repeat.rle'])
    run('-i','boards/glider.rle','-r','0','-o',zero)
    run('-i','boards/glider.rle','-r','20','-o',result)
    run('-i',zero,'-r','20','-o',repeated)
    assert Path(result).read_text() == Path(repeated).read_text()
    run('-i',result,'-r','0','-o',result)  # Das Einlesen endet vor dem Öffnen der Ausgabe.
    assert Path(result).read_text() == Path(repeated).read_text()
    run('-r','0','-o',directory,ok=False)
    run('-i','boards/glider.rle','-r','0','-g',ok=False)
print('PASS: defaults, flags, invalid arguments, reproducible RLE input, output errors, missing display')
