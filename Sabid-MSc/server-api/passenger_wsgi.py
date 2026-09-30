import os
import sys
import json
import threading
import traceback
import tempfile
import signal
import subprocess
import errno
from datetime import datetime, timezone

sys.path.insert(0, os.path.dirname(__file__))

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPT_FILE = os.path.join(BASE_DIR, "app_script.py")
FLOW_FILE = os.path.join(BASE_DIR, "visual_flow.json")
LOG_FILE = os.path.join(BASE_DIR, "wsgi_runtime.log")
SCRIPT_LOG_FILE = os.path.join(BASE_DIR, "logs.txt")
SCRIPT_STDOUT_FILE = os.path.join(BASE_DIR, "app_script.stdout.log")
RUNTIME_STATE_FILE = os.path.join(BASE_DIR, "runtime_state.json")
PID_FILE = os.path.join(BASE_DIR, "runtime.pid")

# Global state
worker_thread = None
thread_lock = threading.Lock()
last_start_message = "never started"
last_stop_message = "never stopped"
last_runtime_error = ""


def write_json_atomic(path, payload):
    directory = os.path.dirname(path) or "."
    os.makedirs(directory, exist_ok=True)

    fd, temp_path = tempfile.mkstemp(prefix=".tmp-", suffix=".json", dir=directory)
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as f:
            json.dump(payload, f)
            f.flush()
            os.fsync(f.fileno())

        os.replace(temp_path, path)

        # Best-effort directory fsync so rename is durable on more filesystems.
        try:
            dir_fd = os.open(directory, os.O_RDONLY)
            try:
                os.fsync(dir_fd)
            finally:
                os.close(dir_fd)
        except Exception:
            pass
    finally:
        if os.path.exists(temp_path):
            try:
                os.remove(temp_path)
            except Exception:
                pass


def read_json(path, fallback=None):
    try:
        with open(path, "r", encoding="utf-8") as f:
            return json.load(f)
    except Exception:
        return fallback


def now_utc_iso():
    return datetime.now(timezone.utc).isoformat()


def log_runtime(message):
    line = f"[{now_utc_iso()}] {message}"
    print(line)
    try:
        with open(LOG_FILE, "a", encoding="utf-8") as f:
            f.write(line + "\n")
    except Exception:
        traceback.print_exc()


# Ensure files exist
def ensure_files():
    os.makedirs(BASE_DIR, exist_ok=True)

    if not os.path.exists(SCRIPT_FILE):
        with open(SCRIPT_FILE, "w", encoding="utf-8") as f:
            f.write("")
        log_runtime("Created missing app_script.py")

    if not os.path.exists(FLOW_FILE):
        with open(FLOW_FILE, "w", encoding="utf-8") as f:
            json.dump({}, f)
        log_runtime("Created missing visual_flow.json")

    if not os.path.exists(LOG_FILE):
        with open(LOG_FILE, "a", encoding="utf-8"):
            pass

    if not os.path.exists(RUNTIME_STATE_FILE):
        write_runtime_state("stopped", "Initialized runtime state")

    if not os.path.exists(PID_FILE):
        with open(PID_FILE, "w", encoding="utf-8") as f:
            f.write("")

    if not os.path.exists(SCRIPT_STDOUT_FILE):
        with open(SCRIPT_STDOUT_FILE, "a", encoding="utf-8"):
            pass


def get_thread_state():
    if worker_thread and worker_thread.is_alive():
        return "running"
    return "stopped"


def parse_pid(value):
    try:
        pid = int(value)
        return pid if pid > 0 else None
    except Exception:
        return None


def read_pid_file():
    try:
        with open(PID_FILE, "r", encoding="utf-8") as f:
            return parse_pid(f.read().strip())
    except Exception:
        return None


def write_pid_file(pid):
    with open(PID_FILE, "w", encoding="utf-8") as f:
        f.write(str(pid))


def clear_pid_file():
    try:
        with open(PID_FILE, "w", encoding="utf-8") as f:
            f.write("")
    except Exception:
        pass


def is_pid_alive(pid):
    if not pid:
        return False

    try:
        os.kill(pid, 0)
        return True
    except OSError as exc:
        if exc.errno == errno.ESRCH:
            return False
        if exc.errno == errno.EPERM:
            return True
        return False
    except Exception:
        return False


def get_active_runner_pid():
    pid = read_pid_file()
    if pid and is_pid_alive(pid):
        return pid

    shared = read_runtime_state()
    shared_pid = parse_pid(shared.get("pid")) if isinstance(shared, dict) else None
    if shared_pid and is_pid_alive(shared_pid):
        write_pid_file(shared_pid)
        return shared_pid

    return None


