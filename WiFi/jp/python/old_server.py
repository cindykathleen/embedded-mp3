import socket           # Sockets
import sys              # Flush
import logging          # Print debug
import time
import http.server
import threading
import os

from commands import create_command_packet
from diagnostics import DiagnosticPacket

# Constant parameters
HTTP_PORT   = 8000
SERVER_PORT = 7000
CLIENT_PORT = 5000
IP          = "localhost"
SERVER_IP   = "192.168.5.227"
CLIENT_IP   = "192.168.1.250"

# Set logging level
logging.basicConfig(level=logging.DEBUG, format='(%(threadName)s) %(message)s',)

# Global variable
PENDING_EXIT = False

# Thread 1
def host_http_server():
    logging.debug("Creating HTTP Server...")
    handler = http.server.SimpleHTTPRequestHandler
    httpd   = socketserver.TCPServer(("localhost", HTTP_PORT), handler)
    # Start running
    logging.debug("Serving on port: %i", HTTP_PORT)
    sys.stdout.flush()
    httpd.serve_forever()


# Thread 2
def diagnostic_task():
    # Initialize server socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((SERVER_IP, SERVER_PORT))
    server_socket.listen(10)
    logging.debug("Created TCP server")

    # Main loop
    while True:
        if PENDING_EXIT:
            return

        # Accept a connection and receive
        client, addr = server_socket.accept()
        packet = client.recv(1024)
        packet = DiagnosticPacket(packet)
        print(packet.get_payload())
        client.close()


# Thread 3
def command_task():

    global PENDING_EXIT

    # Main loop
    while True:
        if PENDING_EXIT:
            return

        # Create socket
        client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        # Connect to server, ignore errors that don't need to be handled
        while True:
            try:
                client_socket.connect((CLIENT_IP, CLIENT_PORT))
                break
            except ConnectionRefusedError:
                pass
            except TimeoutError:
                pass

        # Send a packet
        packet = create_command_packet("PACKET_TYPE_COMMAND_READ", "PACKET_OPCODE_GET_STATUS", 0, 0)
        print(str.encode(packet))
        client_socket.send(str.encode(packet))
        client_socket.close()
        time.sleep(1)


# Main thread
def main():

    global PENDING_EXIT

    threads = [
        # threading.Thread(target=host_http_server),
        threading.Thread(target=diagnostic_task),
        # threading.Thread(target=command_task)
    ]

    for thread in threads:
        thread.dameon = True
        thread.start()

    # Keep main thread alive
    try:
        while threading.active_count() > 0:
            time.sleep(0.001)
    except KeyboardInterrupt:
        PENDING_EXIT = True
        logging.debug("Received KeyboardInterrupt...")
        logging.debug("Closing all threads.")
        os._exit(0)


if __name__ == "__main__":
    main()
