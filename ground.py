#!/usr/bin/env python3
"""Minimal ground system for the cFS hands-on course.

Replaces cFS-GroundSystem 2.1.0, which needs Python 2 and PyQt4. Uses only
the Python 3 standard library and no platform-specific modules, so it is
meant to run on Linux, macOS and Windows (tested on Linux, Python 3.8-3.13).

    python3 ground.py listen              show telemetry from TO_LAB
    python3 ground.py send MID CC [...]   send a command to CI_LAB
    python3 ground.py enable              ask TO_LAB to send telemetry here
    python3 ground.py dump FILE           show the packets in a DS file

Examples:
    python3 ground.py listen
    python3 ground.py listen --only IMU EVENT
    python3 ground.py send 0x1882 0                 # SAMPLE_APP NOOP
    python3 ground.py send 0x18A0 1                 # IMU_APP RESET_COUNTERS
    python3 ground.py dump build/exe/cpu1/cf/imu00000001.dat

Run "python3 ground.py COMMAND --help" for the options of each command.

cFS is assumed to run on a little-endian computer (x86 or ARM), which is
the case for every PC and Raspberry Pi.
"""

import argparse
import datetime
import os
import re
import socket
import struct
import sys
import time

CI_PORT = 1234              # CI_LAB receives commands here
TO_PORT = 1235              # TO_LAB sends telemetry here
TO_ENABLE_MID = 0x1880      # TO_LAB_CMD_MID
TO_ENABLE_CC = 6            # TO_OUTPUT_ENABLE_CC
RESEND_ENABLE_SEC = 5       # re-send TO_LAB enable after this long without telemetry

PRI_HDR_LEN = 6             # CCSDS primary header
CMD_HDR_LEN = 8             # primary + command secondary header
TLM_HDR_LEN = 12            # primary + telemetry secondary header (time)

CFE_FS_HDR_LEN = 64         # CFE_FS_Header_t
CFE_FS_CONTENT_TYPE = 0x63464531   # "cFE1"
DS_FILE_SUBTYPE = 12345     # DS_FILE_HDR_SUBTYPE
DS_FILE_HDR_LEN = 76        # DS_FileHeader_t with OS_MAX_PATH_LEN 64

EVENT_TYPES = {1: "DEBUG", 2: "INFO", 3: "ERROR", 4: "CRIT"}
TIME_EPOCH = datetime.datetime(1980, 1, 1)   # CFE_MISSION_TIME_EPOCH_* in sample_mission_cfg.h

TOP_DIR = os.path.dirname(os.path.abspath(__file__))


# ---------------------------------------------------------------------------
# Packet decoders. Each takes the payload (bytes after the 12-byte header)
# and returns one line of text. Add your own app's packets here.
# ---------------------------------------------------------------------------

def cmd_err(payload):
    """cFE core apps: CommandCounter, CommandErrorCounter"""
    cmd, err = struct.unpack_from("<BB", payload)
    return "cmd %d  err %d" % (cmd, err)


def err_cmd(payload):
    """sample_app, ci_lab and apps made by create_app.py: error count first"""
    err, cmd = struct.unpack_from("<BB", payload)
    return "cmd %d  err %d" % (cmd, err)


def event(payload):
    app, eid, etype = struct.unpack_from("<20sHH", payload)
    message = payload[32:32 + 122]
    return "%-5s %s %d: %s" % (EVENT_TYPES.get(etype, str(etype)), cstr(app), eid, cstr(message))


def imu_hk(payload):
    err, cmd, samples = struct.unpack_from("<BBxxI", payload)
    return "cmd %d  err %d  samples %d" % (cmd, err, samples)


def imu_data(payload):
    sample, x, y, z = struct.unpack_from("<I3f", payload)
    return "#%d  x %+.3f  y %+.3f  z %+.3f" % (sample, x, y, z)


def ds_hk(payload):
    fields = struct.unpack_from("<BBBBBBBxHHHHIIII", payload)
    cmd, err, enabled = fields[0], fields[1], fields[6]
    writes, write_errors = fields[7], fields[8]
    passed = fields[14]
    return "cmd %d  err %d  enabled %d  stored %d  file_writes %d  write_errors %d" % (
        cmd, err, enabled, passed, writes, write_errors)


