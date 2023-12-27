// CPU in C++. RV32I Instructions simulator.
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <bitset>

using namespace std;

uint32_t baseAddress = 0x10010000; //set base address
vector<int32_t> regfile(32,0);
vector<int32_t> memory(32,0);
int32_t pc_display = 0;
int32_t pc = 0;
int32_t valid_address;

signed int map_t(unsigned int x_reg){ // x reg to t reg
	if(x_reg >= 5 && x_reg <= 31){
		return (x_reg - 5) % 10;
	}
	else{
		return x_reg;
	}
}

struct Instruction {
	string opcode;
	int func7;
	int func3;
	int immediate_s;
	unsigned int immediate_j;
	int immediate_i;
	int immediate_b;
	int immediate_lui;
	int rs1;
	int rs2;
	int rd;
};

Instruction decode_stage(const string& binary_instruction){
	Instruction inst;
	bitset<32> bits(binary_instruction);
	unsigned int tmp1;
	unsigned int tmp2;
	unsigned int tmp3;
	unsigned int tmp4;

	int32_t b11 = 0;
	int32_t b4_1 = 0;
	int32_t b10_5 = 0;
	int32_t b12 = 0;
	inst.opcode = bits.to_string().substr(25,7);
	inst.func3 = static_cast<int>(bits.to_ulong() >> 12) & 0x7; // Convert to int
	inst.func7 = static_cast<int>(bits.to_ulong() >> 25); // r-type=func7 & store=immediate[11:5]
	inst.immediate_s = static_cast<int>(
    		((bits.to_ulong() >> 25) & 0x7F) << 5 |  // Bits 31:25 become Bits 11:5
    		((bits.to_ulong() >> 7) & 0x1F)           // Bits 11:7 remain the same
	);
	b11 = (bits.to_ulong() >> 7) & 0x1; //im[11]=bits[7]
	b4_1 = (bits.to_ulong() >> 8) & 0x7; // bits[8:11]
	b10_5 = (bits.to_ulong() >> 25) & 0x3F;
	b12 = (bits.to_ulong() >> 31) & 0x1;
	inst.immediate_b = static_cast<int>(b12 | b10_5 | b4_1 | b11);
	inst.immediate_i = static_cast<int>(bits.to_ulong() >> 20);  // for i-type and load
	inst.immediate_lui = static_cast<int>(bits.to_ulong() >> 12); // LUI: imm[31:12]
	inst.rs1 = static_cast<int>(bits.to_ulong() >> 15) & 0x1F;
	inst.rs2 = static_cast<int>(bits.to_ulong() >> 20) & 0x1F;
	inst.rd = static_cast<int>(bits.to_ulong() >> 7) & 0x1F;
	tmp1 = (bits.to_ulong() >> 23) & 0xFF; //[19:12]
	tmp2 = (bits.to_ulong() >> 22) & 0x1; // [11]
	tmp3 = (bits.to_ulong() >> 12) & 0x3FF; // [10:1]
	tmp4 = (bits.to_ulong() >> 31); // [31]
	inst.immediate_j = static_cast<unsigned int>(tmp4 | tmp3 | tmp2 | tmp1);
	if (bits[31] == 1) {
		inst.immediate_s |= 0xFFFFF000;
		inst.immediate_j |= 0xFFF00000;
        	inst.immediate_i |= 0xFFFFF000; // Extend the sign for negative values
     	   	inst.immediate_b |= 0xFFFFF000;  // Sign extension
    	}
	if(inst.opcode == "0010011" && (inst.func3 == 0x1 || inst.func3 == 0x5) ){ // slli srli
		inst.immediate_i = static_cast<int>(bits.to_ulong() >> 20) & 0x1F;
		if(bits[24] == 1){
			inst.immediate_i |= 0xFFFE0;
		}
	}

	return inst;
}

