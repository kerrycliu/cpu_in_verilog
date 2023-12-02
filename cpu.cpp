// CPU in C++. RV32I Instructions simulator.
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <bitset>

using namespace std;

long int regfile[32] = {0};
vector<uint32_t> memory(4096);
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

uint32_t lw(uint32_t address){
	if (sizeof(address) < memory.size()){
		//cout << "address from lw: " << address << endl;
		for(int i = 0; i < memory.size(); i++){
			if(memory[i] == NULL){
				memory[i] = address;
			}
			if(memory[i] != NULL){
				//cout << "memory[i]:  " << memory[i] << endl;
				if(memory[i] == address){
					//cout << "memory index from lw: " <<  memory[i] << endl;
					return memory[i];
				}
			}
		}
	}
	else{
		cout << "Memory" << address << "  out of range" << endl;
		return 1;
	}
}

void execute_instruction(const Instruction& decoded_inst, int32_t pc, uint32_t base_addr){
	unsigned int imm_bits = (decoded_inst.immediate >> 5) & 0x3F;
	int imm_bits_signed = (decoded_inst.immediate >> 5) & 0x3F;
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
	if(decoded_inst.opcode == "0000011"){
		
		cout << "\tLoad Word Only" << endl;
		uint32_t valid_address = base_addr + decoded_inst.immediate;
		
		//cout << "valid_addree calc" << valid_address  << endl;
		//cout << "rd: " << regfile[decoded_inst.rd] << endl;

		regfile[decoded_inst.rd] = lw(valid_address);
		cout << "regfile[rd]: " << regfile[decoded_inst.rd] << endl;
		/* DEBUG: 
			for(int i = 0; i < 4096; i++){
				if(memory[i] != NULL){
					cout << "memory[i]:  " << memory[i] << endl;
			

					if(memory[i] == valid_address){
						cout << "memory index from lw: " <<  memory[i] << endl;
					}
				}
			}
		*/
		cout << "Load: " << " from address " << valid_address << endl;

	}
	else if(decoded_inst.opcode == "0010011"){
		cout << "\tI-Type Instruction" << endl;
		switch(decoded_inst.func3){
			case 0x0: //addi
				regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] + decoded_inst.immediate;
				cout << "addi: "  << endl;
				cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
				cout << "\tPC: " << pc << endl;
				break;
			case 0x4: // xori
				regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] ^ decoded_inst.immediate;
				cout << "xori: "  << endl;
				cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
				cout << "\tPC: " << pc << endl;
				break;
			case 0x6: // ori
				regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] | decoded_inst.immediate;
				cout << "ori: " << endl;
				cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
				cout << "\tPC: " << &pc << endl;
				break;
			case 0x7: // andi
				regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] & decoded_inst.immediate;
				cout << "andi: " << endl;
				cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
				cout << "\tPC: " << &pc << endl;
				break;
			case 0x1: // slli
				regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] << imm_bits;
				cout << "slli: " << endl;
				cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
				cout << "\tPC: " << &pc << endl;
				break;
			case 0x5: // srli
				// imm[5:11] = 0x00
				if (decoded_inst.immediate == 010000){
					regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] >> imm_bits;
					cout << "srli: "  << endl;
					cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
					cout << "\tPC: " << &pc << endl;
				}
				// imm[5:11] = 0x20 
				else {
					// srai
					regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] >> imm_bits_signed;
					cout << "srai: " << endl;
					cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
					cout << "\tPC: " << &pc << endl;
				}
				break;
			case 0x2: //slti
				if(regfile[decoded_inst.rs1] < decoded_inst.immediate){
					regfile[decoded_inst.rd] = 1;
				}
				else {
					regfile[decoded_inst.rd] = 0;
				}
				cout << "slti: " << endl;
				cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
				cout << "\tPC: " << &pc << endl;
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
						cout << "add: " << endl;
						cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
						cout << "\tPC: " << &pc << endl;
						break;
					case  0x20: //sub
						regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] - decoded_inst.rs2;
						cout << "sub: " << endl;
						cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
						cout << "\tPC: " << &pc << endl;
						break;
				}
			case 0x4: //xor
				regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] ^ decoded_inst.rs2;
				cout << "xor: " << endl;
				cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
				cout << "\tPC: " << &pc << endl;
				break;
			case 0x6: //or:
				regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] | decoded_inst.rs2;
				cout << "xor: " << endl;
				cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
				cout << "\tPC: " << &pc << endl;
				break;
			case 0x7: //and:
				regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] & decoded_inst.rs2;
				cout << "and: " << endl;
				cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
				cout << "\tPC: " << &pc << endl;
				break;
			case 0x1: //sll:
				regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] << decoded_inst.rs2;
				cout << "sll: " << endl;
				cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
				cout << "\tPC: " << &pc << endl;
				break;
			case 0x5:
				switch(decoded_inst.func7){
					case 0x00: //srl
						regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] >> decoded_inst.rs2;
						cout << "srl: " << endl;
						cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
						cout << "\tPC: " << &pc << endl;
						break;
					case 0x20: //sra
						regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] >> decoded_inst.rs2_signed;
						cout << "sra: " << endl;
						cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
						cout << "\tPC: " << &pc << endl;
						break;
				}
				break;
			case 0x2: //slt
				if(regfile[decoded_inst.rs1] < decoded_inst.rs2){
					regfile[decoded_inst.rd] = 1;
					cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
					cout << "\tPC: " << &pc << endl;

				}
				else{
					regfile[decoded_inst.rd] = 0;
					cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
					cout << "\tPC: " << &pc << endl;
				}
				break;
			case 0x3: //sltu
				if(regfile[decoded_inst.rs1] < decoded_inst.rs2){
					regfile[decoded_inst.rd] = 1;
					cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
					cout << "\tPC: " << &pc << endl;
				}
				else{
					regfile[decoded_inst.rd] = 0;
					cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
					cout << "\tPC: " << &pc << endl;
				}
				break;
		}
	}
	else if(decoded_inst.opcode == "0100011"){
		cout << "Store Instructions" << endl;
		int memory_address = regfile[decoded_inst.rs1] + decoded_inst.immediate;
		memory[memory_address] = regfile[decoded_inst.rs2];
		cout << "\tStore instruction in address: " << memory[memory_address] << endl;
	}
	else if(decoded_inst.opcode == "1101111"){
		cout << "JAL: " << endl;
		switch(decoded_inst.func3){
			case 0x0: 
				long int jump_address = pc + decoded_inst.immediate - 1; // undo one pc
				regfile[decoded_inst.rd] = pc + 1;
				pc = jump_address;
				cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
				cout << "\tPC: " << &pc << endl;
				
		}
	}
	else if(decoded_inst.opcode == "1100011"){ 
		switch(decoded_inst.func3){
			case 0x0: 
				cout << "BEQ: " << endl;
				if(regfile[decoded_inst.rs1] == regfile[decoded_inst.rs2]){
					pc += decoded_inst.immediate;
					cout << "PC: " << &pc << endl;	
				}
				else{
					cout << "Error BEQ" << endl;
				}
			break;
			case 0x1:
				cout << "BNE: " << endl;
				if(regfile[decoded_inst.rs1] != regfile[decoded_inst.rs2]){
					pc += decoded_inst.immediate;
					cout << "PC: " << &pc << endl;	
				}
				else{
					cout << "Error BNE" << endl;
				}
			break;
			case 0x4:
				cout << "BLT: " << endl;
				if(regfile[decoded_inst.rs1] < regfile[decoded_inst.rs2]){
					pc += decoded_inst.immediate;
					cout << "PC: " << &pc << endl;	
				}
				else{
					cout << "Error BLT" << endl;
				}
			break;
			case 0x5:
				cout << "BGE: " << endl;
				if(regfile[decoded_inst.rs1] >= regfile[decoded_inst.rs2]){
					pc += decoded_inst.immediate;
					cout << "PC: " << &pc << endl;	
				}
				else{
					cout << "Error BGE" << endl;
				}
			break;
			case 0x6:
				cout << "BLTU: " << endl;
				if(regfile[decoded_inst.rs1] < regfile[decoded_inst.rs2]){
					pc += decoded_inst.immediate;
					cout << "PC: " << &pc << endl;	
				}
				else{
					cout << "Error BLTU" << endl;
				}
			break;
			case 0x7:
				cout << "BGEU: " << endl;
				if(regfile[decoded_inst.rs1] >= regfile[decoded_inst.rs2]){
					pc += decoded_inst.immediate;
					cout << "PC: " << &pc << endl;	
				}
				else{
					cout << "Error BGEU" << endl;
				}
			break;	
		}
	}
	else if(decoded_inst.opcode == "0110111"){
		cout << "LUI instruction " << endl;
		regfile[decoded_inst.rd] = decoded_inst.immediate_lui << 12;
		cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
		cout << "PC: " << &pc << endl;
	}
	else if(decoded_inst.opcode == "0010111"){
		cout << "AUIPC instruction" << endl;
		regfile[decoded_inst.rd] = pc + (decoded_inst.immediate_lui << 12);
				cout << "\tResult: " << regfile[decoded_inst.rd] << endl;
				cout << "PC: " << &pc << endl;
	}

}


int main(){
	uint32_t baseAddress = 0x10010000; //set base address
	vector<string> instr; // vector string for instructions

	ifstream myfile;
	string mystring;
	myfile.open("ldst_cpp.dat");
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

	
	char user_input; 
	cout << "Enter 'r': run entire program at once.  " << endl;
	cout << "Enter 's': run one instruction at a time. Wait for next instruction. " << endl;

	cin >> user_input;

        if (user_input == 'r'){

	// fetch the first instruction
		while(pc < instr.size()){
			string binary_instruction = instr[pc];
			Instruction decoded_inst = decode_stage(binary_instruction);
			execute_instruction(decoded_inst, pc, baseAddress);
			pc++;
		}
	}
	else if(user_input == 's'){
		string user_instruction;
		cout << "Enter 32-bit instruction now: " << endl;
		cin >> user_instruction;
		string binary_instruction = user_instruction;
		Instruction decoded_inst = decode_stage(binary_instruction);
		execute_instruction(decoded_inst, pc, baseAddress);
		pc++;
	}
	else {
		cout << "Invalid input" << endl;
		return 1;
	
	}
	return 0;
}