# MID: (name, decoder or None)
PACKETS = {
    0x0800: ("ES_HK", cmd_err),
    0x0801: ("EVS_HK", cmd_err),
    0x0803: ("SB_HK", cmd_err),
    0x0804: ("TBL_HK", cmd_err),
    0x0805: ("TIME_HK", cmd_err),
    0x0806: ("TIME_DIAG", None),
    0x0808: ("EVENT", event),
    0x080A: ("SB_STATS", None),
    0x080B: ("ES_APP", None),
    0x080C: ("TBL_REG", None),
    0x080F: ("ES_SHELL", None),
    0x0810: ("ES_MEMSTATS", None),
    0x0880: ("TO_LAB_HK", cmd_err),
    0x0881: ("TO_LAB_DATA_TYPES", None),
    0x0883: ("SAMPLE_APP_HK", err_cmd),
    0x0884: ("CI_LAB_HK", err_cmd),
    0x08A0: ("IMU_APP_HK", imu_hk),
    0x08A1: ("IMU_APP_DATA", imu_data),
    0x08B8: ("DS_HK", ds_hk),
    0x08B9: ("DS_DIAG", None),
}


def add_app_packets():
    """Name the telemetry of other apps under apps/ from their *_msgids.h.

    Housekeeping packets of apps made by create_app.py share one layout,
    so *_HK_TLM_MID packets without their own decoder are shown with it.
    """
    apps_dir = os.path.join(TOP_DIR, "apps")
    define = re.compile(r"#define\s+(\w+)_TLM_MID\s+(0x[0-9A-Fa-f]+)")
    try:
        app_names = sorted(os.listdir(apps_dir))
    except OSError:
        return
    for app in app_names:
        inc_dir = os.path.join(apps_dir, app, "fsw", "platform_inc")
        try:
            files = [f for f in os.listdir(inc_dir) if f.endswith("_msgids.h")]
        except OSError:
            continue
        for file_name in files:
            try:
                with open(os.path.join(inc_dir, file_name), encoding="utf-8", errors="replace") as f:
                    text = f.read()
            except OSError:
                continue
            for name, value in define.findall(text):
                mid = int(value, 16)
                if mid not in PACKETS:
                    PACKETS[mid] = (name, err_cmd if name.endswith("_HK") else None)


def cstr(raw):
    """C string from a fixed-size char array"""
    return raw.split(b"\0", 1)[0].decode("ascii", "replace")


def describe(packet):
    """Return (name, text) for one telemetry packet"""
    if len(packet) < TLM_HDR_LEN:
        return "SHORT", "%d bytes: %s" % (len(packet), hexdump(packet))
    mid = struct.unpack_from(">H", packet)[0]
    name, decoder = PACKETS.get(mid, ("MID_0x%04X" % mid, None))
    payload = packet[TLM_HDR_LEN:]
    if decoder:
        try:
            return name, decoder(payload)
        except struct.error:
            pass
    return name, "%d bytes: %s" % (len(packet), hexdump(payload[:24]))


