#!/usr/bin/env python3
# Copyright (c) David Muse
# See the file COPYING for more information.

# For #10697: check ./oradecode and ./oraproxy against the checked-in
# captures in samples/, offline.  Run by hand, from this directory - it is
# not part of test.sh or the Makefile, and needs no database.
#
#   ./oratoolstest.py [OPROXYPORT]
#
# Part 1 runs oradecode over every .oraproxy and .cap file in samples/.
# A file fails if oradecode exits non-zero, prints a traceback or prints
# "undecoded trailing bytes".  The six samples that used to stall at their
# first data descriptor packet (type 15) must also reach their last
# packets: the decode has to end with the packet the capture ends with.
#
# Part 2 replays captures through a live oraproxy started with
# ORAPROXY_UNIVERSAL_REPS=1.  The script plays both ends from one thread:
# it sends the capture's chunks in the order they were captured, client
# chunks to the proxy's listening port and server chunks to a socket that
# stands in for the server, and collects what comes out of the other side
# of the proxy.  Then it compares:
#   - the ojdbc and OCI native captures must come out unchanged,
#     since the rewrite is limited to the data type message, and these
#     offer nothing in it for the rewrite to change
#   - oracle122-oci-portable-login-select.cap must come out with exactly
#     9 data type entries rewritten (types 0x19-0x21), the same entries
#     the data type message in
#     oracle122-oci-portable-universalreps-login-select.cap has
#   - a capture of the same exchange with the entries put back to a
#     two-representation form (what 10048-dev-oci23api7-universalreps-
#     realserver.oraproxy was captured from) must get its rewrite
#     back, without the type 0xb5 entry that already offers only the
#     universal form
# The proxy's lengths never change, so each side's output is the same
# length as its input.

import os
import re
import socket
import struct
import subprocess
import sys
import tempfile
import time

SAMPLES = "samples"

STALLED = [
	("10005-dev-oci23-native-piecewiselobread-smallsdu-realserver-"
							"oracle122.oraproxy"),
	"10314-dev-pythonoracledb-thin-main-sqlrelay-before.oraproxy",
	"10314-dev-pythonoracledb-thin-main-sqlrelay-after.oraproxy",
	"10569-dev-ojdbc-lobs-noprefetch-realserver-oracle122.oraproxy",
	"10569-dev-ojdbc-lobs-prefetch0-realserver-oracle122.oraproxy",
	"10569-dev-ojdbc-lobs-prefetch4000-realserver-oracle122.oraproxy",
]

UNCHANGED = [
	"10569-dev-ojdbc-lobs-realserver-oracle122.oraproxy",
	"10701-dev-ojdbc-dml-realserver-oracle122.oraproxy",
	"10562-dev-oci23-native-dtbinds-tzv26-realserver-oracle122.oraproxy",
	"oracle122-oci-native-login-select.cap",
]

PORTABLE = "oracle122-oci-portable-login-select.cap"
PORTABLE_REF = "oracle122-oci-portable-universalreps-login-select.cap"
ALREADY = "10048-dev-oci23api7-universalreps-realserver.oraproxy"

# the two-representation entry oraproxy looks for, and its rewritten form
ENTRY = re.compile(
	rb"(.)\x00\1\x00(.)\x00\1\x00\x01\x00\x00\x00", re.DOTALL)

failures = []


def fail(message):
	failures.append(message)
	print("FAIL: %s" % message)


def readchunks(path):
	with open(path, "rb") as f:
		data = f.read()
	chunks = []
	pos = 0
	while pos < len(data):
		(length,) = struct.unpack(">I", data[pos + 1:pos + 5])
		chunks.append((data[pos:pos + 1], data[pos + 5:pos + 5 + length]))
		pos += 5 + length
	return chunks


def stream(chunks, direction):
	return b"".join(c for d, c in chunks if d == direction)


def entries(data):
	# the (type, second representation) of every entry-shaped run
	return [(m.group(1)[0], m.group(2)[0]) for m in ENTRY.finditer(data)]


