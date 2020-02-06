#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>

using namespace std;
namespace fs = filesystem;

fs::path inputFilePath;
bool directory = false;
fs::path outputFilePath;
vector<fs::path> inputFilePaths;

string arithCommands[9] = { "add", "sub", "neg", "eq", "gt", "lt", "and", "or", "not" };

enum class command {
	C_ARITHMETIC,
	C_PUSH, C_POP,
	C_LABEL, C_GOTO,
	C_IF,
	C_FUNCTION,
	C_RETURN,
	C_CALL
};

class parserModule {
private:
	bool hasMoreCommands = true;
	command commandType;
	string arg1;
	int arg2;
	ifstream inputFile;
	string line;
	int currentLine;
public:
	parserModule(fs::path _inputFilePath) {
		inputFile = ifstream(_inputFilePath);
		if (!inputFile.is_open()) {
			cerr << _inputFilePath.string() << " could not be opened!";
			exit(1);
		}
	}

	void advance() {
		if (!hasMoreCommands) return;
		if (getline(inputFile, line)) {
			currentLine++;

			line = line.substr(0, line.find("//"));

			if (line == "") {
				advance();
			}

			if (line.find("\t") != string::npos) {
				line.replace(line.find("\t"), 2, "");
			}

			int firstSpace = line.find_first_of(' ');
			int secondSpace = line.substr(firstSpace + 1).find_first_of(' ');

			string firstSegment = line.substr(0, firstSpace);

			if (firstSegment == "push") {
				commandType = command::C_PUSH;
			}
			else if (firstSegment == "pop") {
				commandType = command::C_POP;
			}
			else if (find(begin(arithCommands), end(arithCommands), firstSegment) != end(arithCommands)) {
				commandType = command::C_ARITHMETIC;
				arg1 = firstSegment;
			}
			else if (firstSegment == "label") {
				commandType = command::C_LABEL;
			}
			else if (firstSegment == "goto") {
				commandType = command::C_GOTO;
			}
			else if (firstSegment == "if-goto") {
				commandType = command::C_IF;
			}
			else if (firstSegment == "function") {
				commandType = command::C_FUNCTION;
			}
			else if (firstSegment == "return") {
				commandType = command::C_RETURN;
				arg1 = firstSegment;
			}
			else if (firstSegment == "call") {
				commandType = command::C_CALL;
			}

			if (commandType != command::C_ARITHMETIC && commandType != command::C_RETURN) {
				string secondSegment = line.substr(firstSpace + 1, secondSpace);
				arg1 = secondSegment;

				if (commandType == command::C_PUSH || commandType == command::C_POP || commandType == command::C_FUNCTION || commandType == command::C_CALL) {
					string thirdSegment = line.substr(firstSpace + 2).substr(secondSpace);

					for (auto chr : thirdSegment) {
						if (!isdigit(chr) && chr != ' ') {
							cerr << "Error on line: " << currentLine << endl;
							exit(1);
						}
					}

					arg2 = stoi(thirdSegment);
				}
			}
		}
		else {
			hasMoreCommands = false;
		}
	}

	bool getHasMoreCommands() {
		return hasMoreCommands;
	}

	string getArg1() {
		if (commandType == command::C_RETURN) return "-1";
		return arg1;
	}

	int getArg2() {
		if (commandType == command::C_PUSH || commandType == command::C_POP
			|| commandType == command::C_FUNCTION || commandType == command::C_CALL) {

			return arg2;
		}
		else {
			return -1;
		}
	}

	command getCommandType() {
		return commandType;
	}
};

class codeWriter {
private:
	ofstream outputFile;
	int segmentAddress;
	string currentVMFile;
	int eqs, gts, lts;
	vector<string> callStack;
public:
	codeWriter(fs::path _outputFilePath) {

		//---------------------------------------
		currentVMFile = _outputFilePath.string();
		//---------------------------------------

		for (int i = 0; i < currentVMFile.length(); i++) {
			if (isalpha(currentVMFile[i])) {
				currentVMFile = currentVMFile.substr(i);
				break;
			}
		}
		currentVMFile = currentVMFile.substr(0, currentVMFile.find_first_of("."));

		outputFile = ofstream(_outputFilePath);
		if (!outputFile.is_open()) {
			cerr << outputFilePath.string() << " could not be created/opened!";
			exit(1);
		}
	}

