
"""
    @name    : commands
    @purpose : API to create command packets to be sent over a socket
"""

packet_type_E = {
    "PACKET_TYPE_INFO"          : 0,  # System starting/finishing/initializing something, x bytes were sent
    "PACKET_TYPE_ERROR"         : 1,  # Something failed
    "PACKET_TYPE_STATUS"        : 2,  # Something successful, something complete
    "PACKET_TYPE_COMMAND_READ"  : 3,  # Read commands to the decoder
    "PACKET_TYPE_COMMAND_WRITE" : 4,  # Write commands to the decoder
}

# Denotes the opcode for command packets
packet_opcode_E = {
    "PACKET_OPCODE_NONE"             : 0 ,
    "PACKET_OPCODE_SET_BASS"         : 1 ,
    "PACKET_OPCODE_SET_TREBLE"       : 2 ,
    "PACKET_OPCODE_SET_SAMPLE_RATE"  : 3 ,
    "PACKET_OPCODE_SET_PLAY_CURRENT" : 4 ,
    "PACKET_OPCODE_SET_PLAY_NEXT"    : 5 ,
    "PACKET_OPCODE_SET_PLAY_PREV"    : 6 ,
    "PACKET_OPCODE_SET_STOP"         : 7 ,
    "PACKET_OPCODE_SET_FAST_FORWARD" : 8 ,
    "PACKET_OPCODE_SET_REVERSE"      : 9 ,
    "PACKET_OPCODE_SET_SHUFFLE"      : 10,
    "PACKET_OPCODE_GET_STATUS"       : 11,
    "PACKET_OPCODE_GET_SAMPLE_RATE"  : 12,
    "PACKET_OPCODE_GET_DECODE_TIME"  : 13,
    "PACKET_OPCODE_GET_HEADER_INFO"  : 14,
    "PACKET_OPCODE_GET_BIT_RATE"     : 15,
    "PACKET_OPCODE_SET_RESET"        : 16,
    "PACKET_OPCODE_LAST_INVALID"     : 17,
}

def create_command_packet(packet_type, packet_opcode, value1, value2):

    if packet_type not in packet_type_E:
        raise Exception("Type {} not valid".format(packet_type))
    elif packet_opcode not in packet_opcode_E:
        raise Exception("Opcode {} not valid".format(packet_opcode))
    else:
        return str(packet_type_E[packet_type]) + str(packet_opcode_E[packet_opcode]) + str(value1) + str(value2)