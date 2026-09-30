import psutil

for proc in psutil.process_iter(['pid', 'cmdline', 'name']):
    try:
        cmd = " ".join(proc.info['cmdline'] or [])
        if "app_script.py" in cmd:
            print(f"Killing {proc.pid}")
            proc.kill()
    except:
        pass