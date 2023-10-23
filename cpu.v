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
  reg fetch_decode_reg, decode_exec_reg, exec_mem_reg, mem_wriback_reg;
  reg [31:0] instruction;
  reg [4:0]rs1;
  reg [4:0] rd,rd_prev;
  reg [2:0] func3;
  reg [6:0] opcode;
  reg [31:0] imm;
  reg signed [31:0] alu_result;
  reg signed [31:0] regfile [31:0]; // each element in the regfile is 32 bits long, and the reg file holds 32 elements (regfile[31:0])
  reg [31:0] dmem;
  reg [15:0] clock_counter;
  reg stall;
  parameter addi = 3'b000, slti = 3'b010, sltiu = 3'b011, xori = 3'b100, ori = 3'b110, andi = 3'b111, slli = 3'b001, srli = 3'b101, srai = 3'b101;
  reg [31:0]calc_result;
  reg signed [31:0]slli_imm, srli_imm, srai_imm;
  integer pc_file, reg_file;
  
 
    initial begin
        pc_file = $fopen("pc_trace.txt", "w");
        reg_file = $fopen("reg_trace.txt", "w");
    end 
    
// 16 bit counter
    always@(posedge clk or negedge rst_n or negedge rst_n)
    begin
        if(!rst_n) clock_counter <= 0;
        else clock_counter <= clock_counter + 1;
    end
    
//Fetch
    always@(posedge clk or negedge rst_n)
    begin
        if(!rst_n)
        begin
            pc <= 32'b0;
            instruction <= 8'hx;
            imem_addr <= 32'b0;
            fetch_decode_reg <= 0;
            stall = 0;
        end
        else
        begin
            if(stall == 1)
            begin
            // everything stay the same in stall
                pc_file = $fopen("pc_trace.txt", "a");
                pc <= pc;
                instruction <= instruction;
                imem_addr <= imem_addr;
                $fdisplay(pc_file, "pc : %h\n", pc);
                $fdisplay(pc_file, "STALLED\n");
                $display("pc : %h\n", pc);
                $display("STALLED\n");
                fetch_decode_reg <= 0;
                
            end
            else
            begin
                pc_file = $fopen("pc_trace.txt", "a");
                pc <= pc + 4;
                instruction <= imem_insn;
                fetch_decode_reg <= 1;
                imem_addr <= pc;
                $fdisplay(pc_file, "pc : %h\n", imem_addr);
                $display("pc : %h\n", imem_addr);
            end
        end
    end
        
