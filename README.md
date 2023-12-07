# CPU Project - RISCV - Verilog - C++ Simulator

## Usage and Installation

Update tb.sv, ram.sv, and rom.sv files to desired .dat files.

The code is tested and ran on AMD-Vivado Simulator.

To Run C++ simulator file, need to change the binary file to desired .dat file

The C++ program only supports 32-bit one-line pc instructions. If the binary file is in lines of 8 bits, you need to use the reformat.sh bash script to modify your binary dat file to 32-bit one-liner. 

> In bash script, changed the input and output file name as you wish

```
chmod 777 reformat.sh
./reformat.sh

```

To run code: 
```
g++ cpu.cpp
./a.out
```

Your newly formated binary dat file should be created and use that as the input file in C++ program

## Progress

The cpu.v module handles I-type instructions and R-type instructions with pipelines and with hazard detection and control. 

Currently implementing load and store.

The cpu.cpp simulator handles I and R-type Instructions, displays results in rd and pc registers. Observing stalls.

Currently implementing laod and store.
