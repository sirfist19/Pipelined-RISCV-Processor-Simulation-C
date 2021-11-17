#ifndef myCPU_h
#define myCPU_h
#include <string>
#include <fstream>
#include <iostream>

const int main_mem_size = 4096;

//an enum (or a glorified int really) for instruction types
enum inst_types
{
	and_, or_, add, sub, andi, ori, addi, lw, sw, no_op, error
};

//a struct that writeback sends to the decode stage such that writebacks do not happen
//before reads if that is the wrong order
//Essentially this implements HALF CYCLES
struct wb_info
{
	bool needed;
	unsigned int rd;
	int final_res;

	wb_info()//empty constructor
	{
		needed = false;
		rd = 0;
		final_res = 0;
	}
};
class MainMemory
{
private:
	unsigned char main[main_mem_size];//this is the data of main mem
public:
	void printMem()//for debeugging
	{
		for (int i = 0; i < main_mem_size; i+=16)//prints out 16 bytes of memory per line
		{
			printf("%d  ", main[i]);
			printf("%d  ", main[i + 1]);
			printf("%d  ", main[i + 2]);
			printf("%d  ", main[i + 3]);
			printf("%d  ", main[i + 4]);
			printf("%d  ", main[i + 5]);
			printf("%d  ", main[i + 6]);
			printf("%d  ", main[i + 7]);
			printf("%d  ", main[i + 8]);
			printf("%d  ", main[i + 9]);
			printf("%d  ", main[i + 10]);
			printf("%d  ", main[i + 11]);
			printf("%d  ", main[i + 12]);
			printf("%d  ", main[i + 13]);
			printf("%d  ", main[i + 14]);
			printf("%d  ", main[i + 15]);
			std::cout << std::endl;
		}
	}
	void printCurMem(unsigned int address)//prints one line of memory only
	{
		unsigned int cur = read(address);
		printf("%x", cur);
		std::cout << std::endl;
	}
	unsigned char get(unsigned int i)//gets a byte at a time
	{
		return main[i];
	}
	int read(unsigned int address)
		//gets the current 4 bytes by the address
	{
		unsigned int bits0to8 = get(address + 3) << 24;
		unsigned int bits8to16 = get(address + 2) << 16;
		unsigned int bits16to24 = get(address + 1) << 8;
		unsigned int bits24to32 = get(address + 0);
		//the above lines read unsigned chars from memory and left shift in order to form parts
		//of the needed instruction

		unsigned int temp = bits16to24 | bits24to32;
		unsigned int temp2 = bits8to16 | bits0to8;
		int cur_mem = temp | temp2;
		//these portions are ored together in order to make a complete instruction

		return cur_mem;
	}
	void write(unsigned int address, int val)
	{
		//std::cout << "VAL: " << val << std::endl;
		//does the opposite of read ...
		//meaning this function take an int and splits it up into chars based on each
		//of its 8 bits and then stores this in memory
		unsigned char n1 = (unsigned char) (val & 0x000000ff);
		unsigned char n2 = (unsigned char) (val & 0x0000ff00) << 8;
		unsigned char n3 = (unsigned char) (val & 0x00ff0000) << 16;
		unsigned char n4 = (unsigned char)(val & 0xff000000) << 24;
		main[address] = n1;
		main[address + 1] = n2;
		main[address + 2] = n3;
		main[address + 3] = n4;
	}
	
	MainMemory()//empty constructor
	{
		//simply assign all of the main array to be zeros
		for (int i = 0; i < main_mem_size; i++)
		{
			main[i] = 0x00;
		}
	}
	~MainMemory()//empty destructor
	{
	}
};
class InstMemory
{
private:
	unsigned char main[4096];//the data
public:
	void printMem()//for debugging
	{
		for (int i = 0; i < 4096; i++)
		{
			//std::cout << testint[i] << std::endl;
			//std::cout << main[i] << std::endl;
			printf("%x", main[i]);
			std::cout << std::endl;
		}
	}
	void printCurInst(unsigned int pc)//for debugging
	{
		unsigned int cur = get_cur_inst(pc);
		printf("%x", cur);
		std::cout << std::endl;
	}
	unsigned char get(unsigned int i)//get a byte at a time
	{
		return main[i];
	}
	unsigned int get_cur_inst(unsigned int pc)
		//gets the current instruction specified by pc
	{
		unsigned int bits0to8 = get(pc+3) << 24;
		unsigned int bits8to16 = get(pc + 2) << 16;
		unsigned int bits16to24 = get(pc + 1) << 8;
		unsigned int bits24to32 = get(pc + 0);

		unsigned int temp = bits16to24 | bits24to32;
		unsigned int temp2 = bits8to16 | bits0to8;
		unsigned int cur_inst = temp | temp2;
		
		return cur_inst;
	}

