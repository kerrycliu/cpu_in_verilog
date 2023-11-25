// CPU in C++. RV32I Instructions simulator.
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <bitset>

using namespace std;

long int regfile[32] = {0};
int32_t pc = 0;

/*
int32_t fetch_imem_insn(int32_t imem_addr){
	int32_t imem_insn;
	
	Copy Block of Memory
	memcpy(*destination, *source, size_t num)
		destination: pointer to the destination array
		source: pointer to source of data to be copied
		size: copy exact num bytes
	
	memcpy(&imem_insn, &memory[imem_addr], sizeof(int32_t));
	return imem_insn;
}

void store_word(int32_t imem_addr, int32_t imem_insn){
	memcpy(&memory[imem_addr], &imem_insn, sizeof(int32_t));
}

*/

struct Instruction {
	string opcode;
	int func7;
	int func3;
	int immediate;
	int immediate_lui;
	int rs1;
	int rs2_signed;
	unsigned int rs2;
	long int rd;
};

Instruction decode_stage(const string& binary_instruction){
	Instruction inst;
	bitset<32> bits(binary_instruction);
	/*
	bits.to_ulong(): convert 32-bit binary to unsigned long int
	>> 12: right shift 12 bits. Disgard lower 12 bits and keep the upper 20 bits.
	static_cast<int>: converts the result to integer
	& 0x7: bitwise AND operation with hex 0x7 (00000111). Ensures that only the lower 3 bits of the result are kept, restricting the value to be in the range of [0,7]
	*/
	// func3 = 000
	// xxxxxxxx000 & 0x7 = 000
	inst.opcode = bits.to_string().substr(25,7);
	//inst.opcode = static_cast<int>(bits.to_ulong()) & 0x1F;
	inst.func3 = static_cast<int>(bits.to_ulong() >> 12) & 0x7; // Convert to int
	inst.func7 = static_cast<int>(bits.to_ulong() >> 25); // r-type=func7 & store=immediate[11:5]
	inst.immediate = static_cast<int>(bits.to_ulong() >> 20);  // for i-type and load
	inst.immediate_lui = static_cast<int>(bits.to_ulong() >> 12); // LUI: imm[31:12]
	inst.rs1 = static_cast<int>(bits.to_ulong() >> 15) & 0x1F;
	inst.rs2 = static_cast<unsigned int>(bits.to_ulong() >> 20) & 0x1F;
	inst.rs2_signed = static_cast<int>(bits.to_ulong() >> 20) & 0x1F;
	inst.rd = static_cast<long int>(bits.to_ulong() >> 7) & 0x1F;

	if (bits[31] == 1) {
        	inst.immediate |= 0xFFFFF000; // Extend the sign for negative values
    	}

	return inst;
}

uint8_t lb(int rs1, int imm){
	int memory_address = regfile[rs1] + imm;
	return memory[memory_address] & 0xFF; //only want lower 8 bits
}

