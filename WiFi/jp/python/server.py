import socket           # Sockets
import sys              # Flush
import logging          # Print debug
import json             # Writing data
import os.path          # Checking file path exists
import datetime         # Get current date + time
import pprint           # Pretty print
import time
import http.server
import socketserver
import threading

# Default address
PORT = 11111
IP   = "0.0.0.0"

# Set logging level
logging.basicConfig(level=logging.DEBUG, format='(%(threadName)s) %(message)s',)


""" Opens a UDP socket in server mode """
class UdpServer():

    def __init__(self, port=PORT, ip=IP):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.settimeout(1)
        self.port = port
        self.ip   = ip
        self.sock.bind((self.ip, self.port))
        # logging.debug("UDP Socket initialized.")
        # logging.debug("Listening on port: %i", self.port)
        # sys.stdout.flush()

    def listen(self):
        try:
            while True:
                data, addr = self.sock.recvfrom(4096)
                if data:
                    logging.debug("Packet: %s" % data)
                    sys.stdout.flush()
                else:
                    break
        except socket.timeout as e:
            pass
        except Exception as e:
            raise

    def close(self):
        logging.debug("Closing socket: %i", self.port)
        sys.stdout.flush()
        self.sock.close()


class UdpClient():

    def __init__(self, ip, port):
        self.ip   = ip
        self.port = port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setblocking(0)
        # self.sock.bind(("192.168.1.250", 11111))

    def send(self, message):
        print(self.sock.sendto(message, (self.ip, self.port)))

    def close(self):
        self.sock.close()


# Thread 1
def host_http_server():
    logging.debug("Creating HTTP Server...")
    handler = http.server.SimpleHTTPRequestHandler
    httpd   = socketserver.TCPServer(("", PORT), handler)
    # Start running
    logging.debug("Serving on port: %i", PORT)
    sys.stdout.flush()
    httpd.serve_forever()

# Thread 2
def diagnostic_port():
    logging.debug("Creating UDP port...")
    udp_server = UdpServer(6666, "192.168.1.250")
    while True:
        udp_server.listen()

# Thread 3
def command_port():
    udp_client = UdpClient("192.168.1.250", 5555)
    logging.debug("Created client")
    while True:
        logging.debug("Sending...")
        udp_client.send(str.encode("pingpong"))
        time.sleep(0.5)

def main():
    threads = [
        threading.Thread(target=host_http_server),
        threading.Thread(target=diagnostic_port),
        threading.Thread(target=command_port)
    ]

    for thread in threads:
        thread.dameon = True
        thread.start()

    # Keep main thread alive
    try:
        while threading.active_count() > 0:
            time.sleep(0.001)
    except KeyboardInterrupt:
        logging.debug("Received KeyboardInterrupt...")
        logging.debug("Closing all threads.")
        sys.exit()

if __name__ == "__main__":
    main()