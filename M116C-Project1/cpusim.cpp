#include "myCPU.h"//this started as having <> , is it right to change it?
#include <iostream>
#include <fstream>
#include <string>

/*
Add all the required standard and developed libraries here.
Remember to include all those files when you are submitting the project.
*/

/*
Put/Define any helper function/definitions you need here
*/
void updateAmtZeroInst(unsigned int inst, int& amt_zero_inst)
//counts the number of times an instruction with an opcode of zero is seen
{
	if(inst == 0)
	{
		amt_zero_inst++;
	}
	else
	{
		amt_zero_inst = 0;
	}
}
bool isLoop(int& amt_zero_inst)//evaluates whether the while loop should continue running
							   //based on the amount of zero instructions that have been seen
{
	if (amt_zero_inst >= 5)//if there are less than 5 zero instructions than keep fetching
	{
		return false;
	}
	return true;
}

int main(int argc, char* argv[]) // your project should be executed like this: ./cpusim filename.txt and should print (a0,a1) 
{
	/* This is the front end of your project.
	You need to first read the instructions that are stored in a file and load them into an instruction memory.
	*/
	/* Define your Instruction memory here. Each cell should store 1 byte.
	You can define the memory either dynamically, or define it as a fixed size
	with size 4MB (i.e., 4096 lines). Each instruction is 32 bits
	(i.e., 4 lines, saved in little-endian mode).

	Each line in the input file is stored as an unsigned char
	and is 1 byte (each four lines are one instruction). You need
	to read the file line by line and store it into the memory.
	You may need a mechanism to convert these values to bits
	so that you can read opcodes, operands, etc.
	*/

	//THE FILE TO READ
	//std::string input_file = "instMem-r.txt";//start with loading this one
	//std::string input_file = "Aidan_Test.txt";//my own test file
	//std::string input_file = "instMem-ms.txt";
	//std::string input_file = "instMem-t.txt";
	std::string input_file = argv[1];//read in a file from stdin to main
	
	
	//HERE THE CPU IS INSTANTIATED
	//The CPU holds many things inside itself including pc, instruction memory,
	//the main memory, the register file, controller, ALU, and ALU controller.
	CPU myCPU(input_file);//instantiates the instruction memory inside the CPU

	// Clock
	unsigned int myClock = 0; // data-types can be changed! This is just a suggestion. 
	//NOTE: The pc is built into the CPU class

	//Here are the baton objects ... 
	//these are structures that hold the relevant data between stages
	//these are the pipeling registers!
	//the naming convention goes as follows:
	//is the register is called regStageX, then it lies between stage X and X+1
	//so regFetch is the register between fetch and decode
	batonFetch regFetch;//sent to decode
	batonDecode regDecode;//sent to exec
	batonExec regExec;//sent to Mem
	batonMem regMem;//sent to WB

	int amt_zero_inst = 0;
	int num_inst = 0;//counts the amount of instructions FOR DEBUGGING
	wb_info writeback;//used in order to implement the half-cycle for wb and decode
					  //holds whether or not a value needs to be written back in the decode stage
					  //and the value that needs to be written back

	
	while (isLoop(amt_zero_inst)) // processor's main loop. Each iteration is equal to one clock cycle.  
	{
		//NOTE: INSTRUCTIONS ARE IMPLEMENTED BACKWARDS IN ORDER TO FACILITATE PIPLINING
		//The isValid part is only false when the pipeling register has never been filled
		//After it is filled it will be set to true and that given stage will run

		// writeback
		if(regMem.isValid)
			writeback = myCPU.writeback(regMem);

		// memory
		if(regExec.isValid)
			regMem = myCPU.memory_Stage(regExec);
		//std::cout << "DATA_MEM: " << regMem.dataMemRead<< std::endl;

		// execute
		if(regDecode.isValid)
			regExec = myCPU.execute(regDecode);
		//std::cout <<"RES_ALU: "<< regExec.res_ALU << std::endl;

		// decode
		if(regFetch.isValid)
			regDecode = myCPU.decode(regFetch, writeback);//passing in writeback let's the 
														  //CPU know if it needs to writeback
														  
		//fetch
		regFetch = myCPU.fetch();
		updateAmtZeroInst(regFetch.inst, amt_zero_inst);
		
		if (regFetch.inst != 0)//incrementing the number of instructions
		{
			num_inst++;
		}

		//FOR DEBUGGING
		//printf("%x", regFetch.inst);
		//std::cout << std::endl;

		// _next values should be written to _current values here:
		//NOTE: this is done above using the baton... structures

		//increment clock and the PC
		myClock += 1;
		
		//myPC += 4; // for now we can assume that next PC is always PC + 4
		//NOTE: the pc is updated in the CPU class each cycle
		
		// we should break the loop if ALL instructions in the pipelinehas opcode==0 instruction 
		//NOTE: this is done via the amt_zero_inst above
	}

	//FOR DEBUGGING
	
	/*std::cout <<"Total number of cycles: "<< myClock<<std::endl;
	std::cout <<"Number of Instructions run: "<< num_inst<<std::endl;

	std::cout << "REGISTER FILE:\n";
	myCPU.getRegFile()->printAll();

	std::cout << "MAIN MEM:\n";
	myCPU.getMainMem()->printMem();*/

	//print out a0 and a1
	int a0 = myCPU.getRegFile()->readReg(10);
	int a1 = myCPU.getRegFile()->readReg(11);
	std::cout << "(" << a0 << "," << a1 << ")";

	// clean up the memory (if any)
	delete regDecode.control_sigs;
	// print the stats
	return 0;
}


