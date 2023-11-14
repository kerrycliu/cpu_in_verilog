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
int32_t *pc_ptr = &pc;
uint8_t memory[4096] = {0};

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
	int opcode;
	int func7;
	int func3;
	int immediate;
	int rs1;
	int rs2;
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
	inst.opcode = static_cast<int>(bits.to_ulong()) & 0x1F;
	inst.func3 = static_cast<int>(bits.to_ulong() >> 12) & 0x7; // Convert to int
	inst.func7 = static_cast<int>(bits.to_ulong() >> 25);
	inst.immediate = static_cast<int>(bits.to_ulong() >> 20);
	inst.rs1 = static_cast<int>(bits.to_ulong() >> 15) & 0x1F;
	inst.rs2 = static_cast<int>(bits.to_ulong() >> 20) & 0x1F;
	inst.rd = static_cast<long int>(bits.to_ulong() >> 7) & 0x1F;

	if (bits[31] == 1) {
        	inst.immediate |= 0xFFFFF000; // Extend the sign for negative values
    	}

	return inst;
}

int main(){
	ifstream myfile;
	string mystring;
	vector<string> instr;
	myfile.open("i_type_cpp.dat");
	if (myfile.is_open()){
		while(getline(myfile, mystring)){
			instr.push_back(mystring);
			cout << instr.back() << endl;
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
		
		/* Test Decode Output

		cout << "Executing Instruction:" << endl;
        	cout << "Opcode: " << decoded_inst.opcode << endl;
        	cout << "Func7: " << decoded_inst.func7 << endl;
        	cout << "Func3: " << decoded_inst.func3 << endl;
        	cout << "immediate: " << decoded_inst.immediate << endl;
        	cout << "rs1: " << decoded_inst.rs1 << endl;
        	cout << "rs2: " << decoded_inst.rs2 << endl;
        	cout << "rd: " << decoded_inst.rd << endl;
		
		*/

		int imm_bits = (decoded_inst.immediate >> 5) & 0x3F;
		switch(decoded_inst.opcode){
			case 0x13: 
			//cout << "I-Type Instruction" << endl;
			switch(decoded_inst.func3){
				case 0x0: //addi
					regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] + decoded_inst.immediate;
					cout << "addi: " << decoded_inst.func3 << endl;
					cout << "Result: " << regfile[decoded_inst.rd] << endl;
					cout << "PC: " << &pc << endl;
					cout << "PCC: " << *pc_ptr << endl;
					break;
				case 0x4: // xori
					regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] ^ decoded_inst.immediate;
					cout << "xori: " << decoded_inst.func3 << endl;
					cout << "Result: " << regfile[decoded_inst.rd] << endl;
					cout << "PC: " << &pc << endl;
					cout << "PCC: " << *pc_ptr << endl;
					break;
				case 0x6: // ori
					regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] | decoded_inst.immediate;
					cout << "ori: " << decoded_inst.func3 << endl;
					cout << "Result: " << regfile[decoded_inst.rd] << endl;
					cout << "PC: " << &pc << endl;
					cout << "PCC: " << *pc_ptr << endl;
					break;
				case 0x7: // andi
					regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] & decoded_inst.immediate;
					cout << "andi: " << decoded_inst.func3 << endl;
					cout << "Result: " << regfile[decoded_inst.rd] << endl;
					cout << "PC: " << &pc << endl;
					cout << "PCC: " << *pc_ptr << endl;
					break;
				case 0x1: // slli
					regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] << imm_bits;
					cout << "slli: " << decoded_inst.func3 << endl;
					cout << "Result: " << regfile[decoded_inst.rd] << endl;
					cout << "PC: " << &pc << endl;
					cout << "PCC: " << *pc_ptr << endl;
					break;
				case 0x5: // srli
					regfile[decoded_inst.rd] = regfile[decoded_inst.rs1] >> imm_bits;
					cout << "srli: " << decoded_inst.func3 << endl;
					cout << "Result: " << regfile[decoded_inst.rd] << endl;
					cout << "PC: " << &pc << endl;
					cout << "PCC: " << *pc_ptr << endl;
					break;
				default:
					cout << "TDB" << endl;
					break;
					
			}
			break;
			//cout << "Opcode: " << decoded_inst.opcode << endl;
			
			case 0x33:
				cout << "R-Type Instruction" << endl;
		}
		pc ++;
	}

	return 0;
}