	void setFileName(fs::path _currentVMFile) {
		currentVMFile = _currentVMFile.string();
	}

	void writeArithmetic(string command) {
		if (command == "add") {
			outputFile << "@0" << endl;
			outputFile << "A=M" << endl;
			outputFile << "A=A-1" << endl;
			outputFile << "D=M" << endl;
			outputFile << "A=A-1" << endl;
			outputFile << "M=D+M" << endl;
			outputFile << "@0" << endl;
			outputFile << "M=M-1" << endl;
		}
		else if (command == "sub") {
			outputFile << "@0" << endl;
			outputFile << "A=M" << endl;
			outputFile << "A=A-1" << endl;
			outputFile << "D=M" << endl;
			outputFile << "A=A-1" << endl;
			outputFile << "M=M-D" << endl;
			outputFile << "@0" << endl;
			outputFile << "M=M-1" << endl;
		}
		else if (command == "neg") {
			outputFile << "@0" << endl;
			outputFile << "A=M" << endl;
			outputFile << "A=A-1" << endl;
			outputFile << "M=-M" << endl;
		}
		else if (command == "eq") {
			outputFile << "@0" << endl;
			outputFile << "M=M-1" << endl;
			outputFile << "A=M" << endl;
			outputFile << "D=M" << endl;
			outputFile << "A=A-1" << endl;
			outputFile << "D=M-D" << endl;
			outputFile << "@EQUAL" << eqs << endl;
			outputFile << "D;JEQ" << endl;
			outputFile << "@0" << endl;
			outputFile << "A=M-1" << endl;
			outputFile << "M=0" << endl;
			outputFile << "@DONEEQUAL" << eqs << endl;
			outputFile << "0;JMP" << endl;
			outputFile << "(EQUAL" << eqs << ")" << endl;
			outputFile << "@0" << endl;
			outputFile << "A=M-1" << endl;
			outputFile << "M=-1" << endl;
			outputFile << "(DONEEQUAL" << eqs << ")" << endl;
			eqs++;
		}
		else if (command == "gt") {
			outputFile << "@0" << endl;
			outputFile << "M=M-1" << endl;
			outputFile << "A=M" << endl;
			outputFile << "D=M" << endl;
			outputFile << "A=A-1" << endl;
			outputFile << "D=M-D" << endl;
			outputFile << "@GREATER" << gts << endl;
			outputFile << "D;JGT" << endl;
			outputFile << "@0" << endl;
			outputFile << "A=M-1" << endl;
			outputFile << "M=0" << endl;
			outputFile << "@DONEGREATER" << gts << endl;
			outputFile << "0;JMP" << endl;
			outputFile << "(GREATER" << gts << ")" << endl;
			outputFile << "@0" << endl;
			outputFile << "A=M-1" << endl;
			outputFile << "M=-1" << endl;
			outputFile << "(DONEGREATER" << gts << ")" << endl;
			gts++;
		}
		else if (command == "lt") {
			outputFile << "@0" << endl;
			outputFile << "M=M-1" << endl;
			outputFile << "A=M" << endl;
			outputFile << "D=M" << endl;
			outputFile << "A=A-1" << endl;
			outputFile << "D=M-D" << endl;
			outputFile << "@LESS" << lts << endl;
			outputFile << "D;JLT" << endl;
			outputFile << "@0" << endl;
			outputFile << "A=M-1" << endl;
			outputFile << "M=0" << endl;
			outputFile << "@DONELESS" << lts << endl;
			outputFile << "0;JMP" << endl;
			outputFile << "(LESS" << lts << ")" << endl;
			outputFile << "@0" << endl;
			outputFile << "A=M-1" << endl;
			outputFile << "M=-1" << endl;
			outputFile << "(DONELESS" << lts << ")" << endl;
			lts++;
		}
		else if (command == "and") {
			outputFile << "@0" << endl;
			outputFile << "A=M" << endl;
			outputFile << "A=A-1" << endl;
			outputFile << "D=M" << endl;
			outputFile << "A=A-1" << endl;
			outputFile << "M=M&D" << endl;
			outputFile << "@0" << endl;
			outputFile << "M=M-1" << endl;
		}
		else if (command == "or") {
			outputFile << "@0" << endl;
			outputFile << "A=M" << endl;
			outputFile << "A=A-1" << endl;
			outputFile << "D=M" << endl;
			outputFile << "A=A-1" << endl;
			outputFile << "M=M|D" << endl;
			outputFile << "@0" << endl;
			outputFile << "M=M-1" << endl;
		}
		else if (command == "not") {
			outputFile << "@0" << endl;
			outputFile << "A=M" << endl;
			outputFile << "A=A-1" << endl;
			outputFile << "M=!M" << endl;
		}
	}

