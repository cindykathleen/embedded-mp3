from app import app, red
from flask import render_template, jsonify, url_for, request
import atexit
import signal
import sys
from celery.task.control import revoke
import tasks


# Store task ID to be able to terminate it
diagnostic_task_id = None

# Create diagnostic task at startup
@app.before_first_request
def startup():
    global diagnostic_task_id
    # Sanity check, do not make a second task
    if diagnostic_task_id == None:
        task = tasks.diagnostic_task.apply_async()
        diagnostic_task_id = task.id


# Callback for updating the diagnostic task status
@app.route('/diagnostic_status')
def diagnostic_task_status():
    print("-"*50)
    print(diagnostic_task_id)
    print("-"*50)
    task = tasks.diagnostic_task.AsyncResult(diagnostic_task_id)
    response = {
        "status"  : task.info.get("status"),
        "count"   : task.info.get("count"),
        "message" : task.info.get("message"),
        "test"    : red.get("count").decode(),
        "error"   : task.info.get("error"),
    }
    print(response)
    return jsonify(response)


# Callback for a button press
@app.route("/command/<button>", methods=["POST"])
def send_command(button):
    task = tasks.command_task.apply_async([button])
    return jsonify({}), 202, { "Location" : url_for("command_task_status", task_id=task.id) }


# Callback for updating the command task status
@app.route('/command_status/<task_id>')
def command_task_status(task_id):
    task = tasks.command_task.AsyncResult(task_id)
    print(task.state)
    print(task.info.get("status"))
    response = {
        "state"  : task.state,
        "status" : task.info.get("status")
    }
    return jsonify(response)


# Home page
@app.route('/', methods=['GET', 'POST'])
def index():
    if request.method == 'GET':
        return render_template('index.html')
    elif request.method == 'POST':
        return redirect(url_for('index'))


# Close diagnostic task when server terminates
def signal_handler(signal, frame):
    print("Exiting diagnostic task...")
    revoke(diagnostic_task_id, terminate=True)
    sys.exit(0)


# Main program
if __name__ == '__main__':
    signal.signal(signal.SIGINT, signal_handler)
    app.run(debug=True)
