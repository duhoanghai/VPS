#!/usr/bin/env python3
"""End-to-end: Local = UDP = TCP; prueft ausserdem mehrere Client-Zahlen."""
import pathlib
import socket
import subprocess
import tempfile
import time

ROOT = pathlib.Path(__file__).resolve().parents[1]


def free_port(protocol):
    kind = socket.SOCK_DGRAM if protocol == 'udp' else socket.SOCK_STREAM
    with socket.socket(socket.AF_INET, kind) as sock:
        sock.bind(('127.0.0.1', 0))
        return sock.getsockname()[1]


def call(args, timeout=40):
    return subprocess.run(args, cwd=ROOT, capture_output=True, text=True, timeout=timeout, check=True)


def run_case(protocol, clients, steps, input_path, expected_path, output_path):
    port = free_port(protocol)
    server = subprocess.Popen([
        './bin/server', '--protocol', protocol, '--port', str(port),
        '--clients', str(clients), '--steps', str(steps),
        '--input', str(input_path), '--output', str(output_path)
    ], cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    children = []
    try:
        time.sleep(0.2)
        for _ in range(clients):
            children.append(subprocess.Popen([
                './bin/client', '--protocol', protocol, '--host', '127.0.0.1',
                '--port', str(port)
            ], cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True))
        for child in children:
            stdout, stderr = child.communicate(timeout=30)
            assert child.returncode == 0, (protocol, stdout, stderr)
        stdout, stderr = server.communicate(timeout=15)
        assert server.returncode == 0, (protocol, stdout, stderr)
        assert output_path.read_bytes() == expected_path.read_bytes(), protocol + ' != Lokal'
        print(f'PASS {protocol.upper()}, {clients} Clients, {steps} Schritte')
    finally:
        for process in children + [server]:
            if process.poll() is None:
                process.kill()
                process.communicate()


def main():
    with tempfile.TemporaryDirectory(prefix='vps-blatt2-') as tmp:
        tmp = pathlib.Path(tmp)
        original = ROOT / 'boards/glider.rle'
        cases = [(original, 1, 3), (original, 3, 6),
                 (ROOT / "boards/rectangular.rle", 2, 4),
                 (ROOT / "boards/rectangular.rle", 5, 2)]
        for input_path, clients, steps in cases:
            label = f"{input_path.stem}-{clients}"
            local = tmp / f"local-{label}.rle"
            call(["./bin/local", "--input", str(input_path), "--steps", str(steps), "--output", str(local)])
            for protocol in ("udp", "tcp"):
                run_case(protocol, clients, steps, input_path, local, tmp / f"{protocol}-{label}.rle")


if __name__ == '__main__':
    main()
