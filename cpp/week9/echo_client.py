# 目标：分两批发送 hello/world，并验证 server 返回完全相同的两条 lines。
# 验证：收到 exact bytes 后打印 CLIENT PASS；EOF/timeout/内容错误都会失败。
import socket


def recv_exact(sock: socket.socket, expected_size: int) -> bytes:
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
    sock.sendall(b"hel")
    sock.sendall(b"lo\nworld\n")
    actual = recv_exact(sock, len(expected))

if actual != expected:
    raise RuntimeError(f"unexpected response: {actual!r}")

print("CLIENT PASS")