	void writePushPop(command cmd, string segment, int index) {
		if (segment == "argument") {
			segmentAddress = 2;
		}
		else if (segment == "local") {
			segmentAddress = 1;
		}
		else if (segment == "static") {
			segmentAddress = 7;
		}
		else if (segment == "constant") {
			segmentAddress = -1;
		}
		else if (segment == "this") {
			segmentAddress = 3;
		}
		else if (segment == "that") {
			segmentAddress = 4;
		}
		else if (segment == "pointer") {
			segmentAddress = 6;
		}
		else if (segment == "temp") {
			segmentAddress = 5;
		}

		if (cmd == command::C_PUSH) {
			if (segmentAddress > 0) {
				if (segmentAddress == 6) {
					outputFile << "@3" << endl;
					outputFile << "D=A" << endl;
				}
				else if (segmentAddress == 5) {
					outputFile << "@5" << endl;
					outputFile << "D=A" << endl;
				}
				else if (segmentAddress == 7) {
					outputFile << "@" << currentVMFile << "." << index << endl;
					outputFile << "D=M" << endl;
					outputFile << "@0" << endl;
					outputFile << "A=M" << endl;
					outputFile << "M=D" << endl;
					outputFile << "@0" << endl;
					outputFile << "M=M+1" << endl;
					return;
				}
				else {
					outputFile << "@" << segmentAddress << endl;
					outputFile << "D=M" << endl;
				}

				outputFile << "@" << index << endl;
				outputFile << "D=D+A" << endl;
				outputFile << "A=D" << endl;
				outputFile << "D=M" << endl;
				outputFile << "@0" << endl;
				outputFile << "A=M" << endl;
				outputFile << "M=D" << endl;
				outputFile << "@0" << endl;
				outputFile << "M=M+1" << endl;
			}
			else {
				outputFile << "@" << index << endl;
				outputFile << "D=A" << endl;
				outputFile << "@0" << endl;
				outputFile << "A=M" << endl;
				outputFile << "M=D" << endl;
				outputFile << "@0" << endl;
				outputFile << "M=M+1" << endl;
			}
		}
		else if (cmd == command::C_POP) {
			if (segmentAddress > 0) {
				if (segmentAddress == 6) {
					outputFile << "@3" << endl;
					outputFile << "D=A" << endl;
				}
				else if (segmentAddress == 5) {
					outputFile << "@5" << endl;
					outputFile << "D=A" << endl;
				}
				else if (segmentAddress == 7) {
					outputFile << "@0" << endl;
					outputFile << "M=M-1" << endl;
					outputFile << "A=M" << endl;
					outputFile << "D=M" << endl;
					outputFile << "@" << currentVMFile << "." << index << endl;
					outputFile << "M=D" << endl;
					return;
				}
				else {
					outputFile << "@" << segmentAddress << endl;
					outputFile << "D=M" << endl;
				}

				outputFile << "@" << index << endl;
				outputFile << "D=D+A" << endl;
				outputFile << "@R15" << endl;
				outputFile << "M=D" << endl;
				outputFile << "@0" << endl;
				outputFile << "M=M-1" << endl;
				outputFile << "A=M" << endl;
				outputFile << "D=M" << endl;
				outputFile << "@R15" << endl;
				outputFile << "A=M" << endl;
				outputFile << "M=D" << endl;
			}
			else {
				return;
			}
		}
	}

	void writeInit() {
		outputFile << "@256" << endl;
		outputFile << "D=A" << endl;
		outputFile << "@SP" << endl;
		outputFile << "M=D" << endl;
		writeCall("Sys.init", 0);
	}

