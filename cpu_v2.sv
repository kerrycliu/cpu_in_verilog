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
		if (mem_branch_out_pcsrc == 1) pc <= add_sum_mux;
		else begin
			if (stall == 0) begin
				pc <= pc + 4;
			end
		end
	end
  end

  // ----- stall ----- checks if rd1 or rd2 has the same address as previous rd
  always@(posedge clk or negedge rst_n) begin
  	if ((ID_EX_IMM_WB != 0) or (EX_MEM_IMM_WB != 0)) begin
		if ((decode_rd1 == (ID_EX_IMM_WB or EX_MEM_IMM_EB)) or (decode_rd2 == (ID_EX_IMM_WB or EX_MEM_IMM_WB))) begin
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
			ID_EX_RD1 <= decode_rd1;
			ID_EX_RD2 <= decode_rd2;
			ID_EX_IMM <= decode_immediate;
			ALU_CTRL_IN <= decode_func7_func3;
			ID_EX_IMM_WB <= decode_instr_117;
		end
		else begin
			ID_EX_IMM_WB <= 0;
		end
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

	reg [31:0] stlu_calc;
	parameter I_TYPE_SIGNAL = 2'00, R_TYPE = 2'b10;

	case(alu_op_wire) // ID_EX_EX_CTRL[2:1]
		I_TYPE_SINGAL: begin
			case (execute_alu_control_in[2:0]) // func3
				3'h0: begin
					execute_alu_result = execute_alu_rd1 + execute_mux_out; // addi 
				end
				3'h4: begin
					execute_alu_result = execute_alu_rd1 ^ execute_mux_out; // xori
				end
				3'h6: begin
					execute_alu_result = execute_alu_rd1 | execute_mux_out; // ori
				end
				3'h7: begin
					execute_alu_result = execute_alu_rd1 & execute_mux_out; // andi
				end
				3'h1: begin
					execute_alu_result = execute_alu_rd1 << execute_mux_out[4:0]; // slli 
				end
				3'h5: begin
					if (execute_alu_control_in[3] == 0) begin
						execute_alu_result = execute_alu_rd1 >> execute_mux_out[4:0]; // srli
					end
					else if (execute_alu_control_in[3] == 1) begin
						execute_alu_result = execute_alu_rd1 >>> execute_mux_out[4:0]; //srai
					end
				end
				3'h2: begin
					execute_alu_result = (execute_alu_rd1 < execute_mux_out)?1:0;
				end
				3'h3: begin
					stlu_calc = execute_alu_rd1 - execute_mux_out;
					execute_alu_result = {31'b0, stlu_calc[31]};
				end
			endcase
		R_TYPE : begin
			case(execute_alu_control_in)
			// continue
			endcase
		end
	endcase
	// Zero signal

  end

					


			
		



