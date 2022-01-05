import argparse
import zmq

parser = argparse.ArgumentParser()
parser.add_argument('--cmd')
args = parser.parse_args()

ctx = zmq.Context()
sock = zmq.Socket(ctx, zmq.REQ)
sock.connect("tcp://127.0.0.1:4242")
sock.send(b"add-source,tcp://SIN-CVME-TIFGUN:9999")
b = sock.recv()
print(f"Response: {b.decode()}")