	void writeLabel(string label) {
		if (!callStack.empty()) {
			label = callStack.back() + ":" + label;
		}
		outputFile << "(" << label << ")" << endl;
	}

	void writeGoto(string label) {
		if (!callStack.empty()) {
			label = callStack.back() + ":" + label;
		}
		outputFile << "@" << label << endl;
		outputFile << "0;JMP" << endl;
	}

	void writeIf(string label) {
		if (!callStack.empty()) {
			label = callStack.back() + ":" + label;
		}
		outputFile << "@0" << endl;
		outputFile << "M=M-1" << endl;
		outputFile << "A=M" << endl;
		outputFile << "D=M" << endl;
		outputFile << "@EQUAL" << eqs << endl;
		outputFile << "D;JEQ" << endl;
		outputFile << "@" << label << endl;
		outputFile << "0;JMP" << endl;
		outputFile << "(EQUAL" << eqs << ")" << endl;
		eqs++;
	}

	void writeCall(string functionName, int numArgs) {
		//Push return address
		outputFile << "@" << functionName << ".returnAddress" << endl;
		outputFile << "D=A" << endl;
		outputFile << "@0" << endl;
		outputFile << "M=M+1" << endl;
		outputFile << "A=M-1" << endl;
		outputFile << "M=D" << endl;
		//Push LCL
		outputFile << "@LCL" << endl;
		outputFile << "D=M" << endl;
		outputFile << "@0" << endl;
		outputFile << "M=M+1" << endl;
		outputFile << "A=M-1" << endl;
		outputFile << "M=D" << endl;
		//Push ARG
		outputFile << "@ARG" << endl;
		outputFile << "D=M" << endl;
		outputFile << "@0" << endl;
		outputFile << "M=M+1" << endl;
		outputFile << "A=M-1" << endl;
		outputFile << "M=D" << endl;
		//Push THIS
		outputFile << "@THIS" << endl;
		outputFile << "D=M" << endl;
		outputFile << "@0" << endl;
		outputFile << "M=M+1" << endl;
		outputFile << "A=M-1" << endl;
		outputFile << "M=D" << endl;
		//Push THAT
		outputFile << "@THAT" << endl;
		outputFile << "D=M" << endl;
		outputFile << "@0" << endl;
		outputFile << "M=M+1" << endl;
		outputFile << "A=M-1" << endl;
		outputFile << "M=D" << endl;
		//Reposition ARG
		outputFile << "@0" << endl;
		outputFile << "D=M" << endl;
		outputFile << "@" << numArgs << endl;
		outputFile << "D=D-A" << endl;
		outputFile << "@5" << endl;
		outputFile << "D=D-A" << endl;
		outputFile << "@ARG" << endl;
		outputFile << "M=D" << endl;
		//Reposition LCL
		outputFile << "@0" << endl;
		outputFile << "D=M" << endl;
		outputFile << "@LCL" << endl;
		outputFile << "M=D" << endl;
		//Jump to function
		outputFile << "@" << functionName << endl;
		outputFile << "0;JMP" << endl;
		//Return Address label
		outputFile << "(" << functionName << ".returnAddress)" << endl;

		callStack.push_back(functionName);
	}

