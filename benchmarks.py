#!/usr/bin/env python3
import serial
import sys
import os
import subprocess
import hashlib
import datetime
import time
from utils import m4ignore

dev = serial.Serial("/dev/cu.usbmodem1423", 115200,timeout=10)

def benchmarkBinary(benchmark, binary):
    binpath = os.path.join("bin", binary)

    info = binary.split('_')
    primitive = '_'.join(info[:2])
    scheme = '_'.join(info[2:-2])
    implementation = info[-2]

    if m4ignore(primitive, scheme, implementation):
        return

    # if len(sys.argv) > 1 and scheme not in sys.argv[1:]:
    #     return

    # subprocess.run(["st-flash", "write", binpath, "0x8000000"],
    #                stdout=sys.stdout.buffer, stderr=sys.stdout.buffer)

    if(to_flash_or_not == 0):
        print("Flashing {}..".format(binpath))
        str = "program " + binpath + " 0x08000000 verify reset exit"
        subprocess.run(["openocd", "-f", "interface/stlink-v2.cfg", "-f", "target/stm32f4x.cfg", "-c", str])
        print("Flashed, now running benchmarks..".format(binary))
    # state = 'waiting'
    # marker = b''

    for j in range(prob_parameter_low,prob_parameter_high,prob_parameter_step):
        for i in range(limit_parameter_low,limit_parameter_high,limit_parameter_step):
            print("Limit parameter: " + repr(i))
            print("Prob parameter: " + repr(j))
            a = 0
            sent = 0
            written = 0
            received_marker = 0
            state = 'waiting'
            marker = b''
            while True:
                if(received_marker == 0):
                    x = dev.read()
                if x == b'' and state == 'waiting':
                    print("timed out while waiting for the markers")
                    benchmarkBinary(benchmark, binary)
                    return

                if state == 'waiting':
                    if x == b'=':
                        marker += x
                        if(marker.count(b'=') > 5):
                            received_marker = 1
                            x = b''.decode('utf-8')
                        continue
                    # If we saw at least 5 equal signs, assume we've probably started
                    elif received_marker == 1:
                        state = 'reading'
                        vector = []
                        print("  .. found output marker..")
                elif state == 'reading':
                    if x == b'#':
                        break
                    else:
                        if(sent == 0):
                            if(i < 0):
                                limit_sign = bytes([1])
                                limit_param = bytes([abs(i)&0xFF])
                            else:
                                limit_sign = bytes([0])
                                limit_param = bytes([i&0xFF])
                            prob_param = bytes([j&0xFF])
                            # print(chr(i&0xFF))
                            # print(chr(i&0xFF))
                            # print(chr(j&0xFF))
                            # dev.write(limit_sign.encode('utf-8'))
                            # dev.write(limit_param.encode('utf-8'))
                            # dev.write(prob_param.encode('utf-8'))
                            dev.write(limit_sign)
                            dev.write(limit_param)
                            dev.write(prob_param)
                            # x = dev.read(3)
                            # print(x)
                            # x = dev.read(3)
                            # print(x)
                            sent = 1
                            received_marker = 0
                        if(written == 1):
                            vector.append(x)
                        written = 1
            # timestamp = datetime.datetime.fromtimestamp(time.time()).strftime('%Y%m%d%H%M%S')
            ii = "iteration_loop_count_%d_%d" %(i,j)
            filename = os.path.join('benchmarks/', benchmark, primitive, scheme, implementation, ii)
            os.makedirs(os.path.dirname(filename), exist_ok=True)
            with open(filename, 'w') as f:
                f.write(b''.join(vector).decode('utf-8').strip())
            print("  .. wrote benchmarks!")

            #Calculating average in the files...
            # average = 0
            # count = 0
            # f = open(filename, "r")
            # for x in f:
            #     average = average+int(x)
            #     count = count+1
            # average = float(average)/count
            # print("Average Ticks: " + repr(average))

def doBenchmarks(benchmark):
    try:
        binaries = [x for x in os.listdir('bin') if (benchmark+".bin") in x]
        print(binaries)
    except FileNotFoundError:
        print("There is no bin/ folder. Please first make binaries.")
        sys.exit(1)

    print("This script flashes the benchmarking binaries onto the board, ")
    print(" and then writes the resulting output to the benchmarks directory.")

    for binary in binaries:
        benchmarkBinary(benchmark, binary)

to_flash_or_not = int(sys.argv[1])
prob_parameter_low = int(sys.argv[2])
prob_parameter_high = int(sys.argv[3])
prob_parameter_step = int(sys.argv[4])
limit_parameter_low = int(sys.argv[5])
limit_parameter_high = int(sys.argv[6])
limit_parameter_step = int(sys.argv[7])
doBenchmarks("speed")
