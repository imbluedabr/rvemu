# RVEMU

A simple minimal rv32i_zicsr emulator. I still have to implement PMP and the CLINT, PLIC for better interrupt handeling.

## EasyRTOS
  The test directory contains a very basic multitasking operating system. It has 6 syscalls and can suport up to 4 threads concurently.
  * test/usr contains user programs
  * test/sys contains the kernel
  * test/drv contains the drivers

## Build
  1. `git clone https://github.com/imbluedabr/rvemu`
  2. `cd rvemu`
  3. `make all`
  4. `./rvemu`
