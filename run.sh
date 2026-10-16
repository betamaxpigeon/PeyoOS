#!/bin/bash
qemu-system-x86_64 \
  -m 512M \
  -boot order=d \
  -no-reboot \
  -drive if=pflash,format=raw,readonly=on,file=/Users/vhscrow/share/qemu/edk2-x86_64-code.fd \
  -drive if=pflash,format=raw,file=/tmp/peyoos-uefi-vars.fd \
  -cdrom peyoos.iso \
  -debugcon stdio \
  -global isa-debugcon.iobase=0xe9
