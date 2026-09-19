# A tiny local HTTP -> HTTPS forwarder for desktop testing when the Qt build at hand has no
# usable TLS (Qt 4.7/4.8 MinGW ships OpenSSL 0.9.8, which ok.ru rejects). The app's
# OkNetwork::setUrlRewritePrefix("http://127.0.0.1:8080/") turns "https://host/path" into
# "http://127.0.0.1:8080/host/path"; this script does the TLS to the real host with Python's
# modern stack and streams the reply back unchanged. Never used on the phone.
#
#   python tools/tls-forwarder.py [port]
#   set OKM_PROXY=http://127.0.0.1:8080/ && okm-cli creds.txt
import http.client
import ssl
import sys
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

HOP_BY_HOP = {"connection", "keep-alive", "transfer-encoding", "te", "trailer", "upgrade", "proxy-connection"}


class Forwarder(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def _forward(self):
        # /host/path?query  ->  https://host/path?query
        path = self.path.lstrip("/")
        host, _, rest = path.partition("/")
        if not host:
            self.send_error(400, "missing host")
            return
        length = int(self.headers.get("Content-Length") or 0)
        body = self.rfile.read(length) if length else None

        headers = {k: v for k, v in self.headers.items() if k.lower() not in HOP_BY_HOP and k.lower() != "host"}
        headers["Host"] = host
        conn = http.client.HTTPSConnection(host, 443, timeout=60, context=ssl.create_default_context())
        try:
            conn.request(self.command, "/" + rest, body=body, headers=headers)
            resp = conn.getresponse()
            data = resp.read()
            self.send_response(resp.status, resp.reason)
            for k, v in resp.getheaders():
                if k.lower() in HOP_BY_HOP or k.lower() == "content-length":
                    continue
                self.send_header(k, v)
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)
        except Exception as e:  # noqa: BLE001 - report anything to the client
            self.send_error(502, str(e))
        finally:
            conn.close()

    do_GET = _forward
    do_POST = _forward

    def log_message(self, fmt, *args):
        sys.stderr.write("%s %s\n" % (self.command, self.path.split("?")[0]))


if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
    print("forwarding http://127.0.0.1:%d/<host>/<path> -> https://<host>/<path>" % port)
    ThreadingHTTPServer(("127.0.0.1", port), Forwarder).serve_forever()
