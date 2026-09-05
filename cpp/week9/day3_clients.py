"""Exercise an idle A, active B, later A, and a fresh C against a read-only server."""
import socket
import sys


def send_then_expect_eof(sock, payload):
    """Borrow sock; send bytes, end writing, and require server EOF without echo."""
    sock.sendall(payload)
    sock.shutdown(socket.SHUT_WR)
    result = sock.recv(1)
    if result != b"":
        raise RuntimeError("read-only server unexpectedly sent response bytes")


def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 9090
    endpoint = ("127.0.0.1", port)
    with socket.create_connection(endpoint, timeout=5) as a:
        print("A connected; no payload sent.", flush=True)
        input("Check server has ACCEPTed A, then press Enter: ")
        with socket.create_connection(endpoint, timeout=5) as b:
            send_then_expect_eof(b, b"B-data")
        print("B reached EOF while A remained open and idle.", flush=True)

        send_then_expect_eof(a, b"A-later")
        print("A later sent data and reached EOF.", flush=True)

    with socket.create_connection(endpoint, timeout=5) as c:
        send_then_expect_eof(c, b"C-new")
    print("CLIENT CHECK PASS: B, later A, and fresh C completed.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, RuntimeError, EOFError) as error:
        print("CLIENT CHECK FAIL:", error, file=sys.stderr)
        raise SystemExit(1)