def write_runtime_state(state, detail="", error=""):
    active_pid = get_active_runner_pid()
    payload = {
        "state": state,
        "detail": detail,
        "error": error,
        "pid": active_pid,
        "threadAlive": False,
        "updatedAt": now_utc_iso(),
    }
    try:
        write_json_atomic(RUNTIME_STATE_FILE, payload)
    except Exception as exc:
        log_runtime(f"State write failed: {exc}")


def read_runtime_state():
    payload = read_json(RUNTIME_STATE_FILE, {})
    if not isinstance(payload, dict):
        return {}
    return payload


def refresh_runtime_state_from_pid():
    pid = get_active_runner_pid()
    if pid and is_pid_alive(pid):
        write_runtime_state("running", f"Runner pid {pid} is alive")
        shared = read_runtime_state()
        return shared if isinstance(shared, dict) else {"state": "running", "pid": pid}

    clear_pid_file()
    write_runtime_state("stopped", "No active runner process")
    shared = read_runtime_state()
    return shared if isinstance(shared, dict) else {"state": "stopped", "pid": None}


def auto_fix_known_codegen_issues(code):
    # Backward-compat for previously generated malformed f-string line.
    broken = 'f.write(f"[{time.strftime("%Y-%m-%d %H:%M:%S")}] {message}\\n")'
    if broken in code:
        code = code.replace(
            broken,
            "ts = time.strftime('%Y-%m-%d %H:%M:%S')\nf.write(f'[{ts}] {message}\\n')",
        )
    return code


def compile_script_with_diagnostics(code):
    try:
        compiled = compile(code, "<generated_script>", "exec")
        return compiled, None
    except SyntaxError as err:
        line_no = getattr(err, "lineno", None) or 0
        offset = getattr(err, "offset", None) or 0
        msg = getattr(err, "msg", str(err))
        bad_line = ""
        lines = code.splitlines()
        if 1 <= line_no <= len(lines):
            bad_line = lines[line_no - 1].strip()
        detail = f"SyntaxError line {line_no}:{offset} - {msg} | code: {bad_line}"
        return None, detail


def start_process():
    global last_start_message, last_runtime_error

    with thread_lock:
        active_pid = get_active_runner_pid()
        if active_pid:
            last_start_message = "Already running"
            log_runtime(f"Start requested while already running (pid={active_pid})")
            write_runtime_state("running", f"Already running pid={active_pid}")
            return "Already running"

        try:
            with open(SCRIPT_FILE, "r", encoding="utf-8") as f:
                code = f.read()
        except Exception as exc:
            last_runtime_error = f"Unable to read script file: {exc}"
            log_runtime(last_runtime_error)
            write_runtime_state("stopped", "Script read failed", last_runtime_error)
            return f"Failed to read script: {exc}"

        if not code.strip():
            last_runtime_error = "Script start skipped: app_script.py is empty"
            log_runtime(last_runtime_error)
            write_runtime_state("stopped", "Script empty", last_runtime_error)
            return "Script start skipped: app_script.py is empty"

        fixed_code = auto_fix_known_codegen_issues(code)
        compiled_code, compile_error = compile_script_with_diagnostics(fixed_code)
        if compile_error:
            last_runtime_error = compile_error
            log_runtime(f"Script compile error: {compile_error}")
            write_runtime_state("stopped", "Compile error", compile_error)
            return f"Failed to keep running: {compile_error}"

        # Persist compatibility fixes so the subprocess executes validated code.
        if fixed_code != code:
            with open(SCRIPT_FILE, "w", encoding="utf-8") as f:
                f.write(fixed_code)

        del compiled_code

        try:
            with open(SCRIPT_STDOUT_FILE, "ab", buffering=0) as out_file:
                process = subprocess.Popen(
                    [sys.executable, SCRIPT_FILE],
                    cwd=BASE_DIR,
                    stdout=out_file,
                    stderr=out_file,
                    start_new_session=True,
                    env={
                        **os.environ,
                        "PYTHONUNBUFFERED": "1",
                    },
                )
        except Exception as exc:
            last_runtime_error = str(exc)
            log_runtime(f"Failed to spawn process: {exc}")
            write_runtime_state("stopped", "Spawn failed", last_runtime_error)
            return f"Failed to start: {exc}"

        write_pid_file(process.pid)
        last_start_message = "Started"
        last_runtime_error = ""
        log_runtime(f"Start accepted; subprocess pid={process.pid}")
        write_runtime_state("running", f"Started subprocess pid={process.pid}")

    return "Started"


