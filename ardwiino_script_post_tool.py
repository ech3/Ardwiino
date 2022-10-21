#!/usr/bin/env python3
from pprint import pp
import subprocess
import sys
import re
import traceback
try:
    import usb
except ImportError:
    subprocess.check_call([sys.executable, "-m", "pip", "install", "pyusb"])
try:
    import libusb_package
except ImportError:
    subprocess.check_call([sys.executable, "-m", "pip", "install", "libusb-package"])

import libusb_package

import usb.core
import usb.util
import os
REBOOT=48
BOOTLOADER=49
BOOTLOADER_SERIAL=50

def launch_dfu():
    dev = libusb_package.find(idVendor=0x03eb)
    dev.ctrl_transfer(0xA1, 3, 0, 0, 8)
    command = [0x04, 0x03, 0x00]
    dev.ctrl_transfer(0x21, 1, 0, 0, command)

def launch_dfu_no_reset():
    dev = libusb_package.find(idVendor=0x03eb)
    dev.ctrl_transfer(0xA1, 3, 0, 0, 8)
    command = [0x04, 0x03, 0x01, 0x00, 0x00]
    dev.ctrl_transfer(0x21, 1, 0, 0, command)
    dev.ctrl_transfer(0x21, 1, 0, 0)
Import("env")

def post_upload(source, target, env):
    if "/arduino_uno/" in str(source[0]):
        env.TouchSerialPort("$UPLOAD_PORT", 2400)
    if "_usb" in str(source[0]):
        launch_dfu()


env.AddPostAction("upload", post_upload)