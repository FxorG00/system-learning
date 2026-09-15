"""从进程外验证 Reactor Echo Server 的最小 byte-stream 行为。"""

import socket


def recv_exact(sock: socket.socket, expected_size: int) -> bytes:
    """持续接收，直到得到 expected_size bytes；提前 EOF 视为失败。"""
    received = bytearray()
    while len(received) < expected_size:
        chunk = sock.recv(expected_size - len(received))
        if not chunk:
            raise RuntimeError("unexpected EOF before complete response")
        received.extend(chunk)
    return bytes(received)


expected = b"hello\nworld\n"

with socket.create_connection(("127.0.0.1", 9091), timeout=3.0) as sock:
    sock.settimeout(3.0)

    # 第一批不含完整 line，用来验证 incomplete suffix 会跨 event 保留。
    sock.sendall(b"hel")

    # 第二批既补全第一条 line，又一次带来第二条 line。
    sock.sendall(b"lo\nworld\n")
    actual = recv_exact(sock, len(expected))

if actual != expected:
    raise RuntimeError(f"unexpected response: {actual!r}")

print("REACTOR ECHO SMOKE PASS")