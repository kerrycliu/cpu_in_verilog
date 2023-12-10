# CPU Project - RISCV - Verilog - C++ Simulator

## Usage and Installation
### Verilog
Update tb.sv, ram.sv, and rom.sv files to desired .dat files.

The code is tested and ran on AMD-Vivado Simulator.

### C++ Simulator
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
> Simulator supports user input options:
> Run simulator at once.
> Run instruction by instruction.
> Display PC value.
> Display register value.

Sample output:
```
(base) Kerrys-MacBook-Pro:cpu_in_verilog kerryliu$ ./a.out 
Enter 'r': run entire program at once.  
Enter 'pc': to show pc
Enter 's': run one instruction at a time. Wait for next instruction. Ctrl C to exit.
Enter 'x1,x2...': for register value.  Ctrl C to exit.
r
register: t0 = 10010000
PC: 0
register: t0 = 10010000
PC: 4
register: t1 = 0
PC: 8
register: t3 = 0
PC: 12
register: t5 = 0
PC: 16
register: t2 = 0
PC: 20
register: t1 = 0
PC: 24
register: t0 = 10010004
PC: 28
register: t2 = ffffffff
PC: 32
register: t3 = ffffffff
PC: 36
register: t1 = ffffffff
PC: 40
register: t2 = 0
PC: 44
register: t3 = ffffffff
PC: 48
register: t0 = 10010008
PC: 52
register: t0 = 0
PC: 56
Enter 'r': run entire program at once.  
Enter 'pc': to show pc
Enter 's': run one instruction at a time. Wait for next instruction. Ctrl C to exit.
Enter 'x1,x2...': for register value.  Ctrl C to exit.
x29
Register 29: 0
Enter 'r': run entire program at once.  
Enter 'pc': to show pc
Enter 's': run one instruction at a time. Wait for next instruction. Ctrl C to exit.
Enter 'x1,x2...': for register value.  Ctrl C to exit.
s
Enter 32-bit instruction now: 
00010000000000010000001010110111
register: t0 = 10010000
PC: 60
Enter 'r': run entire program at once.  
Enter 'pc': to show pc
Enter 's': run one instruction at a time. Wait for next instruction. Ctrl C to exit.
Enter 'x1,x2...': for register value.  Ctrl C to exit.
s
Enter 32-bit instruction now: 
00000000000000101000001010010011
register: t0 = 10010000
PC: 64
Enter 'r': run entire program at once.  
Enter 'pc': to show pc
Enter 's': run one instruction at a time. Wait for next instruction. Ctrl C to exit.
Enter 'x1,x2...': for register value.  Ctrl C to exit.
s
Enter 32-bit instruction now: 
00000000000000101010001100000011
register: t1 = 0
PC: 68
Enter 'r': run entire program at once.  
Enter 'pc': to show pc
Enter 's': run one instruction at a time. Wait for next instruction. Ctrl C to exit.
Enter 'x1,x2...': for register value.  Ctrl C to exit.
s
Enter 32-bit instruction now: 
00000000000000000000111000110011
register: t3 = 0
PC: 72
Enter 'r': run entire program at once.  
Enter 'pc': to show pc
Enter 's': run one instruction at a time. Wait for next instruction. Ctrl C to exit.
Enter 'x1,x2...': for register value.  Ctrl C to exit.

```
