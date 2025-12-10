#include <iostream>
#include <vector>
#include <map>
#include <bitset>
#include <string>
#include <fstream>
using namespace std;

    map<string, string> opcodeDict = 
    {
        {"NOP", "0000"},
        {"HLT", "0001"},
        {"ADD", "0010"},
        {"SUB", "0011"},
        {"NOR", "0100"},
        {"AND", "0101"},
        {"XOR", "0110"},
        {"RSH", "0111"},
        {"LDI", "1000"},
        {"ADI", "1001"},
        {"JMP", "1010"},
        {"BRH", "1011"},
        {"CAL", "1100"},
        {"RET", "1101"},
        {"LOD", "1110"},
        {"STR", "1111"}
    };
    map<string, string> regDict = 
    {
        {"r0", "0000"},
        {"r1", "0001"},
        {"r2", "0010"},
        {"r3", "0011"},
        {"r4", "0100"},
        {"r5", "0101"},
        {"r6", "0110"},
        {"r7", "0111"},
        {"r8", "1000"},
        {"r9", "1001"},
        {"r10", "1010"},
        {"r11", "1011"},
        {"r12", "1100"},
        {"r13", "1101"},
        {"r14", "1110"},
        {"r15", "1111"}
    };
    map<string, string> branchDict = 
    {
        {"notzero", "000"},
        {"nz", "000"},
        {"odd", "001"},
        {"noteven", "001"},
        {"neq", "010"},
        {"!=", "010"},
        {"lt", "011"},
        {"<", "011"},
        {"zero", "100"},
        {"z", "100"},
        {"even", "101"},
        {"notodd", "101"},
        {"eq", "110"},
        {"=", "110"},
        {"gt", "111"},
        {">", "111"}
    };

vector<string> splitUp(string assembly)
{
    vector<string> output;
    string tempString = "";
    int section = 0;
    for(int i = 0; i< assembly.length(); i++)
    {
        if (assembly[i] == ' ')
        {
            if (!tempString.empty()) {
                output.push_back(tempString);
                tempString = "";
            }
        }
        else if (assembly[i] == '/')
        {
            return output;
        }
        else
        {
            tempString += assembly[i];
        }
    }

    return output;
}

string stringToAssembly(string input)
{
    vector<string> toConvert = splitUp(input + " ");
    if(toConvert[0] == "NOP")
    {
        return "0000000000000000";
    }
    else if (toConvert[0] == "HLT")
    {
        return "0001000000000000";
    }
    else if (toConvert[0] == "JMP")
    {
        return "101000" + bitset<10>(stoi(toConvert[1])).to_string();
    }
    else if (toConvert[0] == "BRH")
    {
        return "1011" + branchDict[toConvert[1]] + "0" + bitset<8>(stoi(toConvert[2])).to_string();
    }
    else if (toConvert[0] == "RSH")
    {
        return "0111" + regDict[toConvert[1]] + "0000" + regDict[toConvert[2]];
    }
    else if (toConvert[0] == "LDI")
    {
        return "1000" + regDict[toConvert[1]] + bitset<8>(stoi(toConvert[2])).to_string();
    }
    else if (toConvert[0] == "ADI")
    {
        return "1001" + regDict[toConvert[1]] + bitset<8>(stoi(toConvert[2])).to_string();
    }
    else if (toConvert[0] == "CAL")
    {
        return "11000000" + bitset<8>(stoi(toConvert[1])).to_string();
    }
    else if (toConvert[0] == "RET")
    {
        return "1101000000000000";
    }
    else if (toConvert[0] == "LOD" || toConvert[0] == "STR")
    {
        return opcodeDict[toConvert[0]] + regDict[toConvert[1]] + regDict[toConvert[2]] + bitset<4>(stoi(toConvert[3])).to_string();
    }
    else if (toConvert[0] == "INC")
    {
        return "1001" + regDict[toConvert[1]] + "00000001";
    }
    else if (toConvert[0] == "DEC")
    {
        return "1001" + regDict[toConvert[1]]+ "11111111";
    }
    else if (toConvert[0] == "CMP")
    {
        return "0000"+ regDict[toConvert[1]] + regDict[toConvert[2]] + "0000";
    }
    else
    {
        return opcodeDict[toConvert[0]] + regDict[toConvert[1]] + regDict[toConvert[2]] + regDict[toConvert[3]];
    }

    return "0000000000000000";
}

int main()  
{
    vector<string> instructionSet;
    ifstream infile("instructions.as");
    string line;
    while (getline(infile, line)) {
        if (!line.empty())
            instructionSet.push_back(line);
    }
    infile.close();

    for (const string& instr : instructionSet) {
        cout << stringToAssembly(instr) << endl;
    }

    return 0;
}