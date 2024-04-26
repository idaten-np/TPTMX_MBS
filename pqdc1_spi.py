#!/usr/bin/python

#                   sfp dev ch  reg subreg  com data
#./pqdc1_spi.py     1   0   1   0x0 1       w   0x4800

import argparse
import sys
from subprocess import call, Popen, PIPE
from time import sleep

def auto_int(x):
    return int(x, 0)

parser = argparse.ArgumentParser()
parser.add_argument("sfp", help="SFP number [0...4]", type=auto_int)
parser.add_argument("dev", help="device number [0...15]", type=auto_int)
parser.add_argument("ch", help="SPI channel: [0,1,2,3,4=broadcast (write only)]", type=auto_int)
parser.add_argument("register", help="8 bit hex address of target register", type=auto_int)
parser.add_argument("subregister", help="decimal address of subregister [0...15]", type=auto_int)
parser.add_argument("command", help="r = read, w = write")
parser.add_argument("data", help="16 bit data word for write operation", type=auto_int)
args = parser.parse_args()
#print(args.threshold)
#print(hex(args.threshold))

sfp = args.sfp
dev = args.dev
ch = args.ch
channel = ""
if ch == 0:
    channel = "0x1"
elif ch == 1:
    channel = "0x2"
elif ch == 2:
    channel = "0x4"
elif ch == 3:
    channel = "0x8"
elif ch == 4:
    channel = "0xf"
else:
    print("Error>>> Invalid SPI channel parameter \'{0}\'. Exiting...".format(ch))
    sys.exit(0)

reg = args.register
reg_b = bin(reg)[2:].zfill(8)
cmd_str = args.command
cmd = 0
if cmd_str == 'r':
    cmd = 0
elif cmd_str == 'w':
    cmd = 8
else:
    print("Error>>> Invalid command parameter \'{0}\'. Exiting...".format(cmd_str))
    sys.exit(0)
cmd_b = bin(cmd)[2:].zfill(4)
subreg = args.subregister
subreg_b = bin(subreg)[2:].zfill(4)
#if cmd == 8:
#    if args.data:
#        data = args.data
#        data_b = bin(data)[2:].zfill(16)
#    else:
#        print("Error>>> Missing data parameter for write operation. Exiting...")
#        sys.exit(0)
#else:
#    data = 0
#    data_b = bin(data)[2:].zfill(16)
data = args.data
data_b = bin(data)[2:].zfill(16)

s = ""
seq = (reg_b, cmd_b, subreg_b, data_b)
joined = s.join(seq)
assembled_command = hex(int(joined,2))
#print(len(joined))
#print(joined)
#print(assembled_command)

print("gosipcmd -w -x {0} {1} 0x311018 {2}".format(sfp, dev, assembled_command))
call(["gosipcmd", "-w", "-x", "{0}".format(sfp), "{0}".format(dev), "0x311018", "{0}".format(assembled_command)])
call(["gosipcmd", "-w", "-x", "{0}".format(sfp), "{0}".format(dev), "0x311014", "{}1".format(channel)])
call(["gosipcmd", "-w", "-x", "{0}".format(sfp), "{0}".format(dev), "0x311014", "{}0".format(channel)])
if cmd == 0:
    sys.stdout.write("Readback: ")
    p = Popen(["gosipcmd", "-r", "-x", "{0}".format(sfp), "{0}".format(dev), "0x31101c"], stdout=PIPE)
    sys.stdout.write(p.communicate()[0])
    sys.stdout.flush()
