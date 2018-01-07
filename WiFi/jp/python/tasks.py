import socket
from diagnostics import DiagnosticPacket
from commands import create_command_packet
from make_celery import make_celery
from app import app, packet_list
from celery import Celery


# Constant parameters
HTTP_PORT   = 8000
SERVER_PORT = 7000
CLIENT_PORT = 5000
SERVER_IP   = "localhost"
CLIENT_IP   = "192.168.1.250"

# Celery handle
# celery = make_celery(app)
celery = Celery(app.name, broker=app.config['CELERY_BROKER_URL'], backend=app.config['CELERY_RESULT_BACKEND'])
celery.conf.update(app.config)


@celery.task(bind=True)
def diagnostic_task(self):
    """
    """

    # Number of packets received
    count = 0

    self.update_state(state = "STANDBY",
                      meta  = {"status" : "Creating TCP server...",
                               "count"  : count})

    # Initialize server socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setblocking(0)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind((SERVER_IP, SERVER_PORT))
    server_socket.listen(10)
    self.update_state(state = "LISTENING",
                      meta  = {"status" : "Created, listening...",
                               "count"  : count})

    # Main loop
    while True:
        try:
            # Accept a connection and receive
            client, addr = server_socket.accept()
            if client:
                packet = client.recv(1024)
                # Create packet and add to packet list
                packet = DiagnosticPacket(packet)
                packet_list.push_back(packet)

                count += 1
                self.update_state(state = "LISTENING",
                                  meta  = {"status"  : "Listening...received packet",
                                           "count"   : count,
                                           "size"    : packet_list.get_size(),
                                           "message" : packet.payload()})
                client.close()

        except Exception as x:
            self.update_state(state = "LISTENING",
                              meta  = {"status"  : "Listening...exception",
                                       "count"   : count,
                                       "size"    : packet_list.get_size(),
                                       "message" : "None",
                                       "error"   : x})
            pass

    # Should never reach here
    self.update_state(state = "FAULTED",
                      meta  = {"status"  : "Faulted, and exiting",
                               "count"   : count,
                               "size"    : packet_list.get_size(),
                               "message" : "None"})


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