	//constructor first
	InstMemory()//empty constructor
	{
		//simply assign all of the main array to be zeros
		for (int i = 0; i < 4096; i++)
		{
			main[i] = 0x00;
		}
	}

	//a non-empty constructor that takes in a file name and reads that file.
	//each line is a string that is first converted to an int and then to a one byte char
	InstMemory(std::string file)
	{
		//feed in the data from file
		std::ifstream loadFile(file);//load in the new file
		if (!loadFile)//if file loading fails
		{
			std::cout << "ERROR READING FILE CODE 1\n";
			exit(1);
		}
		std::string text;
		int index = 0;
		while (std::getline(loadFile, text))
		{
			std::string::size_type sz;   // alias of size_t
			if (text != "")
			{
				int cur = std::stoi(text, &sz);//cur is now an int
				//std::cout << cur << std::endl;
				unsigned char single = cur;
				main[index] = single;
				index++;
			}
		}

		//fill the rest of memory with zeros (once the lines of the file runs out)
		for (int i = index; i < 4096; i++)
		{
			main[i] = 0x00;
		}
	}
	~InstMemory()//empty destructor
	{
	}
};
class Controller
{
public:
	//the only thing the controller really contains is the controller signals and functions
	//these variables are never set in stone for the CPU's controller... 
	//but in contrast the returnCurrentCtrlSignals() function returns an object which is fed into
	//the pipeling registers for later use

	//fetch stage signals

	//decode stage signals
	int RegWrite; //controls the Register file and when it is written or read

	//exec stage signals
	int ALUSrc;//0 or 1 ... dictates whether or not and imm is fed into the ALU
	inst_types ALUOp; //control for the ALU

	//mem stage signals
	int MemWrite;
	int MemRead;

	//wb stage signals
	int MemToReg;//dictates whether or not the quantity read from memory will go to the registers

	Controller()//empty constructor to prevent uninitialization of class objects
	{
		//set all signals to 0 originally
		RegWrite = 0;
		ALUSrc = 0;
		ALUOp = inst_types::error;//this is default to 9
		MemWrite = 0;
		MemRead = 0;
		MemToReg = 0;
	}

	//figures out the type of the instruction and returns it so it can be used later
	inst_types decodeInstruction(unsigned int inst)
	{
		//this is where the instruction is actually decoded!
		inst_types type = inst_types::error;//this will be set once the instruction is decoded

		//first determine the inst type (r, i, ...) by looking at the opcode
		//the last 7 bits
		unsigned int opcode = inst & 0x0000007f;//isolates the last 7 bits
		unsigned int func3 = (inst & 0x00007000) >> 12;//isolate the next 3 bits

		//The value of all f's is assigned to make sure that I know the values are uninitialized
		unsigned int func7 = 0xffffffff;//only for r-type

		//FOR DEBUGGING
		//printf("Opcode: %x\n", opcode);
		//printf("rd: %x\n", rd);
		//printf("func3: %x\n", func3);
		//printf("rs1: %x\n", rs1);
		//printf("Opcode: %x", opcode);

		if (opcode == 51)//for r-type
		{
			func7 = (inst & 0xfe000000) >> 25;
			//printf("rs2: %x\n", rs2);
			//printf("func7: %x\n", func7);

			if (func7 == 32)
				type = inst_types::sub;
			else if (func3 == 0)
				type = inst_types::add;
			else if (func3 == 7)
				type = inst_types::and_;
			else if (func3 == 6)
				type = inst_types::or_;
			else
				std::cout << "Error: Instruction type not recognized 2.\n";
		}
		else if (opcode == 19)//for i-type
		{
			//imm = ((int)(inst & 0xfff00000)) >> 20;
			//printf("imm: %x\n", imm);
			//std::cout << "IMM in controller in decode: " << imm << std::endl;
			
			if (func3 == 0)
				type = inst_types::addi;
			else if (func3 == 7)
				type = inst_types::andi;
			else if (func3 == 6)
				type = inst_types::ori;
			else
				std::cout << "Error: Instruction type not recognized 3.\n";
		}
		else if (opcode == 3)//for lw
		{
			type = inst_types::lw;
		}
		else if (opcode == 35)//for sw
		{
			type = inst_types::sw;
		}
		else if (inst == 0)//for no_ops after the file ends
		{
			type = inst_types::no_op;
		}
		else
		{
			std::cout << "Error: Instruction type not recognized 4.\n";
		}
		
		return type;
	}

