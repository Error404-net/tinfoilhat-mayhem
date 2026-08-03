#!/usr/bin/env python3
"""Talk to the PortaPack Mayhem USB serial shell (ChibiOS 'ch> ' prompt).

Usage:
  ppserial.py cmd "ls /APPS"          # run one shell command, print reply
  ppserial.py md5 /APPS/tinfoilhat.ppma [local_file_to_compare]
  ppserial.py launch tinfoilhat       # appstart <name> — watch it load

No pyserial needed: the CDC-ACM device ignores termios settings.
Port autodetected (first /dev/cu.usbmodem*) or pass PPSERIAL=/dev/... env.
"""
import glob
import hashlib
import os
import select
import sys
import termios
import time
import tty

PROMPT = b"ch> "


def open_port():
    port = os.environ.get("PPSERIAL")
    if not port:
        cands = sorted(glob.glob("/dev/cu.usbmodem*"))
        if not cands:
            sys.exit("no /dev/cu.usbmodem* found — is the PortaPack plugged in?")
        port = cands[0]
    try:
        fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    except OSError as e:
        sys.exit(f"cannot open {port}: {e}\n"
                 "(if 'Resource busy': close the Mayhem Hub browser tab / other serial app)")
    tty.setraw(fd)
    return fd, port


def read_until(fd, stop=PROMPT, timeout=5.0):
    buf = b""
    deadline = time.time() + timeout
    while time.time() < deadline:
        r, _, _ = select.select([fd], [], [], 0.2)
        if r:
            try:
                chunk = os.read(fd, 65536)
            except OSError:
                break
            buf += chunk
            if stop and buf.endswith(stop):
                break
    return buf


def shell(fd, command, timeout=5.0):
    os.write(fd, command.encode() + b"\r\n")
    out = read_until(fd, timeout=timeout)
    text = out.decode(errors="replace")
    # strip echoed command and trailing prompt
    lines = text.replace("\r", "").split("\n")
    if lines and lines[0].strip() == command.strip():
        lines = lines[1:]
    if lines and lines[-1].strip() == PROMPT.decode().strip():
        lines = lines[:-1]
    return "\n".join(lines).strip()


def cmd_md5(fd, remote_path, local_path=None):
    size_reply = shell(fd, f"filesize {remote_path}")
    try:
        size = int(size_reply.split()[-1])
    except (ValueError, IndexError):
        sys.exit(f"filesize failed: {size_reply!r}")
    print(f"remote size: {size}")

    reply = shell(fd, f"fopen {remote_path}")
    if "ok" not in reply.lower():
        sys.exit(f"fopen failed: {reply!r}")

    data = bytearray()
    while len(data) < size:
        n = min(512, size - len(data))
        reply = shell(fd, f"fread {n}", timeout=10)
        # fread replies with hex bytes; keep only hex-looking tokens
        hexstr = "".join(t for t in reply.split() if all(c in "0123456789abcdefABCDEF" for c in t) and len(t) % 2 == 0)
        if not hexstr:
            sys.exit(f"fread returned no data at offset {len(data)}: {reply!r}")
        data += bytes.fromhex(hexstr)
        if len(data) % 5120 == 0:
            print(f"  {len(data)}/{size}")
    shell(fd, "fclose")

    data = bytes(data[:size])
    digest = hashlib.md5(data).hexdigest()
    print(f"remote md5:  {digest}")

    if local_path:
        local = hashlib.md5(open(local_path, "rb").read()).hexdigest()
        print(f"local  md5:  {local}  ({local_path})")
        print("MATCH — on-card file is byte-identical to the artifact" if local == digest
              else "MISMATCH — the file on the SD card is NOT the CI artifact; re-copy it")


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    fd, port = open_port()
    print(f"[{port}]")
    os.write(fd, b"\r\n")
    read_until(fd, timeout=1.0)  # drain banner/prompt

    action = sys.argv[1]
    if action == "cmd":
        print(shell(fd, sys.argv[2], timeout=8))
    elif action == "md5":
        cmd_md5(fd, sys.argv[2], sys.argv[3] if len(sys.argv) > 3 else None)
    elif action == "launch":
        print(shell(fd, f"appstart {sys.argv[2]}", timeout=8))
    else:
        sys.exit(__doc__)
    os.close(fd)


if __name__ == "__main__":
    main()
