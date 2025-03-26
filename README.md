## QEMU Monitor Cheat Sheet

- Start the monitor with `Ctrl + A` then `C`
- Exit QEMU by using `q` in the monitor
- Use `info registers` to view CPU regs
- Use `stop` and `cont` to control execution


[OpenSBI Calls](https://courses.stephenmarz.com/my-courses/cosc562/risc-v/opensbi-calls/)


### NOTE

unimp is a "pseudo" instruction.

According to RISC-V Assembly Programmer's Manual, the assembler translates unimp to the following instruction:

```csrrw x0, cycle, x0```

This reads and writes the cycle register into x0. Since cycle is a read-only register, CPU determines that the instruction is invalid and triggers an illegal instruction exception.


## Current Status 
[Reached Here](https://operating-system-in-1000-lines.vercel.app/en/11-page-table)