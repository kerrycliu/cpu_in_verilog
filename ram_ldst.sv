`timescale 1ns/1ps

module ram #(addr_width = 32, data_width = 32, string init_file = "dummy.dat" )
(
input rst_n,
input clk,
input wen,
input [addr_width-1:0]addr,
//input [data_width-1:0] data_in,
inout [data_width-1:0]data
);

reg [data_width-1:0] mem [ 255 :0];
wire [7:0] addr_p;
assign addr_p = addr & 8'hff;
/*reg [data_width-1:0] mem [ 4294967295 : 0 ]
wire [31:0] addr_p;
assign addr_p = addr;*/


initial
    begin
        $readmemb (init_file, mem);
    end
    
assign data = rst_n ? ( wen ? 32'hz : mem[addr_p]) : 'x;

always_ff @ (posedge clk)
    begin
        if (rst_n)
            begin
                if (wen)
                    mem[addr_p] <= #0.1 data;
            end
    end


endmodule