def stop_process():
    global last_stop_message

    with thread_lock:
        pid = get_active_runner_pid()
        if not pid:
            last_stop_message = "Not running"
            log_runtime("Stop requested while not running")
            write_runtime_state("stopped", last_stop_message)
            return "Not running"

        log_runtime(f"Stop requested for pid={pid}")

    try:
        os.killpg(pid, signal.SIGTERM)
    except ProcessLookupError:
        clear_pid_file()
        last_stop_message = "Stopped"
        write_runtime_state("stopped", "Process already exited")
        return "Stopped"
    except Exception as exc:
        last_stop_message = f"Stop failed: {exc}"
        log_runtime(last_stop_message)
        write_runtime_state("running", last_stop_message, str(exc))
        return last_stop_message

    deadline = datetime.now(timezone.utc).timestamp() + 5.0
    while datetime.now(timezone.utc).timestamp() < deadline:
        if not is_pid_alive(pid):
            clear_pid_file()
            last_stop_message = "Stopped"
            log_runtime(f"Process {pid} stopped after SIGTERM")
            write_runtime_state("stopped", f"Process {pid} stopped")
            return "Stopped"

    try:
        os.killpg(pid, signal.SIGKILL)
        log_runtime(f"Process {pid} required SIGKILL")
    except ProcessLookupError:
        pass
    except Exception as exc:
        last_stop_message = f"Stop failed after timeout: {exc}"
        log_runtime(last_stop_message)
        write_runtime_state("running", last_stop_message, str(exc))
        return last_stop_message

    clear_pid_file()
    last_stop_message = "Stopped"
    write_runtime_state("stopped", f"Process {pid} force-killed")
    return "Stopped"


def get_state():
    shared = refresh_runtime_state_from_pid()
    shared_state = shared.get("state") if isinstance(shared, dict) else None
    if shared_state in ("running", "stopped"):
        return shared_state
    return "stopped"


def read_body(environ):
    try:
        size = int(environ.get("CONTENT_LENGTH", 0))
    except:
        size = 0
    return environ["wsgi.input"].read(size).decode("utf-8")


def application(environ, start_response):
    global last_runtime_error
    ensure_files()

    path = environ.get("PATH_INFO", "/")
    method = environ.get("REQUEST_METHOD")

    response = ""
    status = "200 OK"

    # ✅ CORS headers
    headers = [
        ("Content-Type", "text/plain"),
        ("Access-Control-Allow-Origin", "*"),
        ("Access-Control-Allow-Methods", "GET, POST, OPTIONS"),
        ("Access-Control-Allow-Headers", "Content-Type"),
    ]

    # ✅ Handle preflight request
    if method == "OPTIONS":
        log_runtime(f"OPTIONS {path}")
        start_response("200 OK", headers)
        return [b""]

    try:
        log_runtime(f"{method} {path} request")

        # --- ROOT ---
        if path == "/":
            response = "Service is running"

        # --- START ---
        elif path == "/start":
            response = start_process()

        # --- STOP ---
        elif path == "/stop":
            response = stop_process()

        # --- STATE ---
        elif path == "/state":
            shared = read_runtime_state()
            payload = {
                "state": get_state(),
                "threadAlive": False,
                "stopEvent": False,
                "lastStart": last_start_message,
                "lastStop": last_stop_message,
                "lastError": last_runtime_error,
                "timestamp": now_utc_iso(),
                "sharedState": shared.get("state") if isinstance(shared, dict) else None,
                "sharedDetail": shared.get("detail") if isinstance(shared, dict) else None,
                "sharedError": shared.get("error") if isinstance(shared, dict) else None,
                "sharedPid": shared.get("pid") if isinstance(shared, dict) else None,
                "sharedUpdatedAt": shared.get("updatedAt") if isinstance(shared, dict) else None,
            }
            headers[0] = ("Content-Type", "application/json")
            response = json.dumps(payload)

        # --- UPDATE SCRIPT ---
        elif path == "/update_script" and method == "POST":
            body = read_body(environ)

            stop_process()

            with open(SCRIPT_FILE, "w", encoding="utf-8") as f:
                f.write(body)

            log_runtime(f"Script updated ({len(body)} bytes)")
            response = "Script updated"

        # --- GET SCRIPT ---
        elif path == "/get_script":
            with open(SCRIPT_FILE, "r", encoding="utf-8") as f:
                response = f.read()

        # --- GET FLOW ---
        elif path == "/get_flow":
            with open(FLOW_FILE, "r", encoding="utf-8") as f:
                response = f.read()

        # --- SET FLOW ---
        elif path == "/set_flow" and method == "POST":
            body = read_body(environ)

            try:
                data = json.loads(body)
                with open(FLOW_FILE, "w", encoding="utf-8") as f:
                    json.dump(data, f, indent=2)
                log_runtime(f"Flow updated ({len(body)} bytes)")
                response = "Flow updated"
            except:
                status = "400 Bad Request"
                response = "Invalid JSON"
                log_runtime("Flow update rejected: invalid JSON")

        else:
            status = "404 Not Found"
            response = "Not Found"
            log_runtime(f"Unhandled route: {method} {path}")

    except Exception as e:
        status = "500 Internal Server Error"
        response = str(e)
        log_runtime(f"Unhandled exception for {method} {path}: {e}")
        traceback.print_exc()

    start_response(status, headers)
    return [response.encode()]