// Decode
    always@(posedge clk or negedge rst_n)
    begin
        if(!rst_n)
        begin
            imm <= 32'bx;
            srai_imm <= 32'bx;
            srli_imm <= 32'bx;
            slli_imm <= 32'bx;
            rs1 <= 5'bx;
            rd <= 5'bx;
            func3 <= 3'bx;
            opcode <= 7'bx;
            decode_exec_reg <= 0;
            stall = 0;
        end
        else
        begin
            if(stall == 1)
            begin         
                imm <= imm;
                srai_imm <= srai_imm;
                srli_imm <= srli_imm;
                
                rs1 <= rs1;
                rd <= rd;
                func3 <= func3;
                opcode <= opcode;
                rd_prev <= rd_prev;
                decode_exec_reg <= 0;              
                if(rs1 == rd_prev) begin 
		          stall = 1;
		        end
                else begin 
		          stall = 0; 
		        end
            end           
            else
            begin
                decode_exec_reg <= 1;
                if((instruction != 8'hx) || fetch_decode_reg)
                begin
                    opcode <= instruction[6:0];
                    func3 <= instruction[14:12];
                    imm <= {{20{instruction[31]}},{instruction[31:20]}};
                    srai_imm <= {{27{instruction[24]}},{instruction[24:20]}};
                    rs1 <= instruction[19:15];
                    if(rs1 == rd_prev) begin
		              stall = 1;
		            end
                    else begin 
		              stall = 0;
	                end
                    rd <= instruction[11:7];
                end
            end
            $display("--------------------------");
            $display("CC: %d", clock_counter);
            $display("opcode: %b", opcode);
            $display("rs1: %b", rs1);
            $display("rd: %d", rd);
            $display("func3: %b", func3);
          //$display("stall: %d; decode_exec_reg: %d", stall, decode_exec_reg);
        end
    end
    
    reg [31:0]imm_ext;
// Execute 
    always@(posedge clk or negedge rst_n)
    begin
        if(!rst_n)
        begin
            alu_result <= 32'bx;
            exec_mem_reg <= 0;
        end
        else
        begin
            begin
                exec_mem_reg <= decode_exec_reg;
                rd_prev <= rd;
                if(decode_exec_reg)
                begin
		// I-type instructions 
                    if(regfile[rs1] === 32'bxxxxx)
                    begin //check condition for i-type operatrions
                        // addi
                        if (func3 == addi) begin                          
                            regfile[rd] <= imm + rs1;
                            alu_result <= imm + rs1;
                        end
                        // xori
                        else if (func3 == xori) begin
                            regfile[rd] <= imm ^ rs1;
                            alu_result <= imm ^ rs1;
                        end
                        // ori
                        else if (func3 == ori) begin
                            regfile[rd] <= imm || rs1;
                            alu_result <= imm || rs1;
                        end
                        // andi
                        else if(func3 == andi) begin
                            regfile[rd] <= imm & rs1;
                            alu_result <= imm & rs1;
                        end
                        // slti set less than imm
                        else if(func3 == slti) begin
                            regfile[rd] <= (rs1 < imm)?1:0;
                            alu_result <= (rs1 < imm)?1:0;
                        end
                        //sltiu set less than imm (u) zero extend 
                        else if(func3 == sltiu) begin
                            regfile[rd] <= (rs1 < imm)?1:0;
                            alu_result <= (rs1 < imm)?1:0;
                        end
                        // slli shift left logical imm[5:11]=0x00  -> rd = rs1 << imm[0:4]
                        else if(func3 == slli) begin                         
                            regfile[rd] <= (rs1 << imm[4:0]);
                            alu_result <= (rs1 << imm[4:0]);
                        end
                        // srli shift right logical  rd = rs1 >> imm[0:4]
                        else if(func3 == srli && imm[11:5] == 7'h00) begin
                            regfile[rd] <= (rs1 >> imm[4:0]);
                            alu_result <= (rs1 >> imm[4:0]);
                        end
                        // shift right alrith imm
                        else if(func3 == srai && imm[11:5] == 7'h20) begin                        
                            regfile[rd] = rs1 >>> srai_imm;
                            alu_result = rs1 >>> srai_imm;
                        end
                    end
                    else
                    begin
                        if (func3 == addi)begin
                            regfile[rd] <= imm + regfile[rs1];
                            alu_result <= imm + regfile[rs1];
                        end
                        else if (func3 == xori) begin
                            regfile[rd] <= imm ^ regfile[rs1];  // remember to change all the rs1 here to regfile[rs1]
                            alu_result <= imm ^ regfile[rs1];
                        end
                        else if (func3 == ori) begin
                            regfile[rd] <= imm | regfile[rs1];
                            alu_result <= imm | regfile[rs1];
                        end
                        else if(func3 == andi) begin
                            regfile[rd] <= imm & regfile[rs1];
                            alu_result <= imm & regfile[rs1];
                        end
                        else if(func3 == slti) begin
                            regfile[rd] <= (regfile[rs1] < imm)?1:0;
                            alu_result <= (regfile[rs1] < imm)?1:0;
                        end
                        // need to fix: sltiu is zero extend
                        else if(func3 == sltiu) begin
                            regfile[rd] <= (regfile[rs1] < imm)?1:0;
                            alu_result <= (regfile[rs1] < imm)?1:0;
                        end
                        else if(func3 == slli) begin
                            regfile[rd] <= (regfile[rs1] << imm[4:0]);
                            alu_result <= (regfile[rs1] << imm[4:0]);
                        end
                        else if(func3 == srli && imm[11:5] == 7'h00) begin // logical = unsigned
                            regfile[rd] <= (regfile[rs1] >> imm[4:0]);
                            alu_result <= (regfile[rs1] >> imm[4:0]);
                        end
                        else if(func3 == srai && imm[11:5] == 7'h20) begin
                            regfile[rd] = regfile[rs1] >>> srai_imm;
                            alu_result = regfile[rs1] >>> srai_imm;
                        end
                    end
                    $display("alu result: %d", alu_result);
                    $display("imm[4:0]: %b", imm[4:0]);
                    $display("imm_ext: %b", imm_ext);
                    $display("srai_imm: %b", srai_imm);
                end
                if(stall)
                begin
                    rd_prev <= 5'bxxxxx;
                end
            end
        end
    end
    
   // Memory Access
    always@(posedge clk or negedge rst_n)
    begin
        if(!rst_n)
        begin
            dmem <= 32'bx;
            mem_wriback_reg <= 0;
        end
        else
        begin
            mem_wriback_reg <= exec_mem_reg;
            if(exec_mem_reg)
            begin
                dmem <= alu_result;
                reg_file = $fopen("reg_trace.txt", "a");
                $fdisplay(reg_file, "register value: %d\n", rd,dmem);
                $display("Register value: %d", dmem);
                $display("Register: %d", rd);
            end
        end
    end
    
    // Write Back
    always@(posedge clk or negedge rst_n)
    begin
        if(!rst_n)
        begin
            dmem_wen <= 0;
        end
        else
        begin
            dmem_wen <= mem_wriback_reg;
            if(mem_wriback_reg)
            begin
                dmem_addr <= dmem_addr + 4;
            end
        end
    end
    
endmodule
