import time
import zmq
ctx = zmq.Context()
sock = zmq.Socket(ctx, zmq.PUSH)
sock.set(zmq.SNDHWM, 16)
sock.set(zmq.SNDBUF, 512)
sock.set(zmq.RCVHWM, 6)
sock.set(zmq.RCVBUF, 64)
sock.bind("tcp://127.0.0.1:50000")
i = 0
while True:
    s = f"<{i:05d}>"
    s = "--".join((s, s, s))
    sock.send(s.encode(), flags=zmq.SNDMORE)
    sock.send(s.encode(), flags=zmq.SNDMORE)
    sock.send(s.encode(), flags=0)
    print(f"Sent: {s}")
    i += 1
    if i % 1 == 0:
        time.sleep(0.02 * 1)
