import argparse
import zmq

parser = argparse.ArgumentParser()
parser.add_argument('--cmd')
args = parser.parse_args()
ctx = zmq.Context()
cmd_addr = "tcp://127.0.0.1:4242"

def cmd(ctx, s):
    global cmd_addr
    sock = zmq.Socket(ctx, zmq.REQ)
    sock.connect(cmd_addr)
    sock.send(s.encode())
    b = sock.recv()
    print("Response: %s", b.decode())

cmd(ctx, "add-source,tcp://127.0.0.1:50000")
cmd(ctx, "add-output,tcp://127.0.0.1:50000,tcp://127.0.0.1:50001")
