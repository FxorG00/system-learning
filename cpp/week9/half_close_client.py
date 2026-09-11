import socket

payload = b"hello\nworld\n"

with socket.create_connection(("127.0.0.1", 9091), timeout=5.0) as sock:
    sock.settimeout(5.0)
    sock.sendall(payload)
    sock.shutdown(socket.SHUT_WR)

    received = bytearray()

    try:
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            received.extend(chunk)
    except TimeoutError:
        print(f"TIMEOUT after receiving: {bytes(received)!r}")
        raise

if bytes(received) != payload:
    raise RuntimeError(
        f"echo mismatch: expected={payload!r}, actual={bytes(received)!r}"
    )

print(f"HALF CLOSE PASS bytes={len(received)}")