	void writeReturn() {
		//Set temporary frame variable
		outputFile << "@LCL" << endl;
		outputFile << "D=M" << endl;
		outputFile << "@R15" << endl;	//Frame
		outputFile << "M=D" << endl;
		//Set temporary return address
		outputFile << "@5" << endl;
		outputFile << "D=D-A" << endl;
		outputFile << "A=D" << endl;
		outputFile << "D=M" << endl;
		outputFile << "@R14" << endl;	//Return address
		outputFile << "M=D" << endl;
		//Reposition return value
		outputFile << "@0" << endl;
		outputFile << "A=M-1" << endl;
		outputFile << "D=M" << endl;
		outputFile << "@ARG" << endl;
		outputFile << "A=M" << endl;
		outputFile << "M=D" << endl;
		//Restore SP from caller
		outputFile << "@ARG" << endl;
		outputFile << "D=M+1" << endl;
		outputFile << "@SP" << endl;
		outputFile << "M=D" << endl;
		//Restore THAT from caller
		outputFile << "@R15" << endl;
		outputFile << "D=M-1" << endl;
		outputFile << "A=D" << endl;
		outputFile << "D=M" << endl;
		outputFile << "@THAT" << endl;
		outputFile << "M=D" << endl;
		//Restore THIS from caller
		outputFile << "@R15" << endl;
		outputFile << "D=M" << endl;
		outputFile << "@2" << endl;
		outputFile << "D=D-A" << endl;
		outputFile << "A=D" << endl;
		outputFile << "D=M" << endl;
		outputFile << "@THIS" << endl;
		outputFile << "M=D" << endl;
		//Restore ARG from caller
		outputFile << "@R15" << endl;
		outputFile << "D=M" << endl;
		outputFile << "@3" << endl;
		outputFile << "D=D-A" << endl;
		outputFile << "A=D" << endl;
		outputFile << "D=M" << endl;
		outputFile << "@ARG" << endl;
		outputFile << "M=D" << endl;
		//Restore LCL from caller
		outputFile << "@R15" << endl;
		outputFile << "D=M" << endl;
		outputFile << "@4" << endl;
		outputFile << "D=D-A" << endl;
		outputFile << "A=D" << endl;
		outputFile << "D=M" << endl;
		outputFile << "@LCL" << endl;
		outputFile << "M=D" << endl;
		//Jump to return address
		outputFile << "@R14" << endl;
		outputFile << "A=M" << endl;
		outputFile << "0;JMP" << endl;

		callStack.pop_back();
	}

	void writeFunction(string functionName, int numLocals) {
		outputFile << "(" << functionName << ")" << endl;
		for (int i = 0; i < numLocals; i++) {
			outputFile << "@0" << endl;
			outputFile << "M=M+1" << endl;
			outputFile << "A=M-1" << endl;
			outputFile << "M=0" << endl;
		}

		callStack.push_back(functionName);
	}

	void close() {
		outputFile.close();
	}
};

int handleArguments(int argc, char* argv[]) {
	if (argc == 2) {
		inputFilePath = argv[1];
		int pos = inputFilePath.string().find(".vm", 1);
		if (pos != string::npos) {
			outputFilePath = inputFilePath.string().substr(0, pos) + ".asm";
		}

		else {
			int lastChar = inputFilePath.string().length() - 1;
			while (inputFilePath.string()[lastChar] == '/' || inputFilePath.string()[lastChar] == '\\') {
				inputFilePath = inputFilePath.string().substr(0, lastChar);
				lastChar = inputFilePath.string().length() - 1;
			}
			outputFilePath = inputFilePath.string() + ".asm";
			directory = true;
		}
	}
	else {
		return 1;
	}
	return 0;
}

int main(int argc, char* argv[]) {
	if (handleArguments(argc, argv) == 1) {
		cerr << "Usage: hackVMTranslatorBasic.exe [inputFileName.vm] or hackVMTranslatorBasic.exe [inputFileDirectory]\noutput filename is inputFileName.asm or inputFileDirectory.asm";
		return 1;
	}

	parserModule bob = parserModule(inputFilePath);
	codeWriter cW = codeWriter(outputFilePath);

	cW.writeInit();

	while (bob.getHasMoreCommands()) {
		bob.advance();
		if (!bob.getHasMoreCommands()) break;
		if (bob.getCommandType() == command::C_ARITHMETIC) {
			cW.writeArithmetic(bob.getArg1());
		}
		else if (bob.getCommandType() == command::C_PUSH || bob.getCommandType() == command::C_POP) {
			cW.writePushPop(bob.getCommandType(), bob.getArg1(), bob.getArg2());
		}
		else if (bob.getCommandType() == command::C_LABEL) {
			cW.writeLabel(bob.getArg1());
		}
		else if (bob.getCommandType() == command::C_GOTO) {
			cW.writeGoto(bob.getArg1());
		}
		else if (bob.getCommandType() == command::C_IF) {
			cW.writeIf(bob.getArg1());
		}
		else if (bob.getCommandType() == command::C_FUNCTION) {
			cW.writeFunction(bob.getArg1(), bob.getArg2());
		}
		else if (bob.getCommandType() == command::C_RETURN) {
			cW.writeReturn();
		}
		else if (bob.getCommandType() == command::C_CALL) {
			cW.writeCall(bob.getArg1(), bob.getArg2());
		}
	}
}