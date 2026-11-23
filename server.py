"""Servidor local: serve index.html e chama o programa em C (./edo). Uso: python3 server.py [porta]"""
import json, os, subprocess, sys
from http.server import BaseHTTPRequestHandler, HTTPServer

DIR = os.path.dirname(os.path.abspath(__file__))
CAMPOS = ("a", "b", "y0", "n", "f", "df", "exata")


class H(BaseHTTPRequestHandler):
    def _send(self, code, body, tipo):
        self.send_response(code)
        self.send_header("Content-Type", tipo)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path.split("?")[0] not in ("/", "/index.html"):
            return self._send(404, b"nao encontrado", "text/plain")
        with open(os.path.join(DIR, "index.html"), "rb") as f:
            self._send(200, f.read(), "text/html; charset=utf-8")

    def do_POST(self):
        if self.path != "/calc":
            return self._send(404, b"nao encontrado", "text/plain")
        tam = int(self.headers.get("Content-Length", 0))
        if tam > 5000:
            return self._send(413, b"muito grande", "text/plain")
        try:
            d = json.loads(self.rfile.read(tam))
            args = [os.path.join(DIR, "edo")] + [str(d.get(k, "")).strip() for k in CAMPOS]
            r = subprocess.run(args, capture_output=True, text=True, timeout=10)  # lista de args: sem shell
            out = r.stdout.encode() or b'{"ok":false,"erro":"falha ao executar o programa em C"}'
        except Exception as ex:
            out = json.dumps({"ok": False, "erro": f"requisicao invalida: {ex}"}).encode()
        self._send(200, out, "application/json")

    def log_message(self, *a):
        pass


if __name__ == "__main__":
    porta = int(sys.argv[1]) if len(sys.argv) > 1 else 8765
    print(f"http://localhost:{porta}")
    HTTPServer(("127.0.0.1", porta), H).serve_forever()
