#!/usr/bin/env python3

from flask import Flask, jsonify, render_template, request, redirect, url_for, current_app
import threading
import time
from syncscribe import syncscribe
import ctypes


app = Flask(__name__)

current_connection = None
event_list_data = []
events_data = []
lock = threading.Lock()
reconnect_lock = threading.Lock()


def events_thread():
    global events_data
    while True:
        time.sleep(1);
        with reconnect_lock:
            while True:
                with lock:
                    if not current_connection:
                        break
                id, data, size = current_connection.wait_event_str()
                if id == 0:
                    continue
                for event in events_data:
                    if event['id'] == id:
                        event['data'] =  data
                        event['data_size'] = size
                        break

event_thread = threading.Thread(target=events_thread)
event_thread.daemon = True
event_thread.start()


def events_list_update_thread():
    global event_list_data
    with lock:
       if current_connection:
           event_list_data = current_connection.request_events_list(1000)

def print_chars_and_codes(s):
    for char in s:
        print(f"'{char}' - {ord(char)}")


@app.route('/', methods=['GET', 'POST'])
def index():
    if request.method == 'POST':
        return redirect(url_for('index'))
    return render_template('events.html')

@app.route('/connect', methods=['POST'])
def connect():
    global current_connection
    data = request.get_json()
    id = data['id']
    addr = data['ip']
    port = int(data['port'])
    protocol = data['protocol']
    with lock:
         current_connection = syncscribe(id, addr, port, protocol)

    return jsonify({"success": True, "message": "Client looking for server"})

@app.route('/events_list')
def events_list():
    with lock:
        return {'events_list': event_list_data}

@app.route('/update_events_list', methods=['POST'])
def update_events():
    thread = threading.Thread(target=events_list_update_thread)
    thread.daemon = True
    thread.start()
    return jsonify({"success": True, "message": "Updating events list"})


@app.route('/events')
def events():
    return {'events': events_data}


@app.route('/server_header')
def server_header():
    header_content = "SyncScribe network monitor"
    with lock:
       if current_connection:
            connection_status = current_connection.connect_status()
            if connection_status[0]:
                header_content = "SyncScribe network monitor: not connected"
            else:
                header_content = connection_status[1]
    return jsonify(header=header_content)


@app.route('/subscribe_event', methods=['POST'])
def subscribe_event():
    data = request.get_json()
    event_id = data['eid']
    event_type = data['etype']
    with lock:
        current_connection.subscribe_event_sync(event_type, event_id)
    events_data.append (
                {
                    "id": event_id,
                    "type": event_type,
                    "data": "",
                    "data_size": 0,
                }
    )

    return jsonify({"success": True, "message": "Subscribed to event successfully."})


@app.route('/set_event', methods=['POST'])
def set_event():
    data = request.get_json()
    event_id = data['eid']
    event_type = data['etype']
    event_size = data['esize']
    new_data = data['data']
    new_type = data['type']
    status = -1

    if event_type == "int":
        if new_type == "hex":
            value = int(new_data.replace(" ", ""), 16)
        elif new_type == "value":
            value = int(new_data)
        status = current_connection.write_int32(syncscribe.flagEcho , event_id, value)
    elif event_type == "string":
        status = current_connection.write_str(syncscribe.flagEcho, event_id, new_data)
    elif event_type == "float":
        status = current_connection.write_float(syncscribe.flagEcho, event_id, float(new_data))
    elif event_type == "double":
        status = current_connection.write_double(syncscribe.flagEcho, event_id, float(new_data))
    elif event_type == "struct":
        binary_data = bytes(int(x, 16) for x in new_data.split())
        #TODO send binary

    if status == 0:
        return jsonify({"success": True, "message": "Event data updated successfully."})
    else:
        return jsonify({"success": False, "message": "Event data updated successfully failed."}), 400




if __name__ == '__main__':
    app.run(debug=False, host='0.0.0.0', port=5001)
