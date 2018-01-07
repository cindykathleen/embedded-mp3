from collections import deque

"""
    @name    : diagnostics
    @purpose : API to parse diagnostic packets
"""


class DiagnosticPacket():

    def __init__(self, buffer):
        self.type    = buffer[0]
        self.length  = buffer[1]
        self.payload = buffer[2:]

    def type(self):
        return self.type

    def length(self):
        return self.length

    def payload(self):
        return self.payload

    def print_packet(self):
        print("[{}] {}".format(self.type, self.payload))


class DiagPacketStruct():

    def __init__(self, max=100):
        self.max   = max
        self.size  = 0
        self.queue = deque()

    def push_back(self, packet):
        if type(packet) is not DiagnosticPacket:
            raise Exception("Input is not of type DiagnosticPacket")
        else:
            self.queue.append(packet)
            if self.size < self.max:
                self.size += 1
            # Get rid of oldest packets
            if self.size == self.max:
                self.queue.pop_left()

    def get_back(self):
        """ Get the last item in the queue """
        return list(self.queue)[:-1]

    def get_index(self, index):
        return list(self.queue)[index]

    def get_list(self):
        return list(self.queue)

    def get_size(self):
        return self.size