def packet_time(packet):
    """cFE time stamp of a telemetry packet, formatted like the cFS console.

    cFE 6.7 copies the time into the header in host byte order (seconds,
    then the upper 16 bits of the subseconds), not in the big-endian order
    of the CCSDS_WR_SEC_HDR_* macros.
    """
    seconds, subseconds = struct.unpack_from("<IH", packet, PRI_HDR_LEN)
    stamp = TIME_EPOCH + datetime.timedelta(seconds=seconds + subseconds / 65536.0)
    return stamp.strftime("%Y-%j-%H:%M:%S") + ".%02d" % (stamp.microsecond // 10000)


def hexdump(data):
    return " ".join("%02x" % b for b in bytearray(data))


def wanted(name, text, only):
    if not only:
        return True
    line = (name + " " + text).upper()
    return any(word.upper() in line for word in only)


# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------

def build_command(mid, cc, payload=b""):
    """CCSDS command packet as cFE 6.7 expects it on a little-endian host.

    The secondary header is one host-order uint16: checksum in the low byte,
    function code in the high byte. On a little-endian host that puts the
    checksum in byte 6 and the function code in byte 7.
    """
    length = CMD_HDR_LEN + len(payload)
    packet = bytearray(struct.pack(">HHH", mid, 0xC000, length - 7))
    packet += bytearray([0, cc & 0x7F]) + bytearray(payload)
    checksum = 0xFF
    for b in packet:
        checksum ^= b
    packet[6] = checksum
    return bytes(packet)


def send_packet(host, port, packet):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.sendto(packet, (host, port))
    finally:
        sock.close()


def local_ip_for(host):
    """IP address of this computer as seen from host (no packet is sent)"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.connect((host, CI_PORT))
        return sock.getsockname()[0]
    except OSError:
        return "127.0.0.1"
    finally:
        sock.close()


def send_enable(host, dest_ip):
    payload = struct.pack("16s", dest_ip.encode("ascii"))
    send_packet(host, CI_PORT, build_command(TO_ENABLE_MID, TO_ENABLE_CC, payload))


class ParamAction(argparse.Action):
    """Collect --u8/--u16/.../--string parameters in the order given"""

    FORMATS = {"u8": "<B", "u16": "<H", "u32": "<I",
               "i8": "<b", "i16": "<h", "i32": "<i", "f32": "<f"}

    def __call__(self, parser, namespace, value, option_string=None):
        kind = option_string.lstrip("-")
        try:
            if kind == "string":
                size, _, text = value.partition(":")
                data = struct.pack("%ds" % int(size), text.encode("ascii"))
            elif kind == "f32":
                data = struct.pack(self.FORMATS[kind], float(value))
            else:
                data = struct.pack(self.FORMATS[kind], int(value, 0))
        except (ValueError, struct.error) as e:
            parser.error("bad value for %s: %s (%s)" % (option_string, value, e))
        params = getattr(namespace, "params", None) or []
        params.append(data)
        namespace.params = params


# ---------------------------------------------------------------------------
# Subcommands
# ---------------------------------------------------------------------------

def do_listen(args):
    add_app_packets()
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.bind(("", args.port))
    except OSError as e:
        sys.exit("error: cannot listen on UDP port %d (%s). Is another ground.py running?"
                 % (args.port, e))
    sock.settimeout(0.5)

    dest_ip = args.dest_ip or local_ip_for(args.host)
    print("Listening for telemetry on UDP port %d. Press Ctrl+C to stop." % args.port)

    last_packet = 0.0
    last_enable = 0.0
    while True:
        now = time.time()
        if not args.no_enable and now - last_packet > RESEND_ENABLE_SEC \
                and now - last_enable > RESEND_ENABLE_SEC:
            send_enable(args.host, dest_ip)
            last_enable = now
            print("  (asked TO_LAB at %s:%d to send telemetry to %s; waiting for cFS...)"
                  % (args.host, CI_PORT, dest_ip))
        try:
            packet, _ = sock.recvfrom(65535)
        except socket.timeout:
            continue
        last_packet = time.time()
        name, text = describe(packet)
        if wanted(name, text, args.only):
            print("%s  %-14s %s" % (time.strftime("%H:%M:%S"), name, text))
            sys.stdout.flush()


def do_send(args):
    payload = b"".join(getattr(args, "params", None) or [])
    packet = build_command(args.mid, args.cc, payload)
    send_packet(args.host, args.port, packet)
    print("sent MID 0x%04X CC %d (%d bytes) to %s:%d: %s"
          % (args.mid, args.cc, len(packet), args.host, args.port, hexdump(packet)))


def do_enable(args):
    dest_ip = args.dest_ip or local_ip_for(args.host)
    send_enable(args.host, dest_ip)
    print("asked TO_LAB at %s:%d to send telemetry to %s:%d"
          % (args.host, CI_PORT, dest_ip, TO_PORT))


def do_dump(args):
    add_app_packets()
    try:
        with open(args.file, "rb") as f:
            data = f.read()
    except OSError as e:
        sys.exit("error: %s" % e)

    offset = 0
    if len(data) >= CFE_FS_HDR_LEN and struct.unpack_from(">I", data)[0] == CFE_FS_CONTENT_TYPE:
        subtype = struct.unpack_from(">I", data, 4)[0]
        description = cstr(data[32:64])
        offset = CFE_FS_HDR_LEN
        print("cFE file header: \"%s\"" % description)
        if subtype == DS_FILE_SUBTYPE and len(data) >= offset + DS_FILE_HDR_LEN:
            close_sec, _, index = struct.unpack_from("<IIH", data, offset)
            file_name = cstr(data[offset + 12:offset + DS_FILE_HDR_LEN])
            if close_sec:
                closed = TIME_EPOCH + datetime.timedelta(seconds=close_sec)
                state = "closed at " + closed.strftime("%Y-%j-%H:%M:%S")
            else:
                state = "still open"
            print("DS file header:  %s, destination %d, %s" % (file_name, index, state))
            offset += DS_FILE_HDR_LEN
    else:
        print("no cFE file header, reading packets from the start")

    count = 0
    while offset + PRI_HDR_LEN <= len(data):
        size = struct.unpack_from(">H", data, offset + 4)[0] + 7
        packet = data[offset:offset + size]
        if len(packet) < size:
            print("(last packet is cut off: %d of %d bytes)" % (len(packet), size))
            break
        name, text = describe(packet)
        if wanted(name, text, args.only):
            stamp = packet_time(packet) if len(packet) >= TLM_HDR_LEN else "-"
            print("%s  %-14s %s" % (stamp, name, text))
        count += 1
        offset += size
    print("%d packets" % count)


def main():
    parser = argparse.ArgumentParser(
        description="Minimal ground system for the cFS hands-on course.",
        epilog="Run 'python3 ground.py COMMAND --help' for details.")
    sub = parser.add_subparsers(dest="command", metavar="COMMAND")

    p = sub.add_parser("listen", help="show telemetry from TO_LAB")
    p.add_argument("--host", default="127.0.0.1",
                   help="computer running cFS (default 127.0.0.1)")
    p.add_argument("--port", type=int, default=TO_PORT,
                   help="UDP port to listen on (default %d)" % TO_PORT)
    p.add_argument("--dest-ip", help="IP address TO_LAB sends to (default: this computer)")
    p.add_argument("--no-enable", action="store_true",
                   help="do not send the TO_LAB enable command")
    p.add_argument("--only", nargs="+", metavar="WORD",
                   help="show only packets whose line contains one of these words")
    p.set_defaults(func=do_listen)

    p = sub.add_parser("send", help="send a command to CI_LAB")
    p.add_argument("mid", type=lambda s: int(s, 0), help="message ID, e.g. 0x1882")
    p.add_argument("cc", type=lambda s: int(s, 0), help="command code, e.g. 0")
    p.add_argument("--host", default="127.0.0.1",
                   help="computer running cFS (default 127.0.0.1)")
    p.add_argument("--port", type=int, default=CI_PORT,
                   help="CI_LAB UDP port (default %d)" % CI_PORT)
    for kind in ("u8", "u16", "u32", "i8", "i16", "i32", "f32"):
        p.add_argument("--" + kind, action=ParamAction, metavar="N",
                       help="add a %s parameter" % kind)
    p.add_argument("--string", action=ParamAction, metavar="LEN:TEXT",
                   help="add a text parameter of LEN bytes, e.g. 16:hello")
    p.set_defaults(func=do_send)

    p = sub.add_parser("enable", help="ask TO_LAB to send telemetry to this computer")
    p.add_argument("--host", default="127.0.0.1",
                   help="computer running cFS (default 127.0.0.1)")
    p.add_argument("--dest-ip", help="IP address TO_LAB sends to (default: this computer)")
    p.set_defaults(func=do_enable)

    p = sub.add_parser("dump", help="show the packets in a DS file")
    p.add_argument("file", help="file written by DS, e.g. build/exe/cpu1/cf/imu00000001.dat")
    p.add_argument("--only", nargs="+", metavar="WORD",
                   help="show only packets whose line contains one of these words")
    p.set_defaults(func=do_dump)

    args = parser.parse_args()
    if not args.command:
        parser.print_help()
        return
    try:
        args.func(args)
    except KeyboardInterrupt:
        print()


if __name__ == "__main__":
    main()
