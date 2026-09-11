# 目标：发送 4 MiB newline message，暂停读取，再验证完整 echo。
# 验证：server 不能丢失或重复任何 byte；成功时打印 SLOW CLIENT PASS。
import socket
import time


def recv_exact(sock: socket.socket, expected_size: int) -> bytes:
    received = bytearray()
    while len(received) < expected_size:
        chunk = sock.recv(min(65536, expected_size - len(received)))
        if not chunk:
            raise RuntimeError(
                f"unexpected EOF: got {len(received)} of {expected_size} bytes"
            )
        received.extend(chunk)
    return bytes(received)


payload = b"x" * (4 * 1024 * 1024) + b"\n"

with socket.create_connection(("127.0.0.1", 9091), timeout=5.0) as sock:
    sock.settimeout(15.0)
    sock.sendall(payload)

    # 暂停 application recv，让 server 更容易积累 pending output。
    time.sleep(1.0)

    response = recv_exact(sock, len(payload))

if response != payload:
    raise RuntimeError("echo payload mismatch")

print(f"SLOW CLIENT PASS bytes={len(response)}")