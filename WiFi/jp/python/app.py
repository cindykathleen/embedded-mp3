from flask import Flask, render_template, jsonify

import socket

from commands import send_command_packet
from diagnostics import DiagnosticPacket, DiagPacketStruct
from make_celery import make_celery

# Flask handle
app = Flask(__name__)
app.config['CELERY_BROKER_URL']     = 'redis://localhost:6379/0'
app.config['CELERY_RESULT_BACKEND'] = 'redis://localhost:6379/0'

# Celery handle
celery = make_celery(app)


@app.route("/_play")
def play():
    return jsonify(result="next packet")

count = 0
@app.route("/_update")
def update():
    global count
    count += 1
    return jsonify(result="packet" + str(count))

packet_list = DiagPacketStruct()
@celery.task()
def diagnostic_task():
    # Initialize server socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((SERVER_IP, SERVER_PORT))
    server_socket.listen(10)
    print("Created TCP server")

    # Main loop
    while True:
        if PENDING_EXIT:
            return

        # Accept a connection and receive
        client, addr = server_socket.accept()
        packet = client.recv(1024)
        packet = DiagnosticPacket(packet)
        packet_list.push_back(packet)
        packet.print()
        client.close()


@app.route("/")
def index():
    diagnostic_task.delay()
    return render_template("index.html")


if __name__ == '__main__':
    app.run()
