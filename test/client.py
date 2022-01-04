import time
import zmq
ctx = zmq.Context()
sock = zmq.Socket(ctx, zmq.PULL)
sock.set(zmq.SNDHWM, 12)
sock.set(zmq.SNDBUF, 512)
sock.set(zmq.RCVHWM, 12)
sock.set(zmq.RCVBUF, 512)
sock.connect("tcp://127.0.0.1:50001")
i = 0
while True:
    m = sock.recv()
    more = sock.get(zmq.RCVMORE)
    s = m.decode()
    print(f"got: more: {more}  {s}")
