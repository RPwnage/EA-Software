from SocketServer import ThreadingMixIn
from BaseHTTPServer import HTTPServer, BaseHTTPRequestHandler
import sys
import time
import optparse

errorcode = 404

class NormalHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.do_all()

    def do_POST(self):
        self.do_all()
    
    def do_HEAD(self):
        self.do_all()
    
    def do_PUT(self):
        self.do_all();
    
    def do_DELETE(self):
        self.do_all();

    def do_all(self):
        self.send_response(200)
        self.send_header('Content-type','text/plain')
        self.end_headers()
        self.wfile.write(self.path)

class NoResponseHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.do_all()

    def do_POST(self):
        self.do_all()

    def do_all(self):
        pass

class SlowResponseHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.do_all()

    def do_POST(self):
        self.do_all()

    def do_all(self):
        time.sleep(30)
        self.send_response(200)
        self.send_header('Content-type','text/plain')
        self.end_headers()
        self.wfile.write(self.path)

class ErrorResponseHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.do_all()

    def do_POST(self):
        self.do_all()

    def do_all(self):
        global errorcode
        self.send_response(errorcode)
        self.send_header('Content-type','text/plain')
        self.end_headers()
        self.wfile.write(self.path)

class ThreadingHTTPServer(ThreadingMixIn, HTTPServer):
    pass

handlers = {'normal':NormalHandler, 'noresponse':NoResponseHandler, 'slow':SlowResponseHandler, 'error':ErrorResponseHandler}

parser = optparse.OptionParser(usage = '%prog --ip=IP --port=PORT --handler=HANDLER --error=ERROR')
parser.add_option('-i', '--ip', dest='ip',
    help='listen on ip', metavar='IP')
parser.add_option('-p', '--port', dest='port', type=int, default=9000,
    help='listen on port PORT', metavar='PORT')
parser.add_option('-l', '--handler', dest='handler',
    help='how to respond', metavar='HANDLER')
parser.add_option('-e', '--error', dest='error', type=int,
    help='what error to respond with', metavar='ERROR')

opts, args = parser.parse_args()

ip = opts.ip
port = opts.port
handler = opts.handler
if opts.error:
    errorcode = opts.error

server = ThreadingHTTPServer((ip, port), handlers[handler])
server.serve_forever()
