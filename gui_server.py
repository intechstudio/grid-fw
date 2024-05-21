import http.server
import socketserver
import sys

PORT = 8000

class MyHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cache-Control", "no-cache, no-store, must-revalidate")
        self.send_header("Pragma", "no-cache")
        self.send_header("Expires", "0")
        super().end_headers()

try:
    with socketserver.TCPServer(("", PORT), MyHTTPRequestHandler) as httpd:
        print("serving at port", PORT)
        print("http://localhost:8000/grid_gui/build/index.html")
        httpd.serve_forever()
except KeyboardInterrupt:
    print("\nCtrl+C detected. Shutting down the server.")
    httpd.server_close()
    sys.exit(0)
