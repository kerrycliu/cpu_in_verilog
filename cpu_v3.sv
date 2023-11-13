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

module cpu (rst_n, clk, imem_addr, imem_insn, dmem_addr, dmem_data, dmem_wen);

input rst_n, clk;
input reg [31:0] imem_insn;
output reg [31:0] imem_addr, dmem_addr;
inout [31:0] dmem_data;
output reg dmem_wen;

reg [15:0] clock_counter; // Clock Cycle Counter

reg [31:0] regfile [31:0]; // Register File Creation
reg stall; // Stall indicator

// ----- FETCH ----- 
reg [31:0] IF_ID_REG; // pipeline reg takes in pc value
reg [31:0] pc; 
reg [31:0] INSTRUCTION; // reg takes the instruction pass to decode stage
reg [31:0] fetch_imem_insn;

// ----- DECODE -----
reg [1:0] decode_wb_control; 
reg [1:0] ID_EX_WB_CTRL;
reg [2:0] decode_m_control;
reg [2:0] ID_EX_M_CTRL;
reg [2:0] decode_ex_control;
reg [2:0] ID_EX_EX_CTRL;

reg [31:0] decode_if_id_out; // from IF_ID_REG goes into IF_ID_REG
reg [31:0] ID_EX_REG;
reg [4:0] decode_rd1; // READ DATA 1  - rs1
reg [31:0] ID_EX_RD1;
reg [4:0] decode_rd2; // READ DATA 2  - rs2
reg [31:0] ID_EX_RD2;
reg [31:0] decode_rd1_ext;
reg [31:0] decode_rd2_ext;
reg [31:0] decode_immediate;
reg [31:0] ID_EX_IMM;
reg [3:0] decode_func7_func3;
reg [3:0] ALU_CTRL_IN;
reg [4:0] decode_instr_117; // pass all the way to WB stage - rd
reg [4:0] ID_EX_IMM_WB;

// ----- EXECUTE -----
reg [1:0] execute_wb_control;
reg [1:0] EX_MEM_WB_CTRL;
reg [2:0] execute_m_control;
reg [2:0] EX_MEM_M_CTRL;

reg [1:0] alu_op_wire; // EX_CONTROL [2:1] goes into ALU_CTRL_IN
reg alu_src_wire; //signal to mux goes into ALU

// Add Sum
reg [31:0] execute_add_sum_in; // from ID_EX_REG goes into AddSum
reg [31:0] execute_immediate_mux_sl1; //from ID_EX_IMM wire that goes into both shiftleft1 and mux
reg [31:0] execute_sl1_out; // output of shiftleft1 goes into AddSum
reg [31:0] execute_add_sum_out; // output of AddSum, goes into EX_MEM_ADD_SUM
reg [31:0] EX_MEM_ADD_SUM;

// ALU
reg [31:0] execute_alu_rd1; //from ID_EX_RD1, goes into ALU
reg [31:0] execute_mux_rd2; //from ID_EX_RD2, goes into mux before ALU
reg [31:0] EX_MEM_RD2; // takes execute_mux_rd2 value
reg [31:0] execute_mux_out; // output of mux, goes into alu
reg [0:0] zero_wire; // zero flag from ALU
reg [0:0] EX_MEM_ZERO;
reg [31:0] execute_alu_result; // result of ALU
reg [31:0] EX_MEM_ALU_RESULT;

// ALU SLT Variables
reg signed [31:0] slt_calc_in1_signed;
reg [31:0] slt_calc_in1_unsigned;
reg signed [31:0] slt_calc_in2_signed;
reg [31:0] slt_calc_in2_unsigned;
reg signed [31:0] slt_calc_result_signed;
reg [31:0] slt_calc_result_unsigned;

//ALU Control
reg [3:0] execute_alu_control_in;
reg [3:0] execute_alu_control_out;

reg [4:0] execute_imm_wb;
reg [4:0] EX_MEM_IMM_WB;

