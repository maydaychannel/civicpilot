#!/usr/bin/env python3
# simple boardd wrapper that updates the panda first
import os
import usb1
import time
import subprocess
from typing import NoReturn
from functools import cmp_to_key

from panda import DEFAULT_FW_FN, DEFAULT_H7_FW_FN, MCU_TYPE_H7, Panda, PandaDFU
from common.basedir import BASEDIR
from common.params import Params


def get_expected_signature(panda: Panda) -> bytes:
  fn = DEFAULT_H7_FW_FN if (panda.get_mcu_type() == MCU_TYPE_H7) else DEFAULT_FW_FN

  try:
    return Panda.get_signature_from_firmware(fn)
  except Exception:
    print("Error computing expected signature")
    return b""


def flash_panda(panda_serial: str) -> Panda:
  panda = Panda(panda_serial)

  # debug skip flashing as its broken in flowpilot
  # use freeon clone to flash panda if needed running OPKR from phr00t's openpilot rep
  return panda

  fw_signature = get_expected_signature(panda)

  panda_version = "bootstub" if panda.bootstub else panda.get_version()
  panda_signature = b"" if panda.bootstub else panda.get_signature()
  print(f"Panda {panda_serial} connected, version: {panda_version}, signature {panda_signature.hex()[:16]}, expected {fw_signature.hex()[:16]}")

  if panda.bootstub or panda_signature != fw_signature:
    print("Panda firmware out of date, update required")
    panda.flash()
    print("Done flashing")

  if panda.bootstub:
    bootstub_version = panda.get_version()
    print(f"Flashed firmware not booting, flashing development bootloader. Bootstub version: {bootstub_version}")
    panda.recover()
    print("Done flashing bootloader")

  if panda.bootstub:
    print("Panda still not booting, exiting")
    raise AssertionError

  panda_signature = panda.get_signature()
  if panda_signature != fw_signature:
    print("Version mismatch after flashing, exiting")
    raise AssertionError

  return panda


def panda_sort_cmp(a: Panda, b: Panda):
  a_type = a.get_type()
  b_type = b.get_type()

  # make sure the internal one is always first
  if a.is_internal() and not b.is_internal():
    return -1
  if not a.is_internal() and b.is_internal():
    return 1

  # sort by hardware type
  if a_type != b_type:
    return a_type < b_type

  # last resort: sort by serial number
  return a.get_usb_serial() < b.get_usb_serial()


def main() -> NoReturn:
  first_run = True
  params = Params()

  # debug
  os.environ['MANAGER_DAEMON'] = 'boardd'
  os.chdir(os.path.join(BASEDIR, "selfdrive/boardd"))
  subprocess.run(["./boardd"], check=True)

  while True:
    time.sleep(1)

  while True:
    try:
      params.remove("PandaSignatures")

      # Flash all Pandas in DFU mode
      for p in PandaDFU.list():
        print(f"Panda in DFU mode found, flashing recovery {p}")
        PandaDFU(p).recover()
      time.sleep(1)

      panda_serials = Panda.list()
      if len(panda_serials) == 0:
        print("Panda not found, waiting...")
        continue

      print(f"{len(panda_serials)} panda(s) found, connecting - {panda_serials}")

      # Flash pandas
      pandas = []
      for serial in panda_serials:
        pandas.append(flash_panda(serial))

      # check health for lost heartbeat
      # OPKR panda doesn't seem to have this
      #for panda in pandas:
      #  health = panda.health()
      #  if health["heartbeat_lost"]:
      #    params.put_bool("PandaHeartbeatLost", True)
      #    print("heartbeat lost", deviceState=health, serial=panda.get_usb_serial())

        #if first_run:
        #  cloudlog.info(f"Resetting panda {panda.get_usb_serial()}")
        #  panda.reset()

      # sort pandas to have deterministic order
      pandas.sort(key=cmp_to_key(panda_sort_cmp))
      panda_serials = list(map(lambda p: p.get_usb_serial(), pandas))

      # log panda fw versions
      params.put("PandaSignatures", b','.join(p.get_signature() for p in pandas))

      # close all pandas
      for p in pandas:
        p.close()
    except (usb1.USBErrorNoDevice, usb1.USBErrorPipe):
      # a panda was disconnected while setting everything up. let's try again
      print("Panda USB exception while setting up")
      continue

    first_run = False

    # run boardd with all connected serials as arguments
    os.environ['MANAGER_DAEMON'] = 'boardd'
    os.chdir(os.path.join(BASEDIR, "selfdrive/boardd"))
    subprocess.run(["./boardd", *panda_serials], check=True)

def run():
  main()

if __name__ == "__main__":
  main()
