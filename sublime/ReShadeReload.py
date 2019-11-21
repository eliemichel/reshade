from sublime_plugin import TextCommand
from socket import create_connection

class ReshadereloadCommand(TextCommand):
	def run(self, edit):
		self.host = "127.0.0.1"
		self.port = 36152

		#self.view.insert(edit, 0, "Reloading ReShade instance at 127.0.0.1:36150...")
		print("Reloading ReShade instance at {}:{}...".format(self.host, self.port))
		s = create_connection((self.host, self.port))
		s.send(b"reload\0")

