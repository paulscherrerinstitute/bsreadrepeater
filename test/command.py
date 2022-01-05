import argparse
import zmq

parser = argparse.ArgumentParser(description='')
parser.add_argument('--cmd')
args = parser.parse_args()
print(f"cmd: {args.cmd}")

ctx = zmq.Context()
sock = zmq.Socket(ctx, zmq.REQ)
sock.connect("tcp://127.0.0.1:4242")
sock.send(args.cmd.encode())
b = sock.recv()
print(f"response: {b.decode()}")
