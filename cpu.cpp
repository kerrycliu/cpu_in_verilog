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
int32_t pc_display = 0;
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
	int immediate_s;
	int immediate_j;
	int immediate_i;
	int immediate_lui;
	unsigned int rs1;
	unsigned int rs2;
	int rd;
};

Instruction decode_stage(const string& binary_instruction){
	Instruction inst;
	bitset<32> bits(binary_instruction);
	inst.opcode = bits.to_string().substr(25,7);
	inst.func3 = static_cast<int>(bits.to_ulong() >> 12) & 0x7; // Convert to int
	inst.func7 = static_cast<int>(bits.to_ulong() >> 25); // r-type=func7 & store=immediate[11:5]
	inst.immediate_s = static_cast<int>(
    		((bits.to_ulong() >> 25) & 0x7F) << 5 |  // Bits 31:25 become Bits 11:5
    		((bits.to_ulong() >> 7) & 0x1F)           // Bits 11:7 remain the same
	);
	inst.immediate_i = static_cast<int>(bits.to_ulong() >> 20);  // for i-type and load
	inst.immediate_lui = static_cast<int>(bits.to_ulong() >> 12); // LUI: imm[31:12]
	inst.rs1 = static_cast<unsigned int>(bits.to_ulong() >> 15) & 0x1F;
	inst.rs2 = static_cast<unsigned int>(bits.to_ulong() >> 20) & 0x1F;
	inst.rd = static_cast<long int>(bits.to_ulong() >> 7) & 0x1F;
	inst.immediate_j = static_cast<int>(
   	  	((bits.to_ulong() >> 20) & 0x001) << 19 |  // Bit 20 becomes Bit 19
    		((bits.to_ulong() >> 1) & 0x3FF) << 9 |    // Bits 10:1 become Bits 10:9
    		((bits.to_ulong() >> 11) & 0x001) << 8 |   // Bit 11 becomes Bit 8
    		((bits.to_ulong() >> 12) & 0xFF0)           // Bits 19:12 remain the same
	);
	if (bits[31] == 1) {
		inst.immediate_s |= 0xFFFFF000;
		inst.immediate_j |= 0xFFF00000;
        	inst.immediate_i |= 0xFFFFF000; // Extend the sign for negative values
    	}
	if(inst.opcode == "0010011" && (inst.func3 == 0x1 || inst.func3 == 0x5) ){ // slli srli
		inst.immediate_i = static_cast<int>(bits.to_ulong() >> 20) & 0x1F;
		//if(bits[24] == 1){
		//	inst.immediate_i |= 0xFFFE0;
		//}
	}

	return inst;
}

