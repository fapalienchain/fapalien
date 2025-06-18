#!/bin/bash
# use testnet settings,  if you need mainnet,  use ~/.fapaliencore/fapaliend.pid file instead
fapalien_pid=$(<~/.fapaliencore/testnet3/fapaliend.pid)
sudo gdb -batch -ex "source debug.gdb" fapaliend ${fapalien_pid}