def universal(t):
	return (bytes([t]) + b"\x00" + bytes([t]) + b"\x00\x01\x00" +
			bytes([t]) + b"\x00\x01\x00\x00\x00")


def undo(m):
	# an entry the proxy would rewrite, from its rewritten form: put the
	# second field back to 2 - except type 0xb5, which was already
	# universal before the rewrite
	t = m.group(1)
	if t[0] == 0xb5:
		return m.group(0)
	return t + b"\x00" + t + b"\x00\x02\x00" + t + b"\x00\x01\x00\x00\x00"


def decode(path):
	proc = subprocess.run([sys.executable, "-I", "./oradecode", path],
				stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	out = proc.stdout.decode("utf-8", "replace")
	err = proc.stderr.decode("utf-8", "replace")
	problem = None
	if proc.returncode != 0 or "Traceback" in err or "Traceback" in out:
		problem = "oradecode failed"
	elif "undecoded trailing bytes" in err:
		problem = "undecoded trailing bytes"
	return out, err, problem


def part1():
	names = sorted(n for n in os.listdir(SAMPLES)
				if n.endswith(".oraproxy") or n.endswith(".cap"))
	for name in names:
		out, err, problem = decode(os.path.join(SAMPLES, name))
		if problem:
			fail("%s: %s" % (name, problem))
	print("oradecode ran over %d samples" % len(names))

	for name in STALLED:
		path = os.path.join(SAMPLES, name)
		chunks = readchunks(path)
		out, err, problem = decode(path)
		packets = re.findall(r"^\[(\d+)\] ", out, re.MULTILINE)
		last = out[out.rfind("\n[") + 1:]
		# the last chunk's direction has to be the last packet's
		want = "client -> server" if chunks[-1][0] == b"C" \
						else "server -> client"
		if problem or not packets or want not in last.split("\n")[0]:
			fail("%s: decode does not reach the last packet "
				"(%s)" % (name, last.split("\n")[0]))
		else:
			print("  %s: %s packets, ends on %s" % (name,
				packets[-1], last.split("\n")[0][:60]))


class Replay:
	# one capture through one oraproxy, with this script as both ends
	def __init__(self, port, chunks):
		self.port = port
		self.chunks = chunks

	def run(self, tmpdir):
		listener = socket.socket()
		listener.bind(("127.0.0.1", 0))
		listener.listen(1)
		serverport = listener.getsockname()[1]

		env = dict(os.environ)
		env["ORAPROXY_UNIVERSAL_REPS"] = "1"
		proxy = subprocess.Popen([sys.executable, "-I", "./oraproxy",
				str(self.port), "127.0.0.1", str(serverport),
				os.path.join(tmpdir, "replay.cap")],
				env=env, stderr=subprocess.PIPE)
		try:
			client = None
			for i in range(50):
				try:
					client = socket.create_connection(
						("127.0.0.1", self.port))
					break
				except OSError:
					time.sleep(0.1)
			if client is None:
				raise RuntimeError("could not connect to oraproxy")
			listener.settimeout(10)
			server, addr = listener.accept()
			toserver = self.play(client, server)
			client.close()
			server.close()
		finally:
			listener.close()
			try:
				proxy.wait(10)
			except subprocess.TimeoutExpired:
				proxy.kill()
				proxy.wait()
			log = proxy.stderr.read().decode("utf-8", "replace")
			proxy.stderr.close()
		return toserver[0], toserver[1], log

	def drain(self, sock, sofar, want, idle):
		# read until want bytes have arrived in total, or nothing more
		# comes for idle seconds (the proxy holds back a partial
		# packet of the data type message until the rest arrives)
		sock.settimeout(idle)
		while len(sofar) < want:
			try:
				got = sock.recv(65536)
			except socket.timeout:
				break
			if not got:
				break
			sofar += got
		return sofar

	def play(self, client, server):
		atserver = b""
		atclient = b""
		sentc = 0
		sents = 0
		for direction, chunk in self.chunks:
			if direction == b"C":
				client.sendall(chunk)
				sentc += len(chunk)
				atserver = self.drain(server, atserver,
							sentc, 0.3)
			else:
				server.sendall(chunk)
				sents += len(chunk)
				atclient = self.drain(client, atclient,
							sents, 5)
		atserver = self.drain(server, atserver, sentc, 5)
		atclient = self.drain(client, atclient, sents, 5)
		return atserver, atclient


def replay(port, chunks):
	tmpdir = tempfile.mkdtemp()
	try:
		return Replay(port, chunks).run(tmpdir)
	finally:
		for n in os.listdir(tmpdir):
			os.remove(os.path.join(tmpdir, n))
		os.rmdir(tmpdir)


def changed(before, after):
	# the type of every entry-shaped run in before that is not the same
	# in after
	out = []
	for m in ENTRY.finditer(before):
		if after[m.start():m.end()] != m.group(0):
			out.append(m.group(1)[0])
	return out


def part2(port):
	for name in UNCHANGED:
		chunks = readchunks(os.path.join(SAMPLES, name))
		toserver, toclient, log = replay(port, chunks)
		if toserver != stream(chunks, b"C"):
			fail("%s: the client's bytes changed" % name)
		elif toclient != stream(chunks, b"S"):
			fail("%s: the server's bytes changed" % name)
		else:
			print("  %s: unchanged" % name)

	# portable: 9 entries rewritten, matching the reference capture
	chunks = readchunks(os.path.join(SAMPLES, PORTABLE))
	ref = readchunks(os.path.join(SAMPLES, PORTABLE_REF))
	toserver, toclient, log = replay(port, chunks)
	sent = stream(chunks, b"C")
	types = changed(sent, toserver)
	refstream = stream(ref, b"C")
	if len(toserver) != len(sent):
		fail("%s: the client's bytes changed length" % PORTABLE)
	elif toclient != stream(chunks, b"S"):
		fail("%s: the server's bytes changed" % PORTABLE)
	elif types != list(range(0x19, 0x22)):
		fail("%s: rewrote types %s, wanted 0x19-0x21" %
			(PORTABLE, " ".join("0x%02x" % t for t in types)))
	elif [t for t in types if universal(t) not in refstream]:
		fail("%s: a rewritten entry is not in %s" %
						(PORTABLE, PORTABLE_REF))
	elif sum(1 for a, b in zip(sent, toserver) if a != b) == 0:
		fail("%s: nothing was rewritten" % PORTABLE)
	else:
		print("  %s: %d entries rewritten (%s)" % (PORTABLE,
			len(types), " ".join("0x%02x" % t for t in types)))

	# 10048: its capture holds the rewritten bytes (it was captured from
	# the proxy), so put the first representation back to the
	# two-representation form first.  the type 0xb5 entry already
	# offers only the universal form (second representation 1) before
	# the rewrite, so it is left as it was captured
	chunks = readchunks(os.path.join(SAMPLES, ALREADY))
	original = []
	for direction, chunk in chunks:
		if direction == b"C":
			chunk = ENTRY.sub(undo, chunk)
		original.append((direction, chunk))
	toserver, toclient, log = replay(port, original)
	sent = stream(original, b"C")
	expect = stream(chunks, b"C")
	types = changed(sent, toserver)
	count = len(changed(expect, sent))
	if toserver != expect:
		fail("%s: the replay does not give the captured bytes back "
			"(%d entries restored, %d rewritten)" %
			(ALREADY, count, len(types)))
	elif 0xb5 in types or len(types) != count:
		fail("%s: rewrote %d of %d entries" %
			(ALREADY, len(types), count))
	else:
		print("  %s: %d entries rewritten, 0xb5 left alone" %
			(ALREADY, len(types)))


def main():
	port = int(sys.argv[1]) if len(sys.argv) > 1 else 15231

	print("oradecode over the samples")
	part1()
	print("oraproxy replay with ORAPROXY_UNIVERSAL_REPS=1")
	part2(port)

	if failures:
		print("%d failures" % len(failures))
		return 1
	print("passed")
	return 0


if __name__ == "__main__":
	sys.exit(main())
