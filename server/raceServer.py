from twisted.web.server import Site
from twisted.web.resource import Resource
from twisted.web.static import File
from twisted.internet import reactor
from twisted.python import log
from twisted.internet.task import LoopingCall
from autobahn.twisted.websocket import WebSocketServerProtocol, \
									   WebSocketServerFactory
from autobahn.twisted.resource import WebSocketResource
from socket import SOL_SOCKET, SO_BROADCAST
from xbee import XBee
import sys, cgi, time, json, serial, struct

class RaceWSServerProtocol(WebSocketServerProtocol): 
	def onOpen(self):
		self.factory.register(self)
		self.factory.sendClockMsg(self)
		self.factory.sendDeviceMsg(self)

	def connectionLost(self, reason):
		WebSocketServerProtocol.connectionLost(self, reason)
		self.factory.unregister(self)

	def onMessage(self, payload, isBinary):
		global timer
		doBroadcast = False
		cmd = json.loads(payload)
		if cmd['command'] == 'start':
			timer.start()
			doBroadcast = True
		elif cmd['command'] == 'stop':
			timer.stop()
			doBroadcast = True
		elif cmd['command'] == 'reset':
			timer.reset()
			doBroadcast = True
		elif cmd['command'] == 'refresh':
			self.factory.sendClockMsg(self)

		if doBroadcast:
			self.factory.sendClockMsg()


class RaceWSServerFactory(WebSocketServerFactory):
	protocol = RaceWSServerProtocol

	def __init__(self, url, debug = False, debugCodePaths = False):
		WebSocketServerFactory.__init__(self, url, debug = debug, debugCodePaths = debugCodePaths)
		self.clients = []

	def register(self, client):
		if not client in self.clients:
			self.clients.append(client)

	def unregister(self, client):
		if client in self.clients:
			self.clients.remove(client)

	def broadcast(self, msg, dest = None):
		if dest == None:
			preparedMsg = self.prepareMessage(msg)
			for c in self.clients:
				c.sendPreparedMessage(preparedMsg)
		else:
			dest.sendMessage(msg)			

	def sendClockMsg(self, dest = None):
		msg = {
			"command": "updateClock", 
			"running": timer.running,
			"time"   : timer.getCount()
		}
		self.broadcast(json.dumps(msg), dest)

	def sendDeviceMsg(self, dest = None):
		msg = {
			'command': 'updateDevices', 
			'timeNow': time.time(),
			'devices': xbNet.remoteUnits
		}
		self.broadcast(json.dumps(msg), dest)


class XBeeNetwork:
	remoteUnits = {}

	def __init__(self, port, rate):
		self.xbSerial = serial.Serial(port, rate)
		self.xbee = XBee(self.xbSerial, callback=self.msgReceived)
		# Initialize remote device states
		for name, dev in devConfig.iteritems():
			self.remoteUnits[name] = {
				'name'     	: dev['name'],
				'lastHeard'	: 0,
				'rssi'     	: 0,
				'batt'		: 0,
				'timeOff'	: 0,
			}

	def halt(self):
		self.xbee.halt()
		self.xbSerial.close()

	def msgReceived(self, data):
		global timer
		if data['id'] == 'rx':		# Message from a device
			srcAddr = data['source_addr'].encode('hex')
			(rssi,) = struct.unpack("<B", data['rssi'])
			
			if srcAddr not in self.remoteUnits:
				return
			self.remoteUnits[srcAddr]['lastHeard'] = time.time()
			self.remoteUnits[srcAddr]['rssi']      = rssi

			if data['rf_data'][0] == 'h':	# Heartbeat message
				(cmd, devTime, devBatt) = \
					struct.unpack("<cLH", data['rf_data'])

				# 3000 = 5V
				# 2000 = 3V
				devBatt = max(0, min(100, (devBatt - 2000) / 10))
				timeOff = time.time() - (devTime/1000.0)
				#if (srcAddr in self.remoteUnits):
				#	print("L %.4f R %.4f E %.4f" % (time.time(), devTime/1000.0, timeOff - self.remoteUnits[srcAddr]['timeOff']))

				self.remoteUnits[srcAddr]['batt']      = devBatt
				self.remoteUnits[srcAddr]['timeOff']   = timeOff

				wsServer.sendDeviceMsg()

			if data['rf_data'][0] == 'e':	# Event message
				(cmd, evtTime, evtType) = \
					struct.unpack("<cLc", data['rf_data'])

				myTime = time.time()
				evTime = evtTime/1000.0
				cpTime = myTime - self.remoteUnits[srcAddr]['timeOff'] - evTime
				#print("Event age: %.4f" % (time.time() - self.remoteUnits[srcAddr]['timeOff'] - (evtTime/1000.0)))
				print("Event %s myTime %.4f evTime %.4f cpTime %.4f" % (evtType, myTime, evTime, cpTime))

				action = None
				if evtType == 'd':
					action = devConfig[srcAddr]['onDown']
				elif evtType == 'u':
					action = devConfig[srcAddr]['onUp']

				if action == 'Start':
					timer.start()
				elif action == 'Stop':
					timer.stop()
				elif action == 'Reset':
					timer.reset()
				elif action == 'StopReset':
					timer.stop()
					timer.reset()

				wsServer.sendClockMsg()


class Timer:
	running  = False
	prevTime = 0.0		# Time counted up to last stop
	lastTime = 0.0		# I have been running since..

	def start(self):
		if not self.running:
			self.running = True
			self.lastTime = time.time()

	def stop(self):
		if self.running:
			self.running = False
			self.prevTime += time.time()-self.lastTime

	def reset(self):
		self.prevTime = 0.0
		self.lastTime = time.time()

	def getCount(self):
		if self.running:
			return self.prevTime+time.time()-self.lastTime
		else:
			return self.prevTime


if __name__ == '__main__':
	devConfig = {
		'0100': {
			'name' 	: 'Green button',
			'onDown': 'StopReset',
			'onUp'	: 'Start',
		},
		'0101': {
			'name'	: 'Red button',
			'onDown': 'Stop',
			'onUp'	: None,
		}
	}


	timer = Timer()
	xbNet = XBeeNetwork('/dev/ttyAMA0', 57600)

	log.startLogging(sys.stdout)

	wsServer = RaceWSServerFactory("ws://localhost:80", debug = False)

	root = File('static/')
	root.putChild('ws', WebSocketResource(wsServer))

	site = Site(root)
	reactor.listenTCP(80, site)

	reactor.run()

	# Reactor has returned, program is ending.
	xbNet.halt()
