# /**************************************************************
# * Description: Wrapper of library of network variables and channels
# * Copyright (c) 2022 Alexander Krapivniy (a.krapivniy@gmail.com)
# *
# * This program is free software: you can redistribute it and/or modify
# * it under the terms of the GNU General Public License as published by
# * the Free Software Foundation, version 3.
# *
# * This program is distributed in the hope that it will be useful, but
# * WITHOUT ANY WARRANTY; without even the implied warranty of
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# * General Public License for more details.
# *
# * You should have received a copy of the GNU General Public License
# * along with this program. If not, see <http://www.gnu.org/licenses/>.
# ***************************************************************/

import ctypes
from datetime import datetime
import os
from socket import htonl, inet_ntoa
import socket
import struct
from syncstypes import *

libsyncs = ctypes.CDLL(os.path.abspath("/home/user/git/syncscribe/libsyncs/libsyncs.so.1.0"))

c_uint8 = ctypes.c_uint8
c_uint16 = ctypes.c_uint16
c_uint32 = ctypes.c_uint32
c_uint32_p = ctypes.POINTER(ctypes.c_uint32)
c_int32 = ctypes.c_int32
c_int32_p = ctypes.POINTER(ctypes.c_int32)
c_int = ctypes.c_int
c_uint = ctypes.c_uint
c_int64 = ctypes.c_int64
c_uint64 = ctypes.c_uint64
c_char_p = ctypes.c_char_p
c_void_p = ctypes.c_void_p
c_float = ctypes.c_float
c_double = ctypes.c_double
libsyncs_callback_type = ctypes.CFUNCTYPE(None, c_void_p, ctypes.POINTER(ctypes.c_char * 32), c_void_p, c_uint32)
libsyncsEventData = (c_uint8 * 1200)
libsyncsID = (c_uint8 * 32)

class LibsyncsChannelInfo(ctypes.Structure):
    _pack_ = 1  # no padding
    _fields_ = [
        ("id",libsyncsID),
        ("anons_count", c_uint32),
        ("request_count", c_uint32),
        ("ip", c_uint32),
        ("port", c_uint16),
    ]

class LibsyncsEventInfo(ctypes.Structure):
    _pack_ = 1  # no padding
    _fields_ = [
        ("id", libsyncsID),
        ("type", c_uint32),
        ("short_data", c_uint8 * 32),
        ("data_size", c_uint16),
        ("time", c_uint64),
        ("count", c_uint32),
        ("consumers_count", c_uint32),
        ("producers_count", c_uint32),
    ]


class LibsyncsClientInfo(ctypes.Structure):
    _pack_ = 1  # no padding
    _fields_ = [
        ("id", libsyncsID),
        ("event_subscribe", c_uint32),
        ("event_write", c_uint32),
        ("rx_event_count", c_uint32),
        ("tx_event_count", c_uint32),
        ("ip", c_uint32),
    ]

libsyncs.syncs_connect.restype = c_void_p
libsyncs.syncs_connect.argtypes = [
    c_char_p,
    c_int,
    c_char_p,
    libsyncs_callback_type,
    c_void_p,
]

libsyncs.syncs_connect_simple.restype = c_void_p
libsyncs.syncs_connect_simple.argtypes = [
    c_char_p,
    c_int,
    c_char_p,
]

libsyncs.syncs_udpconnect.restype = c_void_p
libsyncs.syncs_udpconnect.argtypes = [
    c_char_p,
    c_int,
    c_char_p
]

libsyncs.syncs_isconnect.restype = c_int
libsyncs.syncs_isconnect.argtypes = [
    c_void_p,
]

libsyncs.syncs_disconnect.restype = None
libsyncs.syncs_disconnect.argtypes = [
    c_void_p,
]


libsyncs.syncs_connect_wait.restype = c_int
libsyncs.syncs_connect_wait.argtypes = [
    c_void_p,
]

libsyncs.syncs_connect_status.restype = c_int
libsyncs.syncs_connect_status.argtypes = [
    c_void_p,
    c_char_p,
]

libsyncs.syncs_subscribe_event.restype = c_int
libsyncs.syncs_subscribe_event.argtypes = [
    c_void_p,
    c_uint32,
    c_char_p,
    libsyncs_callback_type,
    c_void_p,
]

