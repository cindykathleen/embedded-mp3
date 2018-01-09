from flask import Flask
from diagnostics import DiagPacketStruct
import redis


# Flask handle
app = Flask(__name__)
app.config['CELERY_BROKER_URL']     = 'redis://localhost:6379/0'
app.config['CELERY_RESULT_BACKEND'] = 'redis://localhost:6379/0'

# Structure to hold recent diagnostic packets
packet_list = DiagPacketStruct(max=10)

# Redis handle
red = redis.StrictRedis(host="localhost", port=6379, db=0)