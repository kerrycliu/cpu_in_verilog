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
//memory[(baseAddress-0x10010000)/sizeof(int32_t)] = -3;
//vector<int32_t> dataMemory = {-3,7,25,-1};
int32_t pc_display = 0;
int32_t pc = 0;


int32_t pre_store(){
	int32_t dmem[4]={-3,25,7,-1};
//	for(int i = 0; i < 4; i++){
//		cout << dmem[i] << endl;
//	}
	uint32_t mybase = baseAddress;
	cout << "Prestoring " << endl;
	int32_t mem_idx;
	stringstream ss;
	ss << hex << mybase;
	ss >> mem_idx;
	mem_idx &= 0xFF;
	for(int j = mem_idx; j < 12; j+=4){
		for(int i = 0; i < 4; i++){
		memory[j] = dmem[i];
		cout << "mem[inx]: " << mem_idx << endl;
		cout << "dmem[i]: " << dmem[j] << endl;
		}
		return memory[mem_idx];	
	}

}

signed int map_t(unsigned int x_reg){ // x reg to t reg
	if(x_reg >= 5 && x_reg <= 31){
		return (x_reg - 5) % 10;
	}
	else{
		return x_reg;
	}
}

int32_t base_address_int(const string& binary_variable){
	bitset<32> bits(binary_variable);
	int new_base = static_cast<int>(bits.to_ulong());

	return new_base;
}

struct Dmem {
	int value;
};

Dmem decode_dmem(const string& binary_val){
	Dmem data;
	bitset<32> bits(binary_val);
	data.value = static_cast<int>(bits.to_ulong());

	return data;
}

struct Instruction {
	string opcode;
	int func7;
	int func3;
	int immediate_s;
	int immediate_j;
	int immediate_i;
	int immediate_b;
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
	bitset<12> imm_bits_branch = static_cast<int>(bits.to_ulong() >> 20) & 0xFFF;
	inst.immediate_b = static_cast<int>(imm_bits_branch.to_ulong());
	inst.immediate_i = static_cast<int>(bits.to_ulong() >> 20);  // for i-type and load
	inst.immediate_lui = static_cast<int>(bits.to_ulong() >> 12); // LUI: imm[31:12]
	inst.rs1 = static_cast<unsigned int>(bits.to_ulong() >> 15) & 0x1F;
	inst.rs2 = static_cast<unsigned int>(bits.to_ulong() >> 20) & 0x1F;
	inst.rd = static_cast<int>(bits.to_ulong() >> 7) & 0x1F;
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
	if (imm_bits_branch[11] == 1) {
     	   inst.immediate_b |= 0xFFFFF000;  // Sign extension
    	}
	inst.immediate_b <<= 1;
	if(inst.opcode == "0010011" && (inst.func3 == 0x1 || inst.func3 == 0x5) ){ // slli srli
		inst.immediate_i = static_cast<int>(bits.to_ulong() >> 20) & 0x1F;
		if(bits[24] == 1){
			inst.immediate_i |= 0xFFFE0;
		}
	}

	return inst;
}


