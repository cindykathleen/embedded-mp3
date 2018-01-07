from app import app
from flask import render_template, jsonify, url_for, request
# import tasks

import socket
from diagnostics import DiagnosticPacket
from commands import create_command_packet
from make_celery import make_celery
from app import app, packet_list
from celery import Celery


CLIENT_PORT = 5000
CLIENT_IP   = "192.168.1.250"

celery = Celery(app.name, broker=app.config['CELERY_BROKER_URL'], backend=app.config['CELERY_RESULT_BACKEND'])
celery.conf.update(app.config)

@celery.task(bind=True)
def command_task(self):
    """
    """
    print("STARTING")
    self.update_state(state = "STANDBY",
                      meta  = {"status" : "Creating command socket..."})
    # Create socket
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.update_state(state = "PROGRESS",
                      meta  = {"status" : "Created, connecting to server..."})

    # Connect to server, ignore errors that don't need to be handled
    while True:
        try:
            client_socket.connect((CLIENT_IP, CLIENT_PORT))
            break
        except ConnectionRefusedError:
            pass
        except TimeoutError:
            pass
    self.update_state(state = "PROGRESS",
                      meta  = {"status" : "Connected, sending packet..."})

    # Send a packet
    packet = create_command_packet("PACKET_TYPE_COMMAND_READ", "PACKET_OPCODE_GET_STATUS", 0, 0)
    client_socket.send(str.encode(packet))
    client_socket.close()
    self.update_state(state = "DONE",
                      meta  = {"status" : "Sent packet"})

    return {"status" : "Sent packet"}


# # Create diagnostic task at startup
# @app.before_first_request
# def startup():
#     task = tasks.diagnostic_task.apply_async()
#     return jsonify({}), 202, { "Location" : url_for("diagnostic_task_status", task_id=task.id) }


# # Callback for updating the diagnostic task status
# @app.route('/diagnostic_status/<task_id>')
# def diagnostic_task_status(task_id):
#     task = tasks.diagnostic_task.AsyncResult(task_id)
#     return jsonify(task.info)


# Callback for a button press
@app.route("/play", methods=["POST"])
def send_command():
    task = command_task.apply_async()
    return jsonify({}), 202, { "Location" : url_for("command_task_status", task_id=task.id) }


# Callback for updating the command task status
@app.route('/command_status/<task_id>')
def command_task_status(task_id):
    task = command_task.AsyncResult(task_id)
    print(task.state)
    response = {
        # "status" : task.info.get("status", "")
    }
    return jsonify(response)


@app.route('/', methods=['GET', 'POST'])
def index():
    if request.method == 'GET':
        return render_template('index.html')
    return redirect(url_for('index'))


if __name__ == '__main__':
    app.run(debug=True)
