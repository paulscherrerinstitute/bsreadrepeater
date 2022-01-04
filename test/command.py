import argparse
import zmq

parser = argparse.ArgumentParser(description='')
parser.add_argument('--cmd')
args = parser.parse_args()

ctx = zmq.Context()
sock = zmq.Socket(ctx, zmq.REQ)
sock.connect("tcp://127.0.0.1:4242")
if args.cmd == "add":
    sock.send(b"add")
if args.cmd == "remove":
    sock.send(b"remove")
if args.cmd == "exit":
    sock.send(b"exit")
b = sock.recv()
print(b.decode())
