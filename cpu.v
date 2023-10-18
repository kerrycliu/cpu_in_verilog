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
    output [31:0] imem_addr,
    input [31:0] imem_insn,
    output [31:0] dmem_addr,
    inout [31:0] dmem_data,
    output dmem_wen
    );
    
    //16-bit counter
    reg [15:0] counter;
    reg [31:0] PC;
    //pipelining
    reg fetch_reg, decode_reg, execute_reg, memAccess_reg, writeBack_reg;
    
    //fetch
    always@(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
           counter <= 16'b0;
           end
        else begin
           counter <= counter + 1;
           end
    end
    
    //decode
    always@(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
           counter <= 16'b0;
           end
        else begin
           counter <= counter + 1;
           end
    end
    
    //execute
    always@(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
           counter <= 16'b0;
           end
        else begin
           counter <= counter + 1;
           end
    end
    // memory access
    always@(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
           counter <= 16'b0;
           end
        else begin
           counter <= counter + 1;
           end
    end
    
    //Write Back
    always@(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
           counter <= 16'b0;
           end
        else begin
           counter <= counter + 1;
           end
    end
    
    
        
    
endmodule