int main(){
	ifstream myfile;
	string mystring;
	vector<string> instr;
	vector<uint8_t> memory(4096,0); //for load and store
	myfile.open("r_type_cpp.dat");
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

	// fetch the first instruction
	while(pc < instr.size()){
		string binary_instruction = instr[pc];
		Instruction decoded_inst = decode_stage(binary_instruction);
		
		//Test Decode Output
		//cout << "Executing Instruction:" << endl;
        	//cout << "Opcode: " << decoded_inst.opcode << endl;
		//cout << "binary inst: " << binary_instruction << endl;
		//cout << "pc: " << pc << endl;
        	//cout << "Func7: " << decoded_inst.func7 << endl;
        	//cout << "Func3: " << decoded_inst.func3 << endl;
        	//cout << "immediate: " << decoded_inst.immediate << endl;
        	//cout << "rs1: " << decoded_inst.rs1 << endl;
        	//cout << "rs2: " << decoded_inst.rs2 << endl;
        	//cout << "rd: " << decoded_inst.rd << endl;
		
		unsigned int imm_bits = (decoded_inst.immediate >> 5) & 0x3F;
		int imm_bits_signed = (decoded_inst.immediate >> 5) & 0x3F;

		if(decoded_inst.opcode == "0010011"){
			cout << "\tI-Type Instruction" << endl;
			switch(decoded_inst.func3){
				case 0x0: //addi
					regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] + decoded_inst.immediate;
					cout << "addi: " << decoded_inst.func3 << endl;
					cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
					cout << "PC: " << &pc << endl;
					break;
				case 0x4: // xori
					regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] ^ decoded_inst.immediate;
					cout << "xori: " << decoded_inst.func3 << endl;
					cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
					cout << "PC: " << &pc << endl;
					break;
				case 0x6: // ori
					regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] | decoded_inst.immediate;
					cout << "ori: " << decoded_inst.func3 << endl;
					cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
					cout << "PC: " << &pc << endl;
					break;
				case 0x7: // andi
					regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] & decoded_inst.immediate;
					cout << "andi: " << decoded_inst.func3 << endl;
					cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
					cout << "PC: " << &pc << endl;
					break;
				case 0x1: // slli
					regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] << imm_bits;
					cout << "slli: " << decoded_inst.func3 << endl;
					cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
					cout << "PC: " << &pc << endl;
					break;
				case 0x5: // srli
					// imm[5:11] = 0x00
					if (decoded_inst.immediate == 010000){
						regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] >> imm_bits;
						cout << "srli: " << decoded_inst.func3 << endl;
						cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
						cout << "PC: " << &pc << endl;
					}
					// imm[5:11] = 0x20 
					else {
						// srai
						regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] >> imm_bits_signed;
						cout << "srai: " << decoded_inst.func3 << endl;
						cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
						cout << "PC: " << &pc << endl;
					}
					break;
				case 0x2: //slti
					if(regfile[decoded_inst.rs1] < decoded_inst.immediate){
						regfile[decoded_inst.rd] = 1;
					}
					else {
						regfile[decoded_inst.rd] = 0;
					}
					cout << "slti: " << decoded_inst.func3 << endl;
					cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
					cout << "PC: " << &pc << endl;
					break;	
			}
		}
		else if(decoded_inst.opcode == "0110011"){
			cout << "\tR-Type Instruction" << endl;
			switch (decoded_inst.func3){
				case 0x0:
					switch(decoded_inst.func7){
						case  0x00: //add
							regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] + decoded_inst.rs2;
							cout << "add: " << decoded_inst.func3 << endl;
							cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
							cout << "PC: " << &pc << endl;
							break;
						case  0x20: //sub
							regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] - decoded_inst.rs2;
							cout << "sub: " << decoded_inst.func3 << endl;
							cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
							cout << "PC: " << &pc << endl;
							break;
					}
				case 0x4: //xor
					regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] ^ decoded_inst.rs2;
					cout << "xor: " << endl;
					cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
					cout << "PC: " << &pc << endl;
					break;
				case 0x6: //or:
					regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] | decoded_inst.rs2;
					cout << "xor: " << endl;
					cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
					cout << "PC: " << &pc << endl;
					break;
				case 0x7: //and:
					regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] | decoded_inst.rs2;
					cout << "xor: " << endl;
					cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
					cout << "PC: " << &pc << endl;
					break;
				case 0x1: //sll:
					regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] | decoded_inst.rs2;
					cout << "xor: " << endl;
					cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
					cout << "PC: " << &pc << endl;
					break;
				case 0x5:
					switch(decoded_inst.func7){
						case 0x00: //srl
							regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] >> decoded_inst.rs2;
							cout << "srl: " << endl;
							cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
							cout << "PC: " << &pc << endl;
							break;
						case 0x20: //sra
							regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] >> decoded_inst.rs2_signed;
							cout << "sra: " << endl;
							cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
							cout << "PC: " << &pc << endl;
							break;
					}
					break;
				case 0x2: //slt
					if(regfile[decoded_inst.rs1] < decoded_inst.rs2){
						regfile[decoded_inst.rd] = 1;
					}
					else{
						regfile[decoded_inst.rd] = 0;
					}
					break;
				case 0x3: //sltu
					if(regfile[decoded_inst.rs1] < decoded_inst.rs2){
						regfile[decoded_inst.rd] = 1;
					}
					else{
						regfile[decoded_inst.rd] = 0;
					}
					break;
			}
		}
		else if(decoded_inst.opcode == "0000011"){
			cout << "\tLoad Instructions" << endl;

			switch(decoded_inst.func3){
				case 0x0: //load byte
					regfile[decoded_inst.rd] = lb(regfile[decoded_inst.rs1], decoded_inst.immediate);
					cout << "LB" << endl;
					cout << "Result: " << regfile[decoded_inst.rd] << endl;
					cout << "PC: " << regfile[decoded_inst.rd] << endl;
					break;
				case 0x1: //load half
					break;
			}
		}
		else if(decoded_inst.opcode == "0100011"){
			cout << "\tStore Instructions" << endl;
		}
		else if(decoded_inst.opcode == "1101111"){
			cout << "J and B Instructions" << endl;
		}
		else if(decoded_inst.opcode == "0110111"){
			cout << "LUI instruction " << endl;
		}
		else if(decoded_inst.opcode == "0010111"){
			cout << "AUIPC instruction" << endl;
		}
		pc++;
	}
	return 0;
}