libsyncs.syncs_subscribe_event_sync.restype = c_int
libsyncs.syncs_subscribe_event_sync.argtypes = [
    c_void_p,
    c_uint32,
    c_char_p,
]

libsyncs.syncs_unsubscribe_event.restype = c_int
libsyncs.syncs_unsubscribe_event.argtypes = [
    c_void_p,
    c_char_p,
]

libsyncs.syncs_request_channelslist.restype = ctypes.POINTER(LibsyncsChannelInfo)
libsyncs.syncs_request_channelslist.argtypes = [
    c_void_p,
    c_uint32_p,
    c_uint,
]
libsyncs.syncs_request_eventslist.restype = ctypes.POINTER(LibsyncsEventInfo)
libsyncs.syncs_request_eventslist.argtypes = [
    c_void_p,
    c_uint32_p,
    c_uint,
]
libsyncs.syncs_request_clientslist.restype = ctypes.POINTER(LibsyncsClientInfo)
libsyncs.syncs_request_clientslist.argtypes = [
    c_void_p,
    c_uint32_p,
    c_uint,
]

libsyncs.syncs_write.argtypes = [
    c_void_p,
    c_int,
    c_char_p,
    c_void_p,
    c_uint32,
]
libsyncs.syncs_write.restype = c_int

libsyncs.syncs_write_int32.argtypes = [
    c_void_p,
    c_int,
    c_char_p,
    c_int32,
]
libsyncs.syncs_write_int32.restype = c_int

libsyncs.syncs_write_int64.argtypes = [
    c_void_p,
    c_int,
    c_char_p,
    c_int64,
]
libsyncs.syncs_write_int64.restype = c_int

libsyncs.syncs_write_float.argtypes = [
    c_void_p,
    c_int,
    c_char_p,
    c_float,
]
libsyncs.syncs_write_float.restype = c_int

libsyncs.syncs_write_double.argtypes = [
    c_void_p,
    c_int,
    c_char_p,
    c_double,
]
libsyncs.syncs_write_double.restype = c_int

libsyncs.syncs_write_str.argtypes = [
    c_void_p,
    c_int,
    c_char_p,
    c_char_p,
]
libsyncs.syncs_write_str.restype = c_int

libsyncs.syncs_write_event.argtypes = [c_void_p, c_int, c_char_p]
libsyncs.syncs_write_event.restype = c_int


libsyncs.syncs_wait_event.argtypes = [
    c_void_p,
    c_uint32_p,
    ctypes.POINTER(libsyncsEventData),
    c_uint32_p,
    c_int,
]
libsyncs.syncs_wait_event.restype = c_char_p


libsyncs.syncs_read.argtypes = [
    c_void_p,
    c_int,
    c_char_p,
    c_void_p,
    c_uint32_p,
]
libsyncs.syncs_read.restype = c_int

libsyncs.syncs_read_int32.argtypes = [
    c_void_p,
    c_int,
    c_char_p,
    ctypes.POINTER(c_int32),
]
libsyncs.syncs_read_int32.restype = c_int

libsyncs.syncs_read_int64.argtypes = [
    c_void_p,
    c_int,
    c_char_p,
    ctypes.POINTER(c_int64),
]
libsyncs.syncs_read_int64.restype = c_int

libsyncs.syncs_read_float.argtypes = [
    c_void_p,
    c_int,
    c_char_p,
    ctypes.POINTER(c_float),
]
libsyncs.syncs_read_float.restype = c_int

libsyncs.syncs_read_double.argtypes = [
    c_void_p,
    c_int,
    c_char_p,
    ctypes.POINTER(c_double),
]
libsyncs.syncs_read_double.restype = c_int

libsyncs.syncs_read_str.argtypes = [
    c_void_p,
    c_int,
    c_char_p,
    c_char_p,
    c_int,
]
libsyncs.syncs_read_str.restype = c_int

