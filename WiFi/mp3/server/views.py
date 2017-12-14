"""

pip install django
run esp32 server with make monitor
run 'python manage.py runserver'

go to 127.0.0.1:8000

wait for esp32 to output ' socket accepted: 4096'
press any button, watch output on make monitor



Command packet 
-------------------------------------------
Header:
    8 bit opcode, 8 bit type
Command:
    16 bit half word/ 2 bytes
-------------------------------------------


Diagnostic packet
-------------------------------------------
Header:
    8 bit length, 8 bit type
Payload:
    MAX_PACKET_SIZE = 128
    8 bit int array[MAX_PACKET_SIZE]
-------------------------------------------


packet opcodes:
-------------------------------------------
    PACKET_OPCODE_NONE                = 0,
    PACKET_OPCODE_SET_BASS            = 1,
    PACKET_OPCODE_SET_TREBLE          = 2,
    PACKET_OPCODE_SET_SAMPLE_RATE     = 3,
    PACKET_OPCODE_SET_PLAY_CURRENT    = 4,
    PACKET_OPCODE_SET_PLAY_NEXT       = 5,
    PACKET_OPCODE_SET_PLAY_PREV       = 6,
    PACKET_OPCODE_SET_STOP            = 7,
    PACKET_OPCODE_SET_FAST_FORWARD    = 8,
    PACKET_OPCODE_SET_REVERSE         = 9,
    PACKET_OPCODE_SET_SHUFFLE         = 10,
    PACKET_OPCODE_GET_STATUS          = 11,
    PACKET_OPCODE_GET_SAMPLE_RATE     = 12,
    PACKET_OPCODE_GET_DECODE_TIME     = 13,
    PACKET_OPCODE_GET_HEADER_INFO     = 14,
    PACKET_OPCODE_GET_BIT_RATE        = 15,
    PACKET_OPCODE_SET_RESET           = 16,
    PACKET_OPCODE_LAST_INVALID        = 17,
-------------------------------------------


packet types:
----------------------------------------------------------------------------------------------------------
    PACKET_TYPE_INFO          = 0,  // System starting/finishing/initializing something, x bytes were sent
    PACKET_TYPE_ERROR         = 1,  // Something failed
    PACKET_TYPE_STATUS        = 2,  // Something successful, something complete
    PACKET_TYPE_COMMAND_READ  = 3,  // Read commands to the decoder
    PACKET_TYPE_COMMAND_WRITE = 4,  // Write commands to the decoder
----------------------------------------------------------------------------------------------------------

"""
from django.shortcuts import render, redirect
from django.conf.urls import url
from django.views.generic import ListView, TemplateView
import socket, time, os, random, ctypes, sys
from django.views import View
import threading
from celery import shared_task

        

class AboutView(TemplateView):
    template_name = "index.html"

        

class Server():
    # 192.168.43.194
    server = '192.168.43.194'
    port = 11000
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    con = s.connect((server,port))
    print(s)
    print(con)


    def send(self,action):

        if action is 'play':
            self.send_data(0x0404)

        elif action is 'pause':
            self.send_data(0x0704)

        elif action is 'previous':
            self.send_data(0x0604)

        elif action is 'next':
            self.send_data(0x0504)

        elif action is 'fastforward':
            self.send_data(0x0804)

        elif action is 'shuffle':
            self.send_data(0x0B04)

        elif action is 'volumeUp':
            self.send_data(0x0B04)

        elif action is 'volumeDown':
            self.send_data(0x0B04)
        return

    def send_data(self,hex_value):
        msg_value = (hex_value).to_bytes(2, byteorder='big')
        self.s.sendto(msg_value, (self.server,self.port))

    def close(self):
        self.s.close()


class Play(View):
    def get(self,request):
        s = Server()
        s.send('play')
        return redirect('home')

class Pause(View):
    def get(self,request):
        s = Server()
        s.send('pause')
        return redirect('home')

class Previous(View):
    def get(self,request):
        s = Server()
        s.send('previous')
        return redirect('home')

class Next(View):
    def get(self,request):
        s = Server()
        s.send('next')
        return redirect('home')

class Shuffle(View):
    def get(self,request):
        s = Server()
        s.send('shuffle')
        return redirect('home')

class Fastforward(View):
    def get(self,request):
        s = Server()
        s.send('fastforward')
        return redirect('home')

class volumeUp(View):
    def get(self,request):
        s = Server()
        s.send('volumeUp')
        return redirect('home')

class volumeDown(View):
    def get(self,request):
        s = Server()
        s.send('volumeDown')
        return redirect('home')