	//returns a pointer to a Controller object that contains all the control signals
	//needed for the rest of the pipeline stages
	//this object when returned with be passed through the pipeline registers
	Controller* returnCurrentCtrlSignals(inst_types type)
	{
		Controller* cur = new Controller();

		//control signals depend of the type of instruction
		switch (type)
		{
			case inst_types::add:
				cur->RegWrite = 1;//adds next to write to register
				cur->MemWrite = 0;
				cur->MemRead = 0;
				cur->ALUSrc = 0;//ALUSrc of 0 makes rs2 (or val2) be read
				cur->ALUOp = inst_types::add;//all r-type have ALUOp of 2
				cur->MemToReg = 0;//do not read from memory
				break;
			case inst_types::addi:
				cur->RegWrite = 1;
				cur->MemWrite = 0;
				cur->MemRead = 0;
				cur->ALUSrc = 1;
				cur->ALUOp = inst_types::addi;//ALUSrc of 1 makes the imm be read
				cur->MemToReg = 0;//do not read from memory
				break;
			case inst_types::or_:
				cur->RegWrite = 1;
				cur->MemWrite = 0;
				cur->MemRead = 0;
				cur->ALUSrc = 0;//ALUSrc of 0 makes rs2 (or val2) be read
				cur->ALUOp = inst_types::or_;//all r-type have ALUOp of 2
				cur->MemToReg = 0;//do not read from memory
				break;
			case inst_types::ori:
				cur->RegWrite = 1;
				cur->MemWrite = 0;
				cur->MemRead = 0;
				cur->ALUSrc = 1;//ALUSrc of 1 makes the imm be read
				cur->ALUOp = inst_types::ori;
				cur->MemToReg = 0;//do not read from memory
				break;
			case inst_types::and_:
				cur->RegWrite = 1;
				cur->MemWrite = 0;
				cur->MemRead = 0;
				cur->ALUSrc = 0;//ALUSrc of 0 makes rs2 (or val2) be read
				cur->ALUOp = inst_types::and_;//all r-type have ALUOp of 2
				cur->MemToReg = 0;//do not read from memory
				break;
			case inst_types::sub:
				cur->RegWrite = 1;
				cur->MemWrite = 0;
				cur->MemRead = 0;
				cur->ALUSrc = 0;//ALUSrc of 0 makes rs2 (or val2) be read
				cur->ALUOp = inst_types::sub;//all r-type have ALUOp of 2
				cur->MemToReg = 0;//do not read from memory
				break;
			case inst_types::andi:
				cur->RegWrite = 1;
				cur->MemWrite = 0;
				cur->MemRead = 0;
				cur->ALUSrc = 1;//ALUSrc of 1 makes the imm be read
				cur->ALUOp = inst_types::andi;
				cur->MemToReg = 0;//do not read from memory
				break;
			case inst_types::lw:
				cur->RegWrite = 1;
				cur->MemWrite = 0;
				cur->MemRead = 1;//lw needs to read from memory
				cur->ALUSrc = 1;//ALUSrc of 1 makes the imm be read
				cur->ALUOp = inst_types::lw;//means func3 and 7 need to be checked
				cur->MemToReg = 1;//do read from memory!
				break;
			case inst_types::sw:
				cur->RegWrite = 0;//store word does not write to registers
				cur->MemWrite = 1;
				cur->MemRead = 0;
				cur->ALUSrc = 1;//we do want the imm for sw
				cur->ALUOp = inst_types::sw;
				cur->MemToReg = 0;//do not read from memory
				break;
			case inst_types::no_op://set everything to 0
				cur->RegWrite = 0;//store word does not write to registers
				cur->MemWrite = 0;
				cur->MemRead = 0;
				cur->ALUSrc = 0;
				cur->ALUOp = inst_types::no_op;
				cur->MemToReg = 0;//do not read from memory
				break;

			//case inst_types::beq:
				//cur->ALUOp = 1; //for later
			default:
				std::cout << "Error: Unrecognized instruction type 1.\n";
				break;
		}
		return cur;
	}
};
class RegisterFile
{
private:
	unsigned int x[32];//the data
public:
	RegisterFile()//empty constructor
	{
		for (int i = 0; i < 32; i++)
		{
			x[i] = 0;//just make everything zero to start with
		}
	}
	void printAll()//for debugging
	{
		for (int i = 0; i < 32; i++)
		{
			std::cout << "x" << i << ": " << x[i] << std::endl;
		}
	}
	void writeReg(int index, unsigned int val)//write to the register using the given index
											  //and set the register to val
	{
		x[index] = val;
	}
	int readReg(int index)//read from a register and the given index
	{
		return x[index];
	}
};
class ALU
{
	//this is really just a collection of fxns ... doesn't need to store anything
public:
	ALU()//empty constructor
	{

	}
	int ALU_caller(unsigned int control, int val1, int val2)
		//calls the correct function based on the control input and calls that function with
		//the values of val1 and val2

