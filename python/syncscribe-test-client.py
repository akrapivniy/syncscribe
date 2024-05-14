#!/usr/bin/env python3

import sys
import ctypes
import time
from syncscribe import *


def cb(client, cid, data, size):
    id = bytes(ctypes.cast(cid, ctypes.POINTER(ctypes.c_char * 32)).contents).decode()
    data_size = ctypes.c_uint(size).value
    data_event = ctypes.cast(data, ctypes.POINTER(ctypes.c_uint8 * data_size)).contents
    print ("Update event ID:",id, " data size:", data_size, " data:", data_event)

def main():

    client = syncscribe("hello", "127.0.0.1", 4444, "tcp")
    x = 1
    client.subscribe_event ("int", "mode" , cb , x)
    client.connect_wait(1000)

    while True:
        data = client.read_int32 (0, "pause")
        print ("Receive:",data)
        client.write_int32 (0, "pause", x)
        x+=1
        time.sleep(1)

if __name__ == '__main__':
    main()
