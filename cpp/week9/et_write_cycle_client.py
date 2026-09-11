# 目标：在同一 connection 上制造两轮 large response，验证 ET 模式下
# EPOLLOUT 被移除后，可以在新 output 到来时重新加入并继续推进。
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


payloads = [
    b"a" * (4 * 1024 * 1024) + b"\n",
    b"b" * (4 * 1024 * 1024) + b"\n",
]

with socket.create_connection(("127.0.0.1", 9091), timeout=5.0) as sock:
    sock.settimeout(20.0)

    for round_index, payload in enumerate(payloads, start=1):
        sock.sendall(payload)

        # 暂停 application recv，让 server send buffer 更容易到达 EAGAIN。
        time.sleep(1.0)

        response = recv_exact(sock, len(payload))
        if response != payload:
            raise RuntimeError(f"round {round_index}: echo payload mismatch")

        print(f"ROUND {round_index} PASS bytes={len(response)}")

print("ET WRITE CYCLE PASS")