//----- MEMORY ACCESS -----
reg [1:0] mem_wb_control;
reg [1:0] MEM_WB_WB_CTRL;
reg mem_m_mem_read;  // signal for data memory block
reg mem_m_mem_write; // signal for data memory block

reg [31:0] add_sum_mux;

reg mem_branch_zero_in; // from EX_MEM_ZERO
reg mem_m_branch_in;   // input for branch AND
reg mem_branch_out_pcsrc; // output for BRANCH AND back to mux in FETCH stage


// Data Memory Block
reg [31:0] mem_alu_result; // from EX_MEM_ALU_RESULT goes into dmem_addr
reg [31:0] MEM_WB_ALU_RESULT;
reg [31:0] mem_rd2_write_data; // from EX_MEM_RD2 goes into dmem_write_data
reg [31:0] mem_read_data_out; // output of data memory block
reg [31:0] MEM_WB_READ_DATA; // takes in mem_read_data_out

reg [4:0] mem_imm_wb;
reg [4:0] MEM_WB_IMM_WB;

// ----- WRITE BACK -----
reg wb_reg_write_control; // RegWrite signal back to decode Register
reg wb_mem_to_reg_control; // MemToReg signal to mux, determines WriteData in decode Register

reg [31:0] wb_read_data_mux_in; // from MEM_WB_READ_DATA
reg [31:0] wb_alu_result; // from MEM_WB_ALU_RESULT
reg [31:0] wb_write_data_mux_out; // output of mux

reg [4:0] wb_write_register_in; // goes back to WriteRegister in decode Register

// ----- PARAMETERS -----
parameter I_TYPE = 7'b0010011;
parameter R_TYPE = 7'b0110011;
parameter S_TYPE = 7'b0100011;
parameter B_TYPE = 7'b1100011;
parameter U_TYPE = 7'b0110111;
parameter J_TYPE = 7'b1101111;

// ----- CLOCK COUNTER AND PC -----
always @(posedge clk or negedge rst_n) begin
    if (~rst_n) begin
        pc <= 32'b0;
        clock_counter <= 16'b0;
    end
    else begin
        if (mem_branch_out_pcsrc == 1) begin
            pc = add_sum_mux;
        end
        else begin
            if (stall == 0) begin
                pc <= pc + 3'b100;
            end
        end
        clock_counter <= clock_counter + 1'b1;
    end
end

// ----- STALL -----
/*
Check rd with read data 1 and 2 in each stage. If same, then stall, else stall=0
*/
always @ * begin
    if (ID_EX_IMM_WB != 0) begin
        if (decode_rd1 == ID_EX_IMM_WB) begin
            stall = 1;
        end
        else if (decode_rd2 == ID_EX_IMM_WB) begin
            stall = 1;
        end
        else begin
            stall = 0;
        end
    end
    if (EX_MEM_IMM_WB != 0) begin
        if (decode_rd1 == EX_MEM_IMM_WB) begin
            stall = 1;
        end
        else if (decode_rd2 == EX_MEM_IMM_WB) begin
            stall = 1;
        end
        else begin
            stall = 0;
        end
    end
    if (ID_EX_IMM_WB == 0 && EX_MEM_IMM_WB == 0) begin
        stall = 0;
    end
end

// ----- FETCH STAGE -----
always @ (*) begin
    imem_addr = pc;
    fetch_imem_insn = imem_insn;
end

always @ (posedge clk or negedge rst_n) begin
    if (~rst_n) begin
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