void execute_instruction(const Instruction& decoded_inst, int32_t pc_display, uint32_t base_addr){
	int imm_bits_signed = static_cast<int>(decoded_inst.immediate_i);
	if(decoded_inst.opcode == "0000011"){
		valid_address = regfile[decoded_inst.rs1] + decoded_inst.immediate_i;
		int32_t mem_ind = 0;
		stringstream ss;
		ss << hex << valid_address;
		ss >> mem_ind;
		mem_ind &= 0xFF;
		regfile[decoded_inst.rd] = memory[mem_ind];

	}
	else if(decoded_inst.opcode == "0010011"){
		switch(decoded_inst.func3){
			case 000: //addi
				regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] + decoded_inst.immediate_i;
				break;
			case 0x4: // xori
				regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] ^ decoded_inst.immediate_i;
				break;
			case 0x6: // ori
				regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] | decoded_inst.immediate_i;
				break;
			case 0x7: // andi
				regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] & decoded_inst.immediate_i;
				break;
			case 0x1: // slli
				regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] << decoded_inst.immediate_i;
				break;
			case 0x5: // srli
				if (decoded_inst.func7 == 0x00){
					regfile[decoded_inst.rd] = static_cast<unsigned int>(regfile[decoded_inst.rs1]) >> decoded_inst.immediate_i;
				}
				else {
					// srai
					regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] >> imm_bits_signed;
				}
				break;
			case 0x2: //slti
				if(regfile[decoded_inst.rs1] < decoded_inst.immediate_i){
					regfile[decoded_inst.rd] = 1;
				}
				else {
					regfile[decoded_inst.rd] = 0;
				}
				break;	
		}
	}
	else if(decoded_inst.opcode == "0110011"){
		switch (decoded_inst.func3){
			case 0x0:
				switch(decoded_inst.func7){
					case  0x00: //add
						regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] + regfile[decoded_inst.rs2];
						break;
					case  0x20: //sub
						regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] - regfile[decoded_inst.rs2];
						break;
				}
				break;
			case 0x4: //xor
				regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] ^ regfile[decoded_inst.rs2];
				break;
			case 0x6: //or:
				regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] | regfile[decoded_inst.rs2];
				break;
			case 0x7: //and:
				regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] & regfile[decoded_inst.rs2];
				break;
			case 0x1: //sll:
				regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] << regfile[decoded_inst.rs2];
				break;
			case 0x5:
				switch(decoded_inst.func7){
					case 0x00: //srl
						regfile[decoded_inst.rd] = static_cast<unsigned int>(regfile[decoded_inst.rs1]) >> regfile[decoded_inst.rs2];
						break;
					case 0x20: //sra
						regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] >> regfile[decoded_inst.rs2];
						break;
				}
				break;
			case 0x2: //slt
				cout << "slt: " << endl;
				if(regfile[decoded_inst.rs1] < regfile[decoded_inst.rs2]){
					regfile[decoded_inst.rd] = 1;

				}
				else{
					regfile[decoded_inst.rd] = 0;
				}
				break;
			case 0x3: //sltu
				if(regfile[decoded_inst.rs1] < regfile[decoded_inst.rs2]){
					regfile[decoded_inst.rd] = 1;
				}
				else{
					regfile[decoded_inst.rd] = 0;
				}
				break;
		}
	}
	else if(decoded_inst.opcode == "0100011"){
		int32_t mem_ind;
		int32_t memory_address = regfile[decoded_inst.rs1] + decoded_inst.immediate_s;
		stringstream ss;
		ss << hex << memory_address;
		ss >> mem_ind;
		mem_ind &= 0xFF;
		try{
			memory.at(mem_ind) = regfile[decoded_inst.rs2];
		}
		catch(const out_of_range& e){
			cout << "Stored in: " << memory_address << endl;
		}
	}
	else if(decoded_inst.opcode == "1101111"){
		int32_t jump_address;
		if(map_t(decoded_inst.rd) == 0){
			regfile[decoded_inst.rd] = 0;
			pc = pc + static_cast<int>(decoded_inst.immediate_j);
		}
		else{
			regfile[decoded_inst.rd] = pc + 1;
		}
	}
	else if(decoded_inst.opcode == "1100011"){ 
		int32_t result, result1,result2;
		switch(decoded_inst.func3){
			case 0x0: 
				if (map_t(decoded_inst.rs2) == 0){
					regfile[decoded_inst.rs2] = 0;
				}
				result = regfile[decoded_inst.rs1] - regfile[decoded_inst.rs2]; 
				if(result == 0){
					pc += static_cast<int>(decoded_inst.immediate_b);
				}
				break;
			case 0x1:
				cout << "BNE: " << endl;
				if(regfile[decoded_inst.rs1] != regfile[decoded_inst.rs2]){
					pc += decoded_inst.immediate_b;
				}
				else{
					cout << "Error BNE" << endl;
				}
				break;
			case 0x4:
				if (map_t(decoded_inst.rs2) == 0){
					regfile[decoded_inst.rs2] = 0;
				}
				result2 = regfile[decoded_inst.rs1] - regfile[decoded_inst.rs2];
				if(result2 <= 0){
					pc = pc + (static_cast<int>(decoded_inst.immediate_b)/2 + 1);
				}
				else{
					cout << "No BLT condition met" << endl;
				}
				break;
			case 0x5:
				if (map_t(decoded_inst.rs2) == 0){
					regfile[decoded_inst.rs2] = 0;
				}
				result1 = regfile[decoded_inst.rs1] - regfile[decoded_inst.rs2];
				if(result1 >= 0){
					pc = pc + (static_cast<int>(decoded_inst.immediate_b)/2 - 1);
				}
				else{
	//				cout << "BGE condition not met" << endl;
				}
				
				break;
			case 0x6:
				if(regfile[decoded_inst.rs1] < regfile[decoded_inst.rs2]){
					pc += decoded_inst.immediate_b;
				}
				else{
					cout << "Error BLTU" << endl;
				}
				break;
			case 0x7:
				cout << "BGEU: " << endl;
				if(regfile[decoded_inst.rs1] >= regfile[decoded_inst.rs2]){
					pc += decoded_inst.immediate_b;
				}
				else{
					cout << "Error BGEU" << endl;
				}
				break;	
		}
	}
	else if(decoded_inst.opcode == "0110111"){
		regfile[decoded_inst.rd] = decoded_inst.immediate_lui << 12;
	}
	else if(decoded_inst.opcode == "0010111"){
		cout << "\tAUIPC instruction" << endl;
		regfile[decoded_inst.rd] = pc + (decoded_inst.immediate_lui << 12);
	}
	else{
		cerr << "invalid instruction" << endl;
	}
    	
	stringstream hh;
	hh << hex << regfile[decoded_inst.rd];
	string res (hh.str());
	cout << "register: t" << map_t(decoded_inst.rd) << " = " << res << endl;
	cout << "PC: " << pc_display << endl;
} 