int32_t lw(int32_t address){
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

void  execute_instruction(const Instruction& decoded_inst, int32_t pc, int32_t pc_display, int32_t base_addr){
	int imm_bits_signed = static_cast<int>(decoded_inst.immediate_i);
	if(decoded_inst.opcode == "0000011"){
		
		cout << "\tLoad Word Only" << endl;
		int32_t valid_address = base_addr + decoded_inst.immediate_i;
		//int32_t valid_address = base_addr + decoded_inst.rs1 + decoded_inst.immediate_i;
		regfile[decoded_inst.rd] = lw(valid_address);
		//cout << "regfile[rd]: " << regfile[decoded_inst.rd] << endl;
		//cout << "load value: " << decoded_inst.rd << endl;
		//cout << "Load: " << " from address " << valid_address << endl;

	}
	else if(decoded_inst.opcode == "0010011"){
		cout << "\tI-Type Instruction" << endl;
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
				// imm[5:11] = 0x00
				if (decoded_inst.func7 == 0x00){
					regfile[decoded_inst.rd] = static_cast<unsigned int>(regfile[decoded_inst.rs1]) >> decoded_inst.immediate_i;
				}
				// imm[5:11] = 0x20 
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
		cout << "\tR-Type Instruction" << endl;
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
		cout << "Store Instructions" << endl;
		int memory_address = regfile[decoded_inst.rs1] + decoded_inst.immediate_s;
		try{
			memory.at(memory_address) = regfile[decoded_inst.rs2];
		}
		catch(const out_of_range& e){
			cout << "Stored in: " << memory_address << endl;
		}
	}
	else if(decoded_inst.opcode == "1101111"){
		cout << "JAL: " << endl;
		switch(decoded_inst.func3){
			case 0x0: 
				long int jump_address = pc + decoded_inst.immediate_j; // undo one pc
				regfile[decoded_inst.rd] = pc + 1;
				pc = jump_address;
				break;
				
		}
	}
	else if(decoded_inst.opcode == "1100011"){ 
		switch(decoded_inst.func3){
			case 0x0: 
				cout << "BEQ: " << endl;
				if(regfile[decoded_inst.rs1] == regfile[decoded_inst.rs2]){
					pc += decoded_inst.immediate_i;
				}
				else{
					cout << "Error BEQ" << endl;
				}
				break;
			case 0x1:
				cout << "BNE: " << endl;
				if(regfile[decoded_inst.rs1] != regfile[decoded_inst.rs2]){
					pc += decoded_inst.immediate_i;
				}
				else{
					cout << "Error BNE" << endl;
				}
				break;
			case 0x4:
				cout << "BLT: " << endl;
				if(regfile[decoded_inst.rs1] < regfile[decoded_inst.rs2]){
					pc += decoded_inst.immediate_i;
				}
				else{
					cout << "Error BLT" << endl;
				}
				break;
			case 0x5:
				cout << "BGE: " << endl;
				if(regfile[decoded_inst.rs1] >= regfile[decoded_inst.rs2]){
					pc += decoded_inst.immediate_i;
				}
				else{
					cout << "Error BGE" << endl;
				}
				break;
			case 0x6:
				cout << "BLTU: " << endl;
				if(regfile[decoded_inst.rs1] < regfile[decoded_inst.rs2]){
					pc += decoded_inst.immediate_i;
				}
				else{
					cout << "Error BLTU" << endl;
				}
				break;
			case 0x7:
				cout << "BGEU: " << endl;
				if(regfile[decoded_inst.rs1] >= regfile[decoded_inst.rs2]){
					pc += decoded_inst.immediate_i;
				}
				else{
					cout << "Error BGEU" << endl;
				}
				break;	
		}
	}
	else if(decoded_inst.opcode == "0110111"){
		cout << "\tLUI instruction " << endl;
		regfile[decoded_inst.rd] = decoded_inst.immediate_lui << 12;
	}
	else if(decoded_inst.opcode == "0010111"){
		cout << "\tAUIPC instruction" << endl;
		regfile[decoded_inst.rd] = pc + (decoded_inst.immediate_lui << 12);
	}
	else{
		cerr << "invalid instruction" << endl;
	}

 	
	cout << "alu_result: " << regfile[decoded_inst.rd] << endl;
	cout << "PC: " << pc_display << endl;
}


int main(){
	int32_t baseAddress = 0x10010000; //set base address
	vector<string> instr; // vector string for instructions

	ifstream myfile;
	string mystring;
	myfile.open("line.dat");
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
	cout << "Enter 's': run one instruction at a time. Wait for next instruction. Ctrl C to exit." << endl;

	cin >> user_input;

        if (user_input == 'r'){

	// fetch the first instruction
		while(pc < instr.size()){
			string binary_instruction = instr[pc];
			Instruction decoded_inst = decode_stage(binary_instruction);
			execute_instruction(decoded_inst, pc, pc_display, baseAddress);
			pc_display += 4;
			pc++;
//			for(int i = 0; i < 32; i++){
//				cout << "regfile: (" << i << ") " << regfile[i] << endl;
//			}
		}
	}
	else if(user_input == 's'){
		while(true){
			string user_instruction;
			cout << "Enter 32-bit instruction now: " << endl;
			cin >> user_instruction;
			string binary_instruction = user_instruction;
			Instruction decoded_inst = decode_stage(binary_instruction);
			execute_instruction(decoded_inst, pc, pc_display, baseAddress);
			pc += 4;
			pc++;
		}
	}
	else {
		cout << "Invalid input" << endl;
		return 1;
	
	}
	return 0;
}


