
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

    def print(self):
        print("[{}] {}".format(self.type, self.payload))