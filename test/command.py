import zmq
ctx = zmq.Context()
sock = zmq.Socket(ctx, zmq.REQ)
sock.connect("tcp://127.0.0.1:4242")
sock.send(b"Some message")
b = sock.recv()
sock.send(b"exit")
print(b.decode())
