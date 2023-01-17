import sys
import os
import paho.mqtt.client as mqtt
import string
import datetime
import time
# import kivy module
import kivy
import logging
kivy.require("1.9.1")
import kivy.uix.button as kb
from kivy.app import App
from kivy.uix.widget import Widget
from kivy.clock import mainthread
import struct


# keeps track of when we last turned the light on
onStartTime = 0

##############################

# Create and set up the logging subsystem
logger = None

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)

# create a file handler
timeFormat = "%a %b %d %Y %H.%M.%S"
today = datetime.datetime.today()
timestamp = today.strftime(timeFormat)
logFile = r'logs/logs' + timestamp + '.log'

handler = logging.FileHandler(logFile)
handler.setLevel(logging.INFO)

# create a logging format
formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
handler.setFormatter(formatter)

# add the handlers to the logger
logger.addHandler(handler)


class Mqtt_service(App):

    def __init__(self, **kwargs):
        # create our MQTT client
        self.mqttc = mqtt.Client()
        self.mqtt_up()


    def on_message(self, mqttc, obj, msg):
        # define our global vars for logger and the start time tracker
        global onStartTime
        global logger

        # get the local time in an easy to read format
        localtime = time.asctime(time.localtime(time.time()))

        # print the message topic and payload for debugging

        # check to see that the topic is our light1confirm
        # - not needed in this example because we are only subscribed to 1 topic as it is
        # - but I prefer to play it safe
        if msg.topic == "/house/light1confirm":
            print(f"{msg.topic} - {msg.payload.decode('UTF-8')}")
            # to do if the message said that we turned the light On
            if msg.payload.decode('UTF-8') == "On":
                # take note of when we turned the light on
                onStartTime = time.time()
                # log the light on time and print
                logMessage = "Light turned on at: " + localtime
                self.updateUI_light(msg.payload.decode('UTF-8'), logMessage)
                print(logMessage)
                logger.info(logMessage)

            # to do if the message said that we turned the light Off
            if msg.payload.decode('UTF-8') == "Off":
                # take note of the total run time
                runTime = time.time() - onStartTime

                # log & print when the light turned off
                logMessage = "Light turned off at: " + localtime
                self.updateUI_light(msg.payload.decode('UTF-8'), logMessage)
                logger.info(logMessage)

                # log & print the total time the light was on for
                logMessage = "The light was on for a total of " + str(int(runTime)) + " seconds"
                self.updateUI_light(msg.payload.decode('UTF-8'), logMessage)
                logger.info(logMessage)

        if msg.topic == "/house/temp_sensor":
            temp = float(int.from_bytes(msg.payload, byteorder='big'))
            print(temp)
            self.updateUI_temp(temp)

        # temp = self.bin_to_float(msg.payload.decode('UTF-8'))
        #  print(str(msg.payload.decode("utf-8")))
        #  print(f"{msg.topic} - {struct.unpack('f', msg.payload.decode())}")
        #   self.updateUI_temp(msg.payload.decode('UTF-8'))
        #     temp = chr(msg.payload)
        #  print(struct.unpack('d', msg.payload))

        if msg.topic == "/house/boiler1confirm":
            print(f"{msg.topic} - {msg.payload.decode('UTF-8')}")
            self.updateUI_boiler(msg.payload.decode('UTF-8'))
            print(msg.payload.decode('UTF-8'))

    def mqtt_up(self):
        # tell it what to do when we recieve a message,bind callback function
        self.mqttc.on_message = self.on_message
        # self.mqttc.on_connect = self.on_connect
#        self.mqttc.on_disconnect = self.on_disconnect
        # connect to the broker
        self.mqttc.connect("91.121.93.94", 1883, 60)
        # start the MQTT client loop in a separate thread
        self.mqttc.loop_start()
        # subscribe to our topics
        self.mqttc.subscribe("/house/light1confirm", 0)
        self.mqttc.subscribe("/house/temp_sensor", 0)
        self.mqttc.subscribe("/house/boiler1confirm", 0)

        self.update_request()

    def update_request(self):
        msg = "update"
        self.mqttc.publish("/house/light1", msg.encode())
        self.mqttc.publish("/house/boiler", msg.encode())

    def publishMsg_light(self, msg):
        self.mqttc.publish("/house/light1", msg.encode())
        print("ok")

    def publishMsg_boiler(self, num, msg):
        self.mqttc.publish("/house/boiler", num.encode())

    def updateUI_light(self, status, msg):
        pass

    def updateUI_boiler(self, status):
        pass

    def updateUI_temp(self, temp):
        pass

