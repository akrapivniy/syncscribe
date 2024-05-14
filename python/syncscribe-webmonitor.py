#!/usr/bin/env python3

from flask import Flask, jsonify, render_template, request, redirect, url_for
import threading
import time
from syncscribe import syncscribe

app = Flask(__name__)

current_connection = None
channels_data = []
events_data = []
clients_data = []
lock = threading.Lock()

def fetch_data():
    global channels_data, events_data, clients_data
    while True:
        with lock:
            if current_connection:
                channels_data = current_connection.request_channels_list(1000)
                events_data = current_connection.request_events_list(1000)
                clients_data = current_connection.request_clients_list(1000)
        time.sleep(1)

thread = threading.Thread(target=fetch_data)
thread.daemon = True
thread.start()

@app.route('/', methods=['GET', 'POST'])
def index():
    if request.method == 'POST':
        global current_connection
        id = request.form['id']
        addr = request.form['ip']
        port = int(request.form['port'])
        protocol = request.form['protocol']
        with lock:
            current_connection = syncscribe(id, addr, port, protocol)
        return redirect(url_for('index'))
    return render_template('monitor.html')

@app.route('/channels')
def channels():
    with lock:
        return {'channels': channels_data}

@app.route('/clients')
def clients():
    with lock:
        return {'clients': clients_data}

@app.route('/events')
def events():
    with lock:
        return {'events': events_data}

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
        status = current_connection.write_int32(0, event_id, value)
    elif event_type == "string":
        status = current_connection.write_str(0, event_id, new_data)
    elif event_type == "float":
        status = current_connection.write_float(0, event_id, float(new_data))
    elif event_type == "double":
        status = current_connection.write_double(0, event_id, float(new_data))
    elif event_type == "struct":
        binary_data = bytes(int(x, 16) for x in new_data.split())
        #TODO send binary

    if status == 0:
        return jsonify({"success": True, "message": "Event data updated successfully."})
    else:
        return jsonify({"success": False, "message": "Event data updated successfully failed."}), 400

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=5001)
