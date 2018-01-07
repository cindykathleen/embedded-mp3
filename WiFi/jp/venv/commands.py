
"""
    @name    : commands
    @purpose : API to create command packets to be sent over a socket
"""

# Type values for command packets
packet_type_E = {
    "PACKET_TYPE_INFO"          : 0,  # System starting/finishing/initializing something, x bytes were sent
    "PACKET_TYPE_ERROR"         : 1,  # Something failed
    "PACKET_TYPE_STATUS"        : 2,  # Something successful, something complete
    "PACKET_TYPE_COMMAND_READ"  : 3,  # Read commands to the decoder
    "PACKET_TYPE_COMMAND_WRITE" : 4,  # Write commands to the decoder
}

# Write opcode values for command packets
packet_opcode_write_E = {
    "PACKET_OPCODE_SET_BASS"         : 0 ,
    "PACKET_OPCODE_SET_TREBLE"       : 1 ,
    "PACKET_OPCODE_SET_SAMPLE_RATE"  : 2 ,
    "PACKET_OPCODE_SET_PLAY_CURRENT" : 3 ,
    "PACKET_OPCODE_SET_PLAY_NEXT"    : 4 ,
    "PACKET_OPCODE_SET_PLAY_PREV"    : 5 ,
    "PACKET_OPCODE_SET_STOP"         : 6 ,
    "PACKET_OPCODE_SET_FAST_FORWARD" : 7 ,
    "PACKET_OPCODE_SET_REVERSE"      : 8 ,
    "PACKET_OPCODE_SET_PAUSE"        : 9 ,
}

# Read opcode values for command packets
packet_opcode_read_E = {
    "PACKET_OPCODE_SET_SHUFFLE"      : 30, # TODO : Update this, or make it in a common folder
    "PACKET_OPCODE_GET_STATUS"       : 31,
    "PACKET_OPCODE_GET_SAMPLE_RATE"  : 32,
    "PACKET_OPCODE_GET_DECODE_TIME"  : 33,
    "PACKET_OPCODE_GET_HEADER_INFO"  : 34,
    "PACKET_OPCODE_GET_BIT_RATE"     : 35,
    "PACKET_OPCODE_SET_RESET"        : 36,
    "PACKET_OPCODE_LAST_INVALID"     : 37,
}

def create_command_packet(opcode, value1, value2):

    """
        @description  : Joins four values into a 4-byte command packet
        @param opcode : Opcode (string) of command, used to lookup value of opcode and value of type
        @param value1 : Command value / byte 1
        @param value2 : Command value / byte 2
        @returns      : Returns the command packet as a string
    """

    packet_type   = None
    packet_opcode = None

    # Write command
    if opcode in packet_opcode_write_E:
        packet_type   = packet_type_E["PACKET_TYPE_COMMAND_WRITE"]
        packet_opcode = packet_opcode_write_E[opcode]
    # Read command
    elif opcode in packet_opcode_read_E:
        packet_type   = packet_type_E["PACKET_TYPE_COMMAND_READ"]
        packet_opcode = packet_opcode_read_E[opcode]
    # Opcode not found
    else:
        raise Exception("Opcode {} not valid".format(opcode))

    return "".join(map(str, [packet_type, packet_opcode, value1, value2]))