		//after calling the correct function, this function returns the ALU result
	{
		int res = 1000;//default value for error checking
		switch (control)
		{
			case 0:
				res = add(val1, val2);
				break;
			case 1:
				res = and_(val1, val2);
				break;
			case 2:
				res = or_(val1, val2);
				break;
			case 3:
				res = sub(val1, val2);
				break;
			default: 
				//ERROR ... res will still be 1000 which is an error
				break;
		}
		return res;
	}
	//operations
	int add(int val1, int val2)
	{
		return (val1 + val2);
	}
	int and_(int val1, int val2)
	{
		return (val1 & val2);
	}
	int sub(int val1, int val2)
	{
		return (val1 - val2);
	}
	int or_(int val1, int val2)
	{
		return (val1 | val2);
	}
};
class ALUController
{
public:
	//simply returns a control signal based off of the type of instruction
	//func3, func7, and opcode were previously used in the control unit to figure out what
	//type of instruction the current instruction was
	unsigned int returnALUCtrlSig(inst_types type)
	{
		switch (type)
		{
			case inst_types::add:
			case inst_types::addi:
			case inst_types::lw:
			case inst_types::sw:
			case inst_types::no_op:
				return 0;
				break;
			case inst_types::andi:
			case inst_types::and_:
				return 1;
				break;
			case inst_types::or_:
			case inst_types::ori:
				return 2;
				break;
			case inst_types::sub:
				return 3;
				break;
			default:
				std::cout << "Instruction tpye not recognized in the ALUController.\n";
				return 100;//ERROR
				break;
		}
	}
};

//HERE ARE THE DEFINITIONS FOR THE BATON OR PIPELINE REGISTER STRUCTURES
//Each pipeling register contains the signals and values needed for the next stage
//Each pipline register also contains an empty constructor that sets default value so that 
//memory that needs to be accessed is not uninitialized.

//the isValid bool is set to false to start but when the fetch, decode, execute, memory, or 
//writeback stages are run the isValid bool is set to true
struct batonFetch
{
	bool isValid;
	unsigned int inst;
	unsigned int pc;

