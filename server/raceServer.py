from twisted.web.server import Site
from twisted.web.resource import Resource
from twisted.web.static import File
from twisted.internet import reactor
from twisted.python import log
from twisted.internet.protocol import DatagramProtocol
from twisted.internet.task import LoopingCall
from autobahn.twisted.websocket import WebSocketServerProtocol, \
									   WebSocketServerFactory
from autobahn.twisted.resource import WebSocketResource
from socket import SOL_SOCKET, SO_BROADCAST
from xbee import XBee
import sys, cgi, time, json, serial, struct

class BroadcastServerProtocol(WebSocketServerProtocol): 
	def onOpen(self):
		self.factory.register(self)

	def connectionLost(self, reason):
		WebSocketServerProtocol.connectionLost(self, reason)
		self.factory.unregister(self)

	def onMessage(self, payload, isBinary):
		global running, prevTime, lastTime
		doBroadcast = False
		cmd = json.loads(payload)
		if cmd['command'] == 'start':
			if not running:
				running = True
				lastTime = time.time()
				doBroadcast = True
		elif cmd['command'] == 'stop':
			if running:
				running = False
				prevTime += time.time()-lastTime
				doBroadcast = True
		elif cmd['command'] == 'reset':
			prevTime = 0.0
			lastTime = time.time()
			doBroadcast = True
		elif cmd['command'] == 'refresh':
			totalTime = (prevTime+time.time()-lastTime) if running else prevTime
			msg = {"command": "updateClock", "running":running,"time":totalTime}
			self.sendMessage(json.dumps(msg))

		if doBroadcast:
			totalTime = (prevTime+time.time()-lastTime) if running else prevTime
			msg = {"command": "updateClock", "running":running,"time":totalTime}
			self.factory.broadcast(json.dumps(msg))

class BroadcastServerFactory(WebSocketServerFactory):
	protocol = BroadcastServerProtocol

	def __init__(self, url, debug = False, debugCodePaths = False):
		WebSocketServerFactory.__init__(self, url, debug = debug, debugCodePaths = debugCodePaths)
		self.clients = []

	def register(self, client):
		if not client in self.clients:
			self.clients.append(client)

	def unregister(self, client):
		if client in self.clients:
			self.clients.remove(client)

	def broadcast(self, msg):
		preparedMsg = self.prepareMessage(msg)
		for c in self.clients:
			c.sendPreparedMessage(preparedMsg)


class EchoClientDatagramProtocol(DatagramProtocol):
	avgTheta = 0.0
	avgCount = 1

	def startProtocol(self):
		self.transport.socket.setsockopt(SOL_SOCKET, SO_BROADCAST, True)
	
	def broadcastPing(self):
		datagram = json.dumps({'command':'ping', 'time':time.time()})
		self.transport.write(datagram, ('10.42.0.255', 8000))

	def datagramReceived(self, datagram, host):
		#print 'Datagram received: ', repr(datagram)
		msg = json.loads(datagram)
		if msg['command'] == 'pong':
			recvTime = time.time()
			theta = msg['pongTime'] - (msg['pingTime']+recvTime) / 2
			weight = 1.0 / self.avgCount
			self.avgCount = min(self.avgCount+1, 100)
			self.avgTheta = self.avgTheta*(1-weight) + theta*weight
			print("Theta: %.4f" % self.avgTheta)

def fixedToFloat(fixed):
	return float(fixed) / 2**12	# 20.12 format
def floatToFixed(float):
	return int(float * 2**12)

class XBeeNetwork:
	remoteUnits = {}

	def __init__(self, port, rate):
		self.xbSerial = serial.Serial(port, rate)
		self.xbee = XBee(self.xbSerial, callback=self.msgReceived)
		self.startTime = time.time()

	def halt(self):
		self.xbee.halt()
		self.xbSerial.close()

	def heartbeat(self):
		syncTime = floatToFixed(time.time() - self.startTime)
		self.xbee.tx_long_addr(
			dest_addr=b'\x00\x00\x00\x00\x00\x00\xFF\xFF',
			data=struct.pack('<cLB', 'S', syncTime, 0))

	def msgReceived(self, data):
		if data['id'] == 'rx_long_addr':
			# Message from a device
			if data['rf_data'][0] == 'u':
				# Update message
				srcAddr = data['source_addr'].encode('hex')
				name = devNames.get(srcAddr, srcAddr)
				(rssi,) = struct.unpack("<B", data['rssi'])
				(cmd, timeErr, timeDown, timeUp) = \
					struct.unpack("<clLL", data['rf_data'])

				state = {
					'name'	   : name,
					'lastHeard': time.time() - self.startTime,
					'rssi'     : rssi,
					'timeErr'  : fixedToFloat(timeErr),
					'timeDown' : fixedToFloat(timeDown),
					'timeUp'   : fixedToFloat(timeUp)}
				self.remoteUnits[srcAddr] = state
				msg = {
					'command': 'updateDevices', 
					'timeNow': time.time() - self.startTime,
					'devices': self.remoteUnits}
				factory.broadcast(json.dumps(msg))


if __name__ == '__main__':
	devNames = {'0013a20040af8322': 'Green button'}

	running 	= False
	prevTime	= 0.0
	lastTime	= time.time()

	xbNet = XBeeNetwork('/dev/ttyAMA0', 57600)
	hb = LoopingCall(xbNet.heartbeat)
	hb.start(1.0)

	log.startLogging(sys.stdout)

	factory = BroadcastServerFactory("ws://localhost:8880", debug = False)
	resource = WebSocketResource(factory)

	root = File('static/')
	root.putChild('ws', resource)

	site = Site(root)
	reactor.listenTCP(8880, site)

	protocol = EchoClientDatagramProtocol()
	reactor.listenUDP(0, protocol)

	pinger = LoopingCall(protocol.broadcastPing)
	pinger.start(1.0)

	reactor.run()

	# Reactor has returned, program is ending.
	xbNet.halt()
