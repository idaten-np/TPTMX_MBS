#!/usr/bin/python
import argparse
import sys
from subprocess import call, Popen, PIPE
from time import sleep
import numpy as np

def auto_int(x):
    return int(x, 0)


integration_time = 0.5


parser = argparse.ArgumentParser()
parser.add_argument("sfp", help="SFP number [0...4]", type=auto_int)
parser.add_argument("dev", help="Device number [0...15]", type=auto_int)
#parser.add_argument("int_time", help="Integration time for hit accumulation", type=float)
args = parser.parse_args()

sfp = args.sfp
dev = args.dev


t0_arr = np.zeros(128, dtype=int)
val0_arr = np.zeros(128, dtype=int)
t1_arr = np.zeros(128, dtype=int)
val1_arr = np.zeros(128, dtype=int)
rates = np.zeros(128, dtype=int)


call(["gosipcmd", "-w", "-x", "{0}".format(sfp), "{0}".format(dev), "0x320804", "0x1"]) # stop
call(["gosipcmd", "-w", "-x", "{0}".format(sfp), "{0}".format(dev), "0x320800", "0x1"]) # clear
call(["gosipcmd", "-w", "-x", "{0}".format(sfp), "{0}".format(dev), "0x320804", "0x0"]) # start
sleep(integration_time)
call(["gosipcmd", "-w", "-x", "{0}".format(sfp), "{0}".format(dev), "0x320804", "0x1"]) # stop

for i in range(35):
    regaddr = "0x3200{0:0{1}X}".format(i*4,2)
    data = Popen(["gosipcmd", "-r", "-x", "{0}".format(sfp), "{0}".format(dev), "{0}".format(regaddr)], stdout=PIPE).communicate()[0]
    #print data
    #print len(data)
    if len(data) > 7:
        val_h = "{0}".format(data[-6:])
    else:
        j = 8 - len(data)
        val_h = "{0}".format(data[-6+j:])
    #print len(val_h)
    #print val_h
    val0_arr[i] = int(val_h, 16)
    #print val0_arr[i]
    rates[i] = val0_arr[i]/integration_time

print "CH"
for i in range(32):
    j=i+1
    if j%4==0:
        print "#{4}-{5}\t{0:12}{1:12}{2:12}{3:12}".format(rates[i-3],rates[i-2],rates[i-1],rates[i],i-3,i)

print "TRIG1\t{0}".format(rates[32])
print "TRIG3\t{0}".format(rates[33])
#print "REF_CLK\t{0}".format(rates[34])