	batonFetch()
	{
		isValid = false;
		inst = 0;
		pc = 0;
	}
};
struct batonDecode
	//what's passed from Decode to Execute
{
	bool isValid;
	Controller* control_sigs;//holds the current control values
	unsigned int val1;//the value that comes from rs1
	unsigned int val2;//the value that comes from rs2
	unsigned int rd;//index needed for write register later
	int imm;//the extended 32 bit immediate value

	//needed as input for the ALUControl
	unsigned int func3; 
	unsigned int func7;

	batonDecode()
	{
		isValid = false;
	}
};
struct batonExec
{
	//what's passed from Execute to Mem
	bool isValid;
	Controller* control_sigs;
	unsigned int val2;//this is the register not the imm
	int res_ALU;//the resulting value from the ALU
	unsigned int rd;

	batonExec()
	{
		isValid = false;
	}
};
struct batonMem
{
	//what's passed from Mem to WB
	bool isValid;
	Controller* control_sigs;
	int dataMemRead;//the value coming from data memory
	int res_ALU;//the result of the ALU
	unsigned int rd;

	batonMem()
	{
		isValid = false;
	}
};
class CPU//here is the CPU class which contains most critical parts
{
private:
	//data contents
	unsigned int pc;
	InstMemory* inst_mem;
	MainMemory* main_mem;
	RegisterFile* regFile;
	Controller* control;
	ALU* alu;
	ALUController* aluControl;
public:
	CPU(std::string file)//constructor with file ... creates dynamic variables for major parts
	{
		pc = 0;
		inst_mem = new InstMemory(file);
		main_mem = new MainMemory();
		regFile = new RegisterFile();
		control = new Controller();
		alu = new ALU();
		aluControl = new ALUController();
	}
	~CPU()//empty constructor that deletes dynamic variables
	{
		delete inst_mem;
		delete regFile;
		delete control;
		delete alu;
		delete aluControl;
		delete main_mem;
	}

	//THE FETCH STAGE
	batonFetch fetch()//fetches the instruction at pc and increments pc
	{
		unsigned int cur = inst_mem->get_cur_inst(pc);//grabs the current instruction from the
													  //instruction memory using pc
		
		//printf("%x", cur);
		//std::cout << std::endl;

		inc_pc();//increment pc

		//return stuff
		batonFetch baton;
		baton.isValid = true;
		baton.inst = cur;
		baton.pc = pc;
		return baton;
	}

	//THE DECODE STAGE
	batonDecode decode(batonFetch& cur, wb_info& writeback)
	{
		//decodes the instruction and returns the instruction type (Ex: add, or, ...)
		//does this through the control unit
		inst_types type = control->decodeInstruction(cur.inst);
		
		//return the current control signals based on the type of instruction
		Controller* cur_controls = control->returnCurrentCtrlSignals(type);
		
		//the rs1 and rs2 values from the 32-bit instruction
		unsigned int rs1 = (cur.inst & 0x000f8000) >> 15;
		unsigned int rs2 = (cur.inst & 0x01f00000) >> 20;

		//rs1 and rs2 are used as indexes in order to read the given register file
		unsigned int val1 = regFile->readReg(rs1);
		unsigned int val2 = regFile->readReg(rs2);

		batonDecode baton;
		baton.isValid = true;
		baton.val1 = val1;
		baton.val2 = val2;
		baton.rd = (cur.inst & 0x00000f80) >> 7;//gets rd from the current instruction

		if (type == inst_types::sw)//if the instruction is a store word than the immediate is different
		{
			//the top and bottom part of the imm (this sign extends by casting to an int)
			int topbits = ((int)(cur.inst & 0xfe000000)) >> 20;
			int bottombits = ((int)(cur.inst & 0x00000f80)) >> 7;

			//place the top and bottom parts of the imm back together and store in baton.imm
			baton.imm = topbits | bottombits;
		}
		else//for all other instructions the immediate is the same (if the instruction has an immediate
		{
			baton.imm = ((int)(cur.inst & 0xfff00000)) >> 20;//this is only valid if it is an i-type inst
		}
		
		//std::cout << "IMM from decode stage: " << baton.imm << std::endl;
		baton.func3 = (cur.inst & 0x00007000) >> 12;
		baton.func7 = (cur.inst & 0xfe000000) >> 25;
		baton.control_sigs = cur_controls;//pass the same control unit from before


		//STUFF FOR THE WB STAGE (forwarded over in the same cycle)
		//this is the half-cycle portion
		if (writeback.needed)
		{
			regFile->writeReg(writeback.rd, writeback.final_res);
			writeback.needed = false;//reset the writeback obj
		}
		return baton;
	}
	