event_types = {
    SYNCS_TYPE_VAR_INT32: "int",
    SYNCS_TYPE_VAR_STRING: "string",
    SYNCS_TYPE_VAR_INT64: "int64",
    SYNCS_TYPE_VAR_FLOAT: "float",
    SYNCS_TYPE_VAR_EMPTY: "empty",
    SYNCS_TYPE_VAR_DOUBLE: "double",
    SYNCS_TYPE_VAR_STRUCTURE: "struct",
}

event_types_consts = {v: k for k, v in event_types.items()}

event_types_format = {
    SYNCS_TYPE_VAR_INT32: "<i",
    SYNCS_TYPE_VAR_INT64: "<q",
    SYNCS_TYPE_VAR_FLOAT: "<f",
    SYNCS_TYPE_VAR_DOUBLE: "<d",
}

class syncscribe(object):
    flagEcho = SYNCS_TYPE_ECHO;

    def stringToEvent(string):
        return event_types_consts.get(string, 0)

    def eventToString(value):
        return str(event_types.get(value, "unknown"))

    def shortdataToString(data, eventType, size):
        eventType = eventType & SYNCS_TYPE_VAR_MASK
        if eventType == SYNCS_TYPE_VAR_STRING:
            return bytes(data[:size]).decode('utf-8')
        elif eventType == SYNCS_TYPE_VAR_STRUCTURE:
            return data.hex()
        elif eventType == 0:
            return ''
        else:
            unpacked_data = struct.unpack(event_types_format.get(eventType, "<s"), bytes(data[:4]))[0]
            return str(unpacked_data)

    def __init__(self, id, addr='127.0.0.1', port=4444, type='tcp'):
        if type == "udp":
            self.desc = libsyncs.syncs_udpconnect(
                addr.encode("utf-8"),
                c_int(port),
                id.encode("utf-8"),
            )
        else:
            self.desc = libsyncs.syncs_connect_simple(
                addr.encode("utf-8"), c_int(port), id.encode("utf-8")
            )

    # def __del__(self):
    #     libsyncs.syncs_disconnect(self.desc)
    #     self.desc = ctypes.POINTER(0)

    def subscribe_event(self, type, id, cb, args):
        return libsyncs.syncs_subscribe_event(
            self.desc,
            syncscribe.stringToEvent(type),
            id.encode("utf-8"),
            libsyncs_callback_type(cb),
            args
        )

    def subscribe_event_sync(self, type, id):
        return libsyncs.syncs_subscribe_event_sync(
            self.desc,
            syncscribe.stringToEvent(type),
            id.encode("utf-8"),
        )

    def unsubscribe_event(self, id):
        return libsyncs.syncs_unsubscribe_event(self.desc, c_char_p(id))

    def connect_wait(self, timeout=0):
        return libsyncs.syncs_connect_wait(self.desc, c_uint32(timeout))

    def find_server(self):
        port = c_int()
        addr = ctypes.create_string_buffer(256)
        libsyncs.syncs_find_server(addr, ctypes.byref(port))
        return addr, port

    def isconnect(self):
        return libsyncs.syncs_isconnect(self.desc)

    def connect_status(self):
        server_id = ctypes.create_string_buffer(32)
        result = libsyncs.syncs_connect_status(self.desc, server_id)
        return result, server_id.value.decode("utf-8").replace('\x00', '')

    def define(self, id, type):
        return syncscribe.syncs_define(self.desc, c_uint32(self.stringToEvent(type)), c_char_p(id))

    def undefine(self, id):
        return syncscribe.syncs_undefine(self.desc, c_char_p(id))

    def write(self, flags, id, data, data_size):
        return libsyncs.syncs_write(self.desc, flags, id.encode("utf-8"), data, data_size)

    def write_int32(self, flags, id, data):
        return libsyncs.syncs_write_int32(self.desc, flags, id.encode("utf-8"), data)

    def write_int64(self, flags, id, data):
        return libsyncs.syncs_write_int64(self.desc, flags, id.encode("utf-8"), data)

    def write_float(self, flags, id, data):
        return libsyncs.syncs_write_float(self.desc, flags, id.encode("utf-8"), data)

    def write_double(self, flags, id, data):
        return libsyncs.syncs_write_double(self.desc, flags, id.encode("utf-8"), data)

    def write_str(self, flags, id, data):
        return libsyncs.syncs_write_str(
            self.desc, flags, id.encode("utf-8"), data.encode("utf-8")
        )

    def write_event(self, flags, id):
        return libsyncs.syncs_write_event(self.desc, flags, id.encode("utf-8"))

    def wait_event(self):
        flags = c_uint32()
        data = libsyncsEventData()
        data_size = c_uint32(ctypes.sizeof(data))

        cid = libsyncs.syncs_wait_event(self.desc, ctypes.byref(flags), ctypes.byref(data), ctypes.byref(data_size), 1)
        if not cid :
            return (0, 0, 0, 0)
        return (cid.decode('utf-8').replace('\x00', ''), flags.value, data, data_size.value)

    def wait_event_str(self):
        id, flags, data, size = self.wait_event()
        return (id, syncscribe.shortdataToString(data,flags,size), size)

    def read(self, flags, id, buffer, buffer_size):
        data_size = ctypes.c_int(buffer_size)
        result = libsyncs.syncs_read(
            self.desc, flags, id.encode("utf-8"), buffer, ctypes.byref(data_size)
        )
        return result, data_size.value

    def read_int32(self, flags, id):
        data = c_int32()
        result = libsyncs.syncs_read_int32(
            self.desc, flags, id.encode("utf-8"), ctypes.byref(data)
        )
        return result, data.value

    def read_int64(self, flags, id):
        data = c_int64()
        result = libsyncs.syncs_read_int64(
            self.desc, flags, id.encode("utf-8"), ctypes.byref(data)
        )
        return result, data.value

    def read_float(self, flags, id):
        data = ctypes.c_float()
        result = libsyncs.syncs_read_float(
            self.desc, flags, id.encode("utf-8"), ctypes.byref(data)
        )
        return result, data.value

    def read_double(self, flags, id):
        data = ctypes.c_double()
        result = libsyncs.syncs_read_double(
            self.desc, flags, id.encode("utf-8"), ctypes.byref(data)
        )
        return result, data.value

    def read_str(self, flags, id, size):
        data = ctypes.create_string_buffer(size)
        result = libsyncs.syncs_read_str(self.desc, flags, id.encode("utf-8"), data, size)
        return result, data.value.decode("utf-8")

    def request_channels_list(self, timeout):
        count = c_uint32()

        result = libsyncs.syncs_request_channelslist(
            self.desc, ctypes.byref(count), timeout
        )
        channels = []
        for i in range(count.value):
            channel = result[i]
            channels.append(
                {
                    "id": str(bytes(channel.id).decode("utf-8")).replace('\x00', ''),
                    "anons_count": channel.anons_count,
                    "request_count": channel.request_count,
                    "ip": inet_ntoa(
                        struct.pack("!I", htonl(channel.ip))
                    ),  # Convert uint32 IP to string
                    "port": channel.port,
                }
            )
        return channels

    def request_events_list(self, timeout):
        count = c_uint32()

        result = libsyncs.syncs_request_eventslist(self.desc, ctypes.byref(count), timeout)
        events = []
        for i in range(count.value):
            event = result[i]
            events.append(
                {
                    "id": str(bytes(event.id).decode("utf-8")).replace('\x00', ''),
                    "type": syncscribe.eventToString(event.type),
                    "short_data": syncscribe.shortdataToString (event.short_data, event.type, event.data_size),
                    "data_size": event.data_size,
                    "count": event.count,
                    "consumers_count": event.consumers_count,
                    "producers_count": event.producers_count,
                }

            )
        return events

    def request_clients_list(self, timeout):
        count = c_uint32()
        result = libsyncs.syncs_request_clientslist(self.desc, ctypes.byref(count), timeout)
        clients = []
        for i in range(count.value):
            client_info = result[i]
            clients.append(
                {
                    "id": bytes(client_info.id).decode("utf-8").replace('\x00', ''),
                    "event_subscribe": client_info.event_subscribe,
                    "event_write": client_info.event_write,
                    "rx_event_count": client_info.rx_event_count,
                    "tx_event_count": client_info.tx_event_count,
                    "ip": (
                        inet_ntoa(struct.pack("!I", htonl(client_info.ip)))
                    ),  # Convert uint32 IP to string
                }
            )
        return clients