void execute_instruction(const Instruction& decoded_inst, int32_t pc, int32_t pc_display, uint32_t base_addr){
	/*
	cout << dmem_data.value << endl;
	int32_t mem_ind;
	int32_t memory_address = baseAddress;
        stringstream ss;
        ss << hex << memory_address;
        ss >> mem_ind;
	mem_ind &= 0xFF;
	memory[mem_ind] = dmem_data.value;
*/
	int imm_bits_signed = static_cast<int>(decoded_inst.immediate_i);
	if(decoded_inst.opcode == "0000011"){
		cout << "\tLoad Word Only" << endl;
		//regfile[decoded_inst.rs1]
		int32_t valid_address = regfile[decoded_inst.rs1] + decoded_inst.immediate_i;
		int32_t mem_ind = 0;
		stringstream ss;
		ss << hex << valid_address;
		ss >> mem_ind;
		mem_ind &= 0xFF;
		regfile[decoded_inst.rd] = memory[mem_ind];
		cout << "Load: " << regfile[decoded_inst.rd] << " from address " << valid_address << endl;

	}
	else if(decoded_inst.opcode == "0010011"){
		cout << "I" << endl;
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
		cout << "R" << endl;
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
		cout << "S" << endl;
		int32_t mem_ind;
		int32_t memory_address = regfile[decoded_inst.rs1] + decoded_inst.immediate_s;
		stringstream ss;
		ss << hex << memory_address;
		ss >> mem_ind;
		mem_ind &= 0xFF;
		try{
			memory.at(mem_ind) = regfile[decoded_inst.rs2];
			cout << "Stored: " << memory[mem_ind] << " in address " << memory_address << endl;
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
					pc += decoded_inst.immediate_b;
				}
				else{
					cout << "Error BEQ" << endl;
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
				cout << "BLT: " << endl;
				if(regfile[decoded_inst.rs1] < regfile[decoded_inst.rs2]){
					pc += decoded_inst.immediate_b;
				}
				else{
					cout << "Error BLT" << endl;
				}
				break;
			case 0x5:
				cout << "BGE: " << endl;
				if(regfile[decoded_inst.rs1] >= regfile[decoded_inst.rs2]){
					cout << "decoded_inst.immediate_b: " << decoded_inst.immediate_b << endl;
					pc += decoded_inst.immediate_b;
				}
				
				break;
			case 0x6:
				cout << "BLTU: " << endl;
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
 	
//	cout << "rs1: " << decoded_inst.rs1 << " rs2: " << regfile[decoded_inst.rs2] << endl;
//	cout << "imm: " << decoded_inst.immediate_i << endl;
	cout << "register: t" << map_t(decoded_inst.rd) << " = " << regfile[decoded_inst.rd] << endl;
	cout << "PC: " << pc_display << endl;
}

void exec_value(const Dmem& dmem_data){

	cout << dmem_data.value << endl;
	int32_t mem_ind;
	int32_t memory_address = baseAddress;
        stringstream ss;
        ss << hex << memory_address;
        ss >> mem_ind;
	mem_ind &= 0xFF;
	memory[mem_ind] = dmem_data.value;
}


int main(){
	vector<string> instr; // vector string for instructions
	vector<string> dmem;

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
	
	ifstream dataFile("dmem_cpp.dat", ios::binary);
	string mystring2;
	if(dataFile.is_open()){
		while (getline(dataFile, mystring2)) {
	//		int32_t intVal = stoi(mystring2, nullptr, 2);
            		dmem.push_back(mystring2);
		//	cout << "Memory[" << baseAddress + dataMemory.size() * sizeof(int32_t) << "] = " << intVal << endl;
			//cout << dataMemory.back() << endl;
			if(dmem.size()==32){
				break;
			}
        	}	
        	dataFile.close();
	}
	
	memory[0] = -3;
	memory[4] = 25;
	memory[8] = 7;
	memory[12]= -1;
	
	char user_input; 
	cout << "Enter 'r': run entire program at once.  " << endl;
	cout << "Enter 's': run one instruction at a time. Wait for next instruction. Ctrl C to exit." << endl;

	cin >> user_input;

//	int32_t dmem_values[4] = {-3,25,7,-1};
	
//	for (int i = 0; i < 4; i++){
//		memory[i] = pre_store();
//		cout << "main: " << memory[i] << endl;
//	}
//	cout << "memory" << endl;
	//for (size_t i = 0; i < dataMemory.size(); ++i) {
        //	int32_t intValue = std::stoi(dataMemory[i], nullptr, 2);
        //	reinterpret_cast<int32_t*>(&dataMemory[baseAddress + i * sizeof(int32_t)])[0] = intValue;
	//	cout << dataMemory << endl;
    //	}
	
	int data_counter = 0;
	
        if (user_input == 'r'){
		while(pc < instr.size()){
		//	while( data_counter < dmem.size()){
		//		string binary_val = dmem[data_counter];
		//		Dmem dmem_data = decode_dmem(binary_val);
		//		exec_value(dmem_data);
		//		data_counter++;
		//	}
			string binary_instruction = instr[pc];
			//string binary_val = dmem[data_counter];
			//Dmem dmem_data = decode_dmem(binary_val);
			Instruction decoded_inst = decode_stage(binary_instruction);
			execute_instruction(decoded_inst, pc, pc_display, baseAddress);
			pc_display += 4;
			pc++;
		}

	}
	else if(user_input == 's'){
		while(true){
			string user_instruction;
			cout << "Enter 32-bit instruction now: " << endl;
			cin >> user_instruction;
			string binary_val = dmem[pc];
			Dmem dmem_data = decode_dmem(binary_val);
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