int reg_file(string reg_val){
	int reg;
	reg = stoi(reg_val.substr(1));
	cout << regfile[reg] << " in func" << endl;
	return regfile[reg];
}

int main(){
	vector<string> instr; // vector string for instructions
	vector<string> dmem;
	
	ifstream myfile;
	string mystring;
	myfile.open("arith_mean_cpp.dat");
	if (myfile.is_open()){
		while(getline(myfile, mystring)){
			instr.push_back(mystring);
			//cout << instr.back() << endl;
			if(instr.size() == 32){
				break;
			}
		}
		myfile.close();
	}
	
// Manually insert values to memory (dmem)
	memory[0] = 1;
	memory[4] = -1;
	memory[8] = 3;
	memory[12]= -3;
	memory[16] = 9;
	memory[20] = 6;
	memory[24] = 0;
	memory[28] = 0;
	memory[32] = 10;

/* Test case for another memory value/location
	memory[0] = -3;
	memory[4] = 7;
	memory[8] = 25;
	memory[2]= -1;
*/
	while(true){
		string user_input; 
		cout << "Enter 'r': run entire program at once.  " << endl;
		cout << "Enter 'pc': to show pc" << endl;
		cout << "Enter 's': run one instruction at a time. Wait for next instruction. Ctrl C to exit." << endl;
		cout << "Enter 'reg': for register value.  Ctrl C to exit." << endl;


		cin >> user_input;

		int data_counter = 0;
	
        	if (user_input == "r"){
			while(pc < instr.size()){
				string binary_instruction = instr[pc];
				Instruction decoded_inst = decode_stage(binary_instruction);
				execute_instruction(decoded_inst, pc_display, baseAddress);
				pc_display += 4;
				pc++;
			}
		}
		else if(user_input == "s"){
			string user_instruction;
			cout << "Enter 32-bit instruction now: " << endl;
			cin >> user_instruction;
			string binary_instruction = user_instruction;
			Instruction decoded_inst = decode_stage(binary_instruction);
			execute_instruction(decoded_inst, pc_display, baseAddress);
			pc_display += 4;
		}
		else if (user_input == "pc"){
			cout << "pc: " << pc_display << endl;
		}
		else if(user_input == "reg"){
			string reg_value;
			cout << "Enter Register number (ie: x0)" << endl;
			cin >> reg_value;
			int out;
			out = reg_file(reg_value);
			cout << "Register: " << reg_value  << " (t" << map_t(stoi(reg_value.substr(1))) << ")" << " has the value of " << out <<endl;
		}
    	}

	return 0;
}