	//THE EXECUTE STAGE
	batonExec execute(batonDecode& cur)
	{
		//gets the control signals needed for the ALU from ALU control using ALU op
		unsigned int aluControlSig = aluControl->returnALUCtrlSig(cur.control_sigs->ALUOp);
		//std::cout << "ALUControlSig: " << aluControlSig << std::endl;
		
		//makes val2 either the immediate or the rs2 value
		int val2 = exec_mux(cur.control_sigs->ALUSrc, cur.imm, cur.val2);

		//stores the result of the ALU computation in res
		unsigned int res = alu->ALU_caller(aluControlSig, cur.val1, val2);

		//the pipeline register for the next stage
		batonExec baton;
		baton.isValid = true;
		baton.control_sigs = cur.control_sigs;
		baton.val2 = cur.val2;
		baton.res_ALU = res;
		baton.rd = cur.rd;

		return baton;
	}
	
	//a function representing the multiplexer in the execute stage
	//returns either the immediate or value from rs2 register based on the ALUSrc value
	int exec_mux(unsigned int ALUSrc, unsigned int imm, unsigned int val2)
	{
		switch (ALUSrc)
		{
			case 0:
				return val2;
				break;
			case 1:
				return imm;
				break;
			default:
				std::cout << "ERROR: ALUSRC value invalid for exec mux";
				return 50;
				break;
		}
	}

	//THE MEMORY STAGE
	batonMem memory_Stage(batonExec& cur)
	{
		//getting control signals
		int memRead = cur.control_sigs->MemRead;
		int memWrite = cur.control_sigs->MemWrite;
		int dataMemRead = 0;//default the value to 0

		if (memWrite == 1)//write to memory with the address from the ALU and the value from rs2
		{
			main_mem->write(cur.res_ALU, cur.val2);
		}
		if (memRead == 1)//read from memory at the current address
		{
			dataMemRead = main_mem->read(cur.res_ALU);
		}

		//pipeline register for the writeback stage
		batonMem baton;
		baton.isValid = true;
		baton.control_sigs = cur.control_sigs;
		baton.rd = cur.rd;
		baton.res_ALU = cur.res_ALU;
		baton.dataMemRead = dataMemRead;
		return baton;
	}

	//a function representing the multiplexer in the writeback stage
	//returns either the value read from data memory or value from the ALU
	//based on the MemToReg value
	int wb_mux(unsigned int MemToReg, unsigned int dataMemRead, unsigned int res_ALU)
	{
		switch (MemToReg)
		{
		case 0:
			return res_ALU;
			break;
		case 1:
			return dataMemRead;
			break;
		default:
			std::cout << "ERROR: Value invalid for wb mux";
			return 50;
			break;
		}
	}
	
	//THE WRITEBACK STAGE
	//returns a stuct of type wb_info that is utilized in the same cycle during the decode
	//stage in order to implement half cycles
	wb_info writeback(batonMem& cur)
	{
		wb_info writeback;
		int final_res = wb_mux(cur.control_sigs->MemToReg, cur.dataMemRead, cur.res_ALU);

		//if the CPU is supposed to write to registers
		if (cur.control_sigs->RegWrite == 1)
		{
			//write to the register file
			//if rd is 0 it means that 0 is being written to ... don't allow that
			if (cur.rd != 0)
			{
				//instead of writing back here do it later by passing an object to the 
				//decode stage later
				writeback.final_res = final_res;
				writeback.rd = cur.rd;
				writeback.needed = true;
			}	
		}
		return writeback;
	}

	//general getter functions for the CPU data
	InstMemory* getInstrMem()
	{
		return inst_mem;
	}
	MainMemory* getMainMem()
	{
		return main_mem;
	}
	RegisterFile* getRegFile()
	{
		return regFile;
	}
	Controller* getController()
	{
		return control;
	}
	unsigned int get_pc()
	{
		return pc;
	}

	//incements the pc by 4
	//this is called once a cycle in the fetch stage
	void inc_pc()
	{
		pc += 4;
	}

};
#endif
