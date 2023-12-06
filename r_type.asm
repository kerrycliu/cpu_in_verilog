.text 
addi t0, x0, 5		//5
addi t1, t0, -10	//-5
addi t2, t1, 3		//-2
add t3, t0, t1		//0
sub t4, t1, t2		//-3
sll t0, t0, t0		//160
srl t6, t2, t4		//7
sra t5, t4, t3		//-3
or t4, t2, t2		//-2
and t3, t5, t6		//5
xor t2, t4, t1		//5
slt t5, t3, t2		
sltu t5, t2, t3
nop
