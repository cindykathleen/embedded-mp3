from app import app
from flask import render_template, jsonify, url_for, request
import tasks


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
    task = tasks.command_task.apply_async()
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


@app.route('/', methods=['GET', 'POST'])
def index():
    if request.method == 'GET':
        return render_template('index.html')
    return redirect(url_for('index'))


if __name__ == '__main__':
    app.run(debug=True)
