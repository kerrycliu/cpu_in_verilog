`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company:
// Engineer:
//
// Create Date: 09/25/2023 12:22:07 PM
// Design Name:
// Module Name: cpu
// Project Name:
// Target Devices:
// Tool Versions:
// Description:
//
// Dependencies:
//
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
//
//////////////////////////////////////////////////////////////////////////////////
module cpu(
    input rst_n,
    input clk,
    output reg [31:0] imem_addr,
    input [31:0] imem_insn,
    output reg [31:0] dmem_addr,
    output reg [31:0] dmem_data,
    output reg dmem_wen
    );  
  reg [31:0] pc; 
  reg [15:0] clock_counter; 
  reg [31:0] regfile [31:0];
  reg [0:0] stall;

  // ----- Fetch -----
  reg [31:0] fetch_imem_insn;
  reg [31:0] INSTRUCTION;
  
  reg [31:0]  IF_ID_REG; // pipeline register takes in pc
  
  // ----- Decode -----
  reg [1:0] decode_wb_control;
  reg [2:0] decode_ex_control;
  reg [2:0] decode_m_control;
  reg [1:0] ID_EX_WB_CTRL;
  reg [2:0] ID_EX_M_CTRL;
  reg [2:0] ID_EX_EX_CTRL;
 
  reg [31:0] decode_if_id_out; //goes into IF_ID_REG
  reg [31:0] ID_EX_REG; // pipeline takes in IF_ID_REG

  reg [4:0] decode_rd1;
  reg [4:0] decode_rd2;
  reg [31:0] decode_rd1_ext;
  reg [31:0] decode_rd2_ext;
  reg [31:0] ID_EX_RD1;
  reg [31:0] ID_EX_RD2
  reg [31:0] decode_immediate;
  reg [31:0] ID_EX_IMM;
  reg [3:0] decode_func7_func3;
  reg [3:0] ALU_CTRL_IN;
  reg [4:0] decode_instr_117;
  reg [4:0] ID_EX_IMM_WB;
  
  // ----- Execution -----
  reg [1:0] execute_wb_control;
  reg [1:0] EX_MEM_WB_CTRL;
  reg [2:0] execute_m_control;
  reg [2:0] EX_MEM_M_CTRL;

  reg [1:0] alu_op_wire; // goes into ALU_CTRL_IN ln:56
  reg alu_src; // signal to exe mux goes into ALU
  reg [4:0] execute_imm_wb; // output wire from decode_instr_117, input to EX_MEM reg
  reg [4:0] EX_MEM_IMM_WB;

  // Add Sum
  reg [31:0] execute_add_sum_in;
  reg [31:0] execute_immediate_mux_sl1;
  reg [31:0] execute_sl1_out; //goes into Add Sum
  reg [31:0] execute_add_sum_out; // goes into EX/MEM reg
  reg [31:0] EX_MEM_ADD_SUM;

  // ALU
  reg [31:0] execute_alu_rd1;
  reg [31:0] execute_mux_rd2; // goes into mux as execute_immediate_mux_sl1  ln:71
  reg [31:0] EX_MEM_RD2;
  reg [31:0] execute_mux_out; // goes into ALU
  reg [31:0] execute_alu_result; // output of ALU
  reg [31:0] EX_MEM_ALU_RESULT;
  reg zero; // zero wire - branch for later
  reg EX_MEM_ZERO;

  // ALU Control
  reg [3:0] execute_alu_control_in;
  reg [3:0] execute_alu_control_out;
  
  // ----- Memory Access -----
  reg [1:0] mem_wb_control;
  reg [1:0] MEM_WB_WB_CTRL;

  reg mem_m_branch_in; //goes into branch adder
  reg mem_m_mem_read;
  reg mem_m_mem_write;
  
  reg mem_branch_zero_in; // zero wire from EX_MEM_ZERO goes into branch adder.
  reg mem_branch_out_pcsrc;
  
  reg [31:0] add_sum_mux; // goes into mux before pc in fetch stage.

  reg[31:0] mem_alu_result; // goes into address input for data memory block
  reg [31:0] MEM_WB_ALU_RESULT;
  reg [31:0] mem_rd2_write_data; // goes into write data input in data mem block
  reg [31:0] mem_read_data_out; // output for data memory block
  reg [31:0] MEM_WB_READ_DATA;

  reg [4:0] mem_imm_wb; // wire from inst_117
  reg [4:0] MEM_WB_IMM_WB;

  // ----- Write Back -----
  reg wb_reg_write_control;
  reg wb_mem_to_reg_control; // wire goes into mux
  reg [31:0] wb_read_data_mux_in;
  reg [31:0] wb_alu_result_mux_in;
  reg [31:0] wb_write_data_mux_out;

  reg [4:0] wb_write_register_in; // from inst 117, goes into write reg in decode reg


  // ----- clk -----
  always@(posedge clk or negedge rst_n) begin
  	if (!rst_n) clock_counter <= 16'b0;
	else clock_counter <= clock_counter + 1'b1;
  end

  // ----- pc -----
  always@(posedge clk or negedge rst_n) begin
  	if (!rst_n) pc <= 32'b0;
	else begin
		if (mem_branch_out_pcsrc == 1) begin // check the branch adder output in MEM stage
			mux_out_pc_in = add_sum_mux;
			pc <= mux_out_pc_in; // get pc value 
		end
		else begin
			if (stall == 0) begin
				pc <= pc + 4;
			end
		end
	end
  end

  // ----- stall ----- checks if rd1 or rd2 has the same address as previous rd
  always@(posedge clk or negedge rst_n) begin
  	if ((ID_EX_IMM_WB != 0) or (EX_MEM_IMM_WB != 0)) begin // if there is rd
		if ((decode_rd1 == (ID_EX_IMM_WB or EX_MEM_IMM_EB)) or (decode_rd2 == (ID_EX_IMM_WB or EX_MEM_IMM_WB))) begin // if rs1 or rs2 is either of the rd result, set stall to 1
			stall = 1;
		end
		else begin 
			stall = 0;
		end
	end
  end
  
  // ----- fetch stage -----
  always@(*) begin
  	imm_addr = pc;
	fetch_imem_insn = imem_insn;
  end

  always@(posedge clk or negedge rst_n) begin
  	if (!rst_n) begin
		IF_ID_REG <= 32'b0;
		INSTRUCTION <= 32'b0;
	end
	else begin
		if (stall == 0) begin
			IF_ID_REG <= pc;
			INSTRUCTION <= fetch_imem_insn;
		end
	end
  end

  // ----- Decode Stage -----
  // breaking down the INSTRUCTION aka instructions
  /*
  WB CTRL:  WB[1] = wb_reg_write_control  // Set reg write sig in ID reg
  		1: write to register in ID stage
		0:
            WB[0] = wb_mem_to_reg_control // does into mux, set write data reg value in ID stage
  M CTRL: M[2] = mem_m_branch_in // input to branch AND gate
  		1: signal to mux determines the WriteData (wb_read_data_in or wb_alu_result_mux_in)
		0:
  	  M[1] = mem_m_mem_read // dmem MemRead signal
	  	1:
		0:
	  M[0] = mem_m_mem_write // dmem MemWrite signal
	  	1: MemWrite = mem_rd2_write_data xxxxxxxxxxxxxxxxxxx
  EX CTRL: EX[2:1] = alu_op // goes into ALU CONTROL, determine instruction TYPE
  		I-type: 00
		R-Type: 10
		S-Type: 01
		other : 11 (Jump, Branch, Load)
	   EX[0] = alu_src // signal to mux, determines execute_mux_rd2 value (execute_rd2 or execute_immediate_mux_sl1)
	   	1: execute_immediate_mux_sl1
		0: execute_alu_rd2
  */
  parameter I_TYPE = 7'b0010011, R_TYPE = 7'b0110011, S_TYPE = 7'b0100011, B_TYPE = 7'b1100011, U_TYPE = 7'b0110111, J_TYPE = 7'b1101111;
  always@(*) begin
  	case (INSTRUCTION[6:0])
		I_TYPE: begin
			decode_wb_control = 2'b10; // write back (arith or logical inst)
			decode_m_control = 3'b000; // no mem op (arith or logical inst)
			decode_ex_control = 3'b001; // ALU op (arith or logical inst)
			decode_immediate = {{20{INSTRUCTION[31]}}, INSTRUCTION[31:20]};
		end
		R_TYPE: begin
			decode_wb_control = 2'b10; // write back
			decode_m_control = 3'b000; // no mem op
			decode_ex_control = 3'b100; // ALU op
		end
		S_TYPE: begin
			decode_m_control = 3'b010; // mem write (store)
			decode_ex_control = 3'b010; // alu_op
			decode_immediate = {{20{INSTRUCTION[31]}}, INSTRUCTION[31:25], INSTRUCTION[11:7]};
		end
		B_TYPE: begin
			decode_wb_control = 2'00;
			decode_immediate = {{19{INSTRUCTION[31]}}, INSTRUCTION[31], INSTRUCTION[7], INSTRUCTION[30:25], INSTRUCTION[11:8], 1'b0};
		end
		U_TYPE: begin
			decode_immediate = {INSTRUCTION[31:12], 12'b0};
		end
		J_TYPE: begin
			decode_immediate = {{11{INSTRUCTION[31]}}, INSTRUCTION[31], INSTRUCTION[19:12], INSTRUCTION[20], INSTRUCTION[30:21], 1'b0};
		default : begin
			decode_wb_control = 2'b00; // no write back (branch or jump)
			decode_m_control = 3'b000;
			decode_ex_control = 3'b000;
			decode_immediate = 32'0;
		end
	endcase
 	//pass pipeline wire
	decode_if_id_out = IF_ID_REG;
	// parsing instructions, get rs1 and rs2
	decode_rd1 = INSTRUCTION[19:15]; // rs1
	decode_rd2 = INSTRUCTION[24:20]; // rs2
	// making regfile
	decode_rd1_ext = regfile[decode_rd1];
	decode_rd2_ext = regfile[decode_rd2];
	
	// decode_func7_func3: instr[30] is the determine bit for func7, inst[14:12] is func3
	decode_func7_func3 = {INSTRUCTION[30],INSTRUCTION[14:12]}; 
	
	decode_instr_117 = INSTRUCTION[11:7]; // rd
  end
  always@(posedge clk or negedge rst_n) begin
	if (!rst_n) begin
		ID_EX_WB_CTRL <= 2'b0;
		ID_EX_M_CTRL <= 3'b0;
		ID_EX_EX_CTRL <= 3'b0;
		ID_EX_REG <= 32'b0;
		ID_EX_RD1 <= 32'b0;
		ID_EX_RD2 <= 32'b0;
		ID_EX_IMM <= 32'b0;
		ALU_CTRL_IN <= 4'b0;
		ID_EX_IMM_WB <= 5'b0;
	end
	else begin
		if (stall == 0) begin
			ID_EX_WB_CTRL <= decode_wb_control;
			ID_EX_M_CTRL <= decode_m_control;
			ID_EX_EX_CTRL <= decode_ex_control;
			ID_EX_REG <= decode_if_id_out;
			ID_EX_RD1 <= decode_rd1_ext;
			ID_EX_RD2 <= decode_rd2_ext;
			ID_EX_IMM <= decode_immediate;
			ALU_CTRL_IN <= decode_func7_func3;
			ID_EX_IMM_WB <= decode_instr_117;
		end
		else begin
			ID_EX_IMM_WB <= 0;
		end
	end
  end
  always@(negedge clk) begin
  	if(wb_reg_write_control) begin
		regfile[wb_write_register_in] = wb_write_data_mux_out;
	end
  end

  // ----- Execute Stage -----
  always@(*) begin
  	execute_wb_control = ID_EX_WB_CTRL;
	execute_m_control = ID_EX_M_CTRL;
	alu_op_wire =ID_EX_EX_CTRL[2:1];
	alu_src = ID_EX_EX_CTRL[0];
	
	// add sum
	execute_add_sum_in = ID_EX_REG;
	execute_immediate_mux_sl1 = ID_EX_IMM;
	execute_sl1_out = execute_immediate_mux_sl1 << 1;
	execute_add_sum_out = execute_add_sum_in + execute_sl1_out;
	EX_MEM_ADD_SUM = execute_add_sum_out;

	// ALU Control
	execute_alu_rd1 = ID_EX_RD1;
	execute_alu_rd2 = ID_EX_RD2;
	if (alu_scr) begin
		execute_mux_out = execute_immediate_mux_sl1;
	end
	else begin
		execute_mux_out = execute_alu_rd2;
	end

	execute_alu_control_in = ALU_CTRL_IN;
	/*
	ALU Control
	0000	ADD
	0001	SUB
	0010	SLT
	0011	SLTU
	0110	XOR
	0111	OR
	1000	AND
	0100	SLL
	0101	SRL
	1001	SRA
	*/

	reg [31:0] stlu_calc; // signed stlu var
	parameter I_TYPE_SIGNAL = 2'00, R_TYPE = 2'b10, S_TYPE = 2'b01;

	case(alu_op_wire) // ID_EX_EX_CTRL[2:1]
		I_TYPE_SINGAL: begin
			case (execute_alu_control_in[2:0]) // func3
				3'h0: begin
					execute_alu_control_out = 4'b0000; //ADDI
				end
				3'h4: begin
					execute_alu_control_out = 4'b0110; //XORI
				end
				3'h6: begin 
					execute_alu_control_out = 4'b0111; //ORI
				end
				3'h7: begin
					execute_alu_control_out = 4'b1000; //ANDI
				end
				3'h1: begin
					execute_alu_control_out = 4'b0100; //SLLI
				end
				3'h5: begin
					if (execute_alu_control_in[3] == 0) begin // imm[5:11]=0x00 -> srli
						execute_alu_control_out = 4'b0101; //SRLI
					end
					else if (execute_alu_control_in[3] == 1) begin // imm[5:11]=0x20 -> srai
						execute_alu_control_out = 4'b1001; //SRAI
					end
				end
				3'h2: begin
					execute_alu_control_out = 4'0010; //SLTI
				end
				3'h3: begin
					execute_alu_control_out = 4'b0011; //SLTIU
				end
			endcase
		end
		R_TYPE : begin
			case(execute_alu_control_in) // both func7func3
				4'b0000: begin
					execute_alu_control_out = 4'b0000; //ADD
				end
				4'b1000: begin
					execute_alu_control_out = 4'b0001; //SUB
				end
				4'b0100: begin
					execute_alu_control_out = 4'b0110; //XOR
				end
				4'b0110: begin
					execute_alu_control_out = 4'b0111; //OR
				end
				4'b0111: begin
					execute_alu_control_out = 4'b1000; //AND
				end
				4'b0001: begin
					execute_alu_control_out = 4'b0100; //SLL
				end
				4'b0101: begin
					execute_alu_control_out = 4'b0101; //SRL
				end
				4'b1101: begin
					execute_alu_control_out = 4'b1001; //SRA
				end
				4'b0010: begin
					execute_alu_control_out = 4'b0010; //SLT
				end
				4'b0011: begin
					execute_alu_control_out = 4'b0011; //SLTU
				end
			endcase
		end
		// TODO: LAB6 begin here
		/*
		S_TYPE: begin
			case()
			endcase
		end
		*/
	endcase
	// ALU
	parameter ADD = 4'b0000, SUB = 4'b0001, SLT = 4'b0010, SLTU = 4'b0011, XOR = 4'b0110, OR = 4'b0111, AND = 4'b1000, SRL = 4'b0101, SRA = 4'b1001;
	case (execute_alu_control_out)
		ADD: begin
			execute_alu_result = execute_alu_rd1 + execute_mux_out;
		end
		SUB: begin
			execute_alu_result = execute_alu_rd1 - execute_mux_out;
		end
		SLT: begin
			if(execute_alu_rd1 < execute_mux_out) begin
				execute_alu_result = 1;
			end
			else begin
				execute_alu_result = 0;
			end
		end
		SLTU: begin
			sltu_calc = execute_alu_rd1 - execute_mux_out;
			execute_alu_result = {31'b0, sltu_calc[31]};
		end
		XOR: begin
			execute_alu_result = execute_alu_rd1 ^ execute_mux_out;
		end
		OR: begin
			execute_alu_result = execute_alu_rd1 | execute_mux_out;
		end
		AND: begin
			execute_alu_result = execute_alu_rd1 & execute_mux_out;
		end
		SLL: begin
			execute_alu_result = execute_alu_rd1 << execute_mux_out[4:0];
		end
		SRL: begin
			execute_alu_result = execute_alu_rd1 >> execute_mux_out[4:0];
		end
		SRA: begin
			execute_alu_result = execute_alu_rd1 >>> execute_mux_out[4:0];
		end
	endcase
	
	if(execute_alu_result == 0) begin
		// set zero flag
		zero = 1;
	end
	else begin
		zero = 0;
	end

	execute_imm_wb = ID_EX_IMM_WB;
  end

  always@(posedge clk or negedge rst_n) begin
  	if (!rst_n) begin
		EX_MEM_WB_CTRL <= 2'b0;
		EX_MEM_M_CTRL <= 3'b0;
		EX_MEM_IMM_WB <= 5'b0;
		EX_MEM_ADD_SUM <= 32'b0;
		EX_MEM_RD2 <= 32'b0;
		EX_MEM_ALU_RESULT <= 32'b0;
		EX_MEM_ZERO <= 0;
	end
	else begin
		EX_MEM_WB_CTRL <= execute_wb_control;
		EX_MEM_M_CTRL <= execute_m_control;
		EX_MEM_IMM_WB <= execute_imm_wb;
		EX_MEM_ADD_SUM <= execute_add_sum_out;
		EX_MEM_RD2 <= execute_mux_rd2;
		EX_MEM_ALU_RESULT <= execute_alu_result;
		EX_MEM_ZERO <= zero;
	end
  end

  // ----- Memory Access Stage -----
  always@(*) begin
  	mem_wb_control = EX_MEM_WB_CTRL;
	mem_m_branch_in = EX_MEM_M_CTRL[2];
	mem_m_mem_read = EX_MEM_M_CTRL[1];
	mem_m_mem_write = EX_MEM_M_CTRL[0];
	mem_branch_zero_in = EX_MEM_ZERO;
	mem_branch_out_pcsrc = mem_branch_in + mem_branch_zero_in; // branch adder result goes into mux in fetch
	mem_alu_result = EX_MEM_ALU_RESULT;
	add_sum_mux = EX_MEM_ADD_SUM; // goes into the same mux as mem_branch_out_pcsrc
	mem_rd2_write_data = EX_MEM_RD2;
	mem_imm_wb = EX_MEM_IMM_WB;
	dmem_addr = mem_alu_result;
	dmem_wen = mem_m_mem_write; // control sig
	mem_read_data_out = dmem_data;
  end
  
  if(mem_m_mem_write) begin
  	mem_rd_write_data = 32'bx;
  end

  always@(posedge clk or negedge rst_n) begin
   	if (!rst_n) begin
		MEM_WB_WB_CTRL <= 2'b0;
		MEM_WB_ALU_RESULT <= 32'b0;
		MEM_WB_READ_DATA <= 32'b0;
		MEM_WB_IMM_WB <= 5'b0;
	end
	else begin
		MEM_WB_WB_CTRL <= mem_wb_control;
		MEM_WB_ALU_RESULT <= mem_alu_result;
		MEM_WB_READ_DATA <= mem_read_data_out;
		MEM_WB_IMM_WB <= mem_imm_wb;
	end
  end

  // ----- Write Back Stage -----
  always@(*) begin
  	wb_reg_write_control = MEM_WB_WB_CTRL[1];
	wb_mem_to_reg_control = MEM_WB_WB_CTRL[0];

	wb_read_data_mux_in = MEM_WB_READ_DATA;
	wb_alu_result_mux_in = MEM_WB_ALU_RESULT;
	
	if(wb_mem_to_reg_control == 1) begin
		wb_write_data_mux_out = wb_read_data_mux_in;
	end
	else begin
		wb_write_data_mux_out = wb_alu_result_mux_in;
	end

	wb_write_register_in = MEM_WB_IMM_WB;
  end
endmodule


			
		