// ----- DECODE STAGE -----
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
always @ (*) begin
    // Decoding the opcode and generating the appropriate control signals
    // WB signals are: Regwrite, Memtoreg
    // M signals are: Branch, Memread, Memwrite
    // EX Signals are: ALUOp[1:0], ALUSrc
    case (INSTRUCTION[6:0])
        I_TYPE : begin // I-Type
            decode_wb_control = 2'b10;
            decode_m_control = 3'b000;
            decode_ex_control = 3'b001;
        end 
        R_TYPE : begin  // R-Type
            decode_wb_control = 2'b10;
            decode_m_control = 3'b000;
            decode_ex_control = 3'b100;            
        end
        default : begin
            decode_wb_control = 2'b00;
            decode_m_control = 3'b000;
            decode_ex_control = 3'b000;
        end 
    endcase

    decode_if_id_out = IF_ID_REG;

    decode_rd1 = INSTRUCTION[19:15];
    decode_rd2 = INSTRUCTION[24:20];

    // Set register x0
    regfile[5'b0] = 32'b0;
    // Read data from register file
    decode_rd1_ext = regfile[decode_rd1];
    decode_rd2_ext = regfile[decode_rd2];

    case (INSTRUCTION[6:0])
        I_TYPE : begin
            // Immediate
            decode_immediate = { {20{INSTRUCTION[31]}}, INSTRUCTION[31:20] };
        end
        S_TYPE : begin
            // Store
            decode_immediate = { {20{INSTRUCTION[31]}}, INSTRUCTION[31:25], INSTRUCTION[11:7] };
        end
        B_TYPE : begin
            // Branch
            decode_immediate = { {19{INSTRUCTION[31]}}, INSTRUCTION[31], INSTRUCTION[7], INSTRUCTION[30:25], INSTRUCTION[11:8], 1'b0 };
        end
        U_TYPE : begin
            // Upper Immediate
            decode_immediate = { INSTRUCTION[31:12], 12'b0 };
        end
        J_TYPE : begin
            // Jump
            decode_immediate = { {11{INSTRUCTION[31]}}, INSTRUCTION[31], INSTRUCTION[19:12], INSTRUCTION[20], INSTRUCTION[30:21], 1'b0 };
        end
        default: decode_immediate = 32'b0;
    endcase

    // Other wires
    decode_func7_func3 = {INSTRUCTION[30], INSTRUCTION[14:12]};
    decode_instr_117 = INSTRUCTION[11:7];
end

always @ (negedge clk) begin
    // Write data to register file
    if (wb_reg_write_control) begin
        regfile[wb_write_register_in] = wb_write_data_mux_out;
    end
end

always @ (posedge clk or negedge rst_n) begin
    if (~rst_n) begin
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
            // Control Signals
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


// ----- EXECUTE STAGE -----
always @ (*) begin
    execute_wb_control = ID_EX_WB_CTRL;
    execute_m_control = ID_EX_M_CTRL;
    alu_op_wire = ID_EX_EX_CTRL[2:1];
    alu_src_wire = ID_EX_EX_CTRL[0];

    execute_add_sum_in = ID_EX_REG;
    execute_immediate_mux_sl1 = ID_EX_IMM;
    execute_sl1_out = execute_immediate_mux_sl1 << 1;
    execute_add_sum_out = execute_add_sum_in + execute_sl1_out;

    execute_alu_rd1 = ID_EX_RD1;
    execute_mux_rd2 = ID_EX_RD2;
    if (alu_src_wire == 1) begin
        execute_alu_result = execute_immediate_mux_sl1;
    end
    else begin
        execute_alu_result = execute_mux_rd2;
    end

    execute_alu_control_in = ALU_CTRL_IN;

    // ALU Control Calculation
    case (alu_op_wire)
        2'b00 : begin // I-Type instruction
            case (execute_alu_control_in[2:0])
                3'b000: begin
                    execute_alu_control_out = 4'b0000; // I-Type ADD
                end
                3'b010: begin
                    execute_alu_control_out = 4'b1000; // I-Type SLTI
                end
                3'b011: begin
                    execute_alu_control_out = 4'b1001; // I-Type SLTIU
                end
                3'b100: begin
                    execute_alu_control_out = 4'b0100; // I-Type XORI
                end
                3'b110: begin
                    execute_alu_control_out = 4'b0011; // I-Type ORI
                end
                3'b111: begin
                    execute_alu_control_out = 4'b0010; // I-Type ANDI
                end
                3'b001: begin
                    if (execute_alu_control_in[3] == 0) begin
                        execute_alu_control_out = 4'b0101; // I-Type SLLI
                    end
                end
                3'b101: begin
                    if (execute_alu_control_in[3] == 0) begin
                        execute_alu_control_out = 4'b0110; // I-Type SRLI
                    end
                    else if (execute_alu_control_in[3] == 1) begin
                        execute_alu_control_out = 4'b0111; // I-Type SRAI
                    end
                end
            endcase
        end
        2'b01 : begin // Sets to subtract and test if zero for beq
            execute_alu_control_out = 4'b0001;
        end
        2'b10 : begin // Sets depending on funct7 and funct3 fields
            case (execute_alu_control_in) // R-Type Instruction Decoding
                4'b0000 : begin
                    execute_alu_control_out = 4'b0000; // R-Type ADD
                end
                4'b1000 : begin
                    execute_alu_control_out = 4'b0001; // R-Type Subtract
                end
                4'b0001 : begin
                    execute_alu_control_out = 4'b0101; // R-Type SLL
                end
                4'b0010 : begin
                    execute_alu_control_out = 4'b1000; // R-Type SLT
                end
                4'b0011 : begin
                    execute_alu_control_out = 4'b1001; // R-Type SLTU
                end
                4'b0100 : begin
                    execute_alu_control_out = 4'b0100; // R-Type XOR
                end
                4'b0101 : begin
                    execute_alu_control_out = 4'b0110; // R-Type SRL
                end
                4'b1101 : begin
                    execute_alu_control_out = 4'b0111; // R-Type SRA
                end
                4'b0110 : begin
                    execute_alu_control_out = 4'b0011; // R-Type OR
                end
                4'b0111 : begin
                    execute_alu_control_out = 4'b0010; // R-Type AND
                end   
            endcase
        end
    endcase

    // ALU Calculation
    case (execute_alu_control_out)
        4'b0000 : begin // Add Function
            execute_alu_result = execute_alu_rd1 + execute_alu_result;
        end
        4'b0001 : begin // Subtract Function
            execute_alu_result = execute_alu_rd1 - execute_alu_result;
        end
        4'b0010 : begin // AND Function
            execute_alu_result = execute_alu_rd1 & execute_alu_result;
        end
        4'b0011 : begin // OR Function
            execute_alu_result = execute_alu_rd1 |  execute_alu_result;
        end
        4'b0100 : begin // XOR Function
            execute_alu_result = execute_alu_rd1 ^ execute_alu_result;
        end
        4'b0101: begin // SLL Function
            execute_alu_result = execute_alu_rd1 << execute_alu_result[4:0];
        end
        4'b0110: begin // SRL Function
            execute_alu_result = execute_alu_rd1 >> execute_alu_result[4:0];
        end
        4'b0111: begin // SRA Function
            execute_alu_result = $signed(execute_alu_rd1) >>> execute_alu_result[4:0];
        end
        4'b1000: begin // SLT Function
            slt_calc_in1_signed = execute_alu_rd1;
            slt_calc_in2_signed = execute_alu_result;
            slt_calc_result_signed = slt_calc_in1_signed - slt_calc_in2_signed;
            execute_alu_result = {31'b0, slt_calc_result_signed[31]};
        end
        4'b1001: begin // SLTU Function
            slt_calc_in1_unsigned = execute_alu_rd1;
            slt_calc_in2_unsigned = execute_alu_result;
            slt_calc_result_unsigned = slt_calc_in1_unsigned - slt_calc_in2_unsigned;   // compare as unsigned
            execute_alu_result = {31'b0, slt_calc_result_unsigned[31]};
        end
    endcase

    // Zero signal calculation
    if (execute_alu_result == 4'b0) begin
        zero_wire = 1;
    end
    else begin
        zero_wire = 0;
    end

    execute_imm_wb = ID_EX_IMM_WB;
end

//-------------------------- EX/MEM Register Logic ? ------------------------------------------------------------------------------------------------
always @ (posedge clk or negedge rst_n) begin
    if (~rst_n) begin
        EX_MEM_WB_CTRL <= 2'b0;
        EX_MEM_M_CTRL <= 3'b0;

        EX_MEM_ADD_SUM <= 32'b0;

        EX_MEM_ZERO <= 1'b0;
        EX_MEM_ALU_RESULT <= 32'b0;

        EX_MEM_RD2 <= 32'b0;
        EX_MEM_IMM_WB <= 5'b0;
    end else begin
        // Control Signals
        EX_MEM_WB_CTRL <= execute_wb_control;
        EX_MEM_M_CTRL <= execute_m_control;

        // Address Sum
        EX_MEM_ADD_SUM <= execute_add_sum_out;

        // ALU
        EX_MEM_ZERO <= zero_wire;
        EX_MEM_ALU_RESULT <= execute_alu_result;

        EX_MEM_RD2 <= execute_mux_rd2;
        EX_MEM_IMM_WB <= execute_imm_wb;
    end
end

//-------------------------- MEM Stage Logic ------------------------------------------------------------------------------------------------
always @ (*) begin
    // Control Signals
    mem_wb_control = EX_MEM_WB_CTRL;
    mem_m_branch_in = EX_MEM_M_CTRL[2];
    mem_m_mem_read = EX_MEM_M_CTRL[1];
    mem_m_mem_write = EX_MEM_M_CTRL[0];

    // Branch Adder that wraps back to first mux in IF stage
    add_sum_mux = EX_MEM_ADD_SUM;

    // And gate for pcsrc
    mem_branch_out_pcsrc = mem_m_branch_in + mem_branch_zero_in;

    // Data Memory Block
    mem_alu_result = EX_MEM_ALU_RESULT;
    mem_rd2_write_data = EX_MEM_RD2;

    // Supply address to external memory
    dmem_addr = mem_alu_result;

    // Supply read/write signal to external memory
    dmem_wen = mem_m_mem_write;

    mem_read_data_out = dmem_data;

    mem_imm_wb = EX_MEM_IMM_WB;
end

// Continuous assignment for tri-state buffer
assign dmem_data = mem_m_mem_write ? mem_rd2_write_data : 32'bz; // if this instruction is a store, then we supply the data to store to the external memory

//-------------------------- MEM/WB Register Logic ? ------------------------------------------------------------------------------------------------
always @ (posedge clk or negedge rst_n) begin
    if (~rst_n) begin
        MEM_WB_WB_CTRL <= 2'b0;

        MEM_WB_READ_DATA <= 32'b0;
        MEM_WB_ALU_RESULT <= 32'b0;

        MEM_WB_IMM_WB <= 5'b0;
    end else begin
        // Control Signals
        MEM_WB_WB_CTRL <= mem_wb_control;

        // Write Back Mux
        MEM_WB_READ_DATA <= mem_read_data_out;
        MEM_WB_ALU_RESULT <= mem_alu_result;
    
        // Write Register
        MEM_WB_IMM_WB <= mem_imm_wb;
    end
end

//-------------------------- Write Back Stage Logic ------------------------------------------------------------------------------------------------
always @ (*) begin
    // Control Signals
    wb_reg_write_control = MEM_WB_WB_CTRL[1];
    wb_mem_to_reg_control = MEM_WB_WB_CTRL[0];

    // Write Back Mux
    wb_read_data_mux_in = MEM_WB_READ_DATA;
    wb_alu_result = MEM_WB_ALU_RESULT;
    wb_write_data_mux_out = wb_mem_to_reg_control ? wb_read_data_mux_in : wb_alu_result;

    wb_write_register_in = MEM_WB_IMM_WB;
end
endmodule