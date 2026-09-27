"""Exercise cross-process loading against only the test host we create."""
import subprocess
import sys
import time

host, loader = sys.argv[1:]
process = subprocess.Popen([host, "--serve"], creationflags=subprocess.CREATE_NO_WINDOW,
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE)
try:
    time.sleep(1)
    if process.poll() is not None:
        raise RuntimeError(process.communicate())
    def call(*options):
        result = subprocess.run([loader, "--test-host", "--pid", str(process.pid), *options],
                                capture_output=True, text=True, timeout=35)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        return result.stdout
    assert "not loaded" in call("--status")
    assert "Hooks installed" in call()
    for attempt in range(50):
        state = call("--status")
        if "DX11 ready: 1" in state:
            break
        time.sleep(.1)
    assert "Running: 1" in state and "DX11 ready: 1" in state, state
    assert "Overlay stopped" in call("--stop")
    state = call("--status")
    assert "Running: 0" in state and "DX11 ready: 0" in state, state
    assert "Hooks installed" in call()
    time.sleep(.2)
    assert "DX11 ready: 1" in call("--status")
    call("--stop")
    print("Cross-process DLL loading, status, shutdown, and restart passed.")
finally:
    # This is our disposable test process, never a game process.
    process.terminate()
    process.communicate(timeout=5)
