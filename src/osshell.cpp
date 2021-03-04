#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <sys/stat.h>
#include <filesystem>
#include <sys/wait.h>

void splitString(std::string text, char d, std::vector<std::string>& result);
void vectorOfStringsToArrayOfCharArrays(std::vector<std::string>& list, char ***result);
void freeArrayOfCharArrays(char **array, size_t array_length);

int main (int argc, char **argv)
{   
    //compile with: 'g++ -std=c++17 -o osshell osshell.cpp -lpthread'
    // Get list of paths to binary executables
    std::vector<std::string> os_path_list;
    char* os_path = getenv("PATH");
    splitString(os_path, ':', os_path_list);

    int i;
    
    // Welcome message
    printf("Welcome to OSShell! Please enter your commands ('exit' to quit).\n");

    std::vector<std::string> command_list; // to store command user types in, split into its variour parameters
    char **command_list_exec; // command_list converted to an array of character arrays
    // Repeat:
    //  Print prompt for user input: "osshell> " (no newline)
    //  Get user input for next command
    //  If command is `exit` exit loop / quit program
    //  If command is `history` print previous N commands
    //  For all other commands, check if an executable by that name is in one of the PATH directories
    //   If yes, execute it
    //   If no, print error statement: "<command_name>: Error command not found" (do include newline)

    namespace fs = std::filesystem;
    std::vector<std::string> userHistoryVec;
    std::string temp;
    FILE *history;

    history = fopen("./history.txt", "a+");
    char line[2000];

    while ( fgets(line, sizeof(line), history))
    {
        temp.assign(line);
        temp.pop_back();
        userHistoryVec.push_back(temp.c_str());
    }
    fclose(history);


    while(true)
    {
        std::string command;
        std::cout << "osshell> ";
        std::getline(std::cin, command);
        splitString(command, ' ', command_list);
        vectorOfStringsToArrayOfCharArrays(command_list, &command_list_exec);
        if(command.compare("")== 0){

        } else {
            char **userHistory;
            if(userHistoryVec.size()==129)
            {
                userHistoryVec.erase(userHistoryVec.begin());
            }
            userHistoryVec.push_back(command);
            if(command.compare("exit") == 0)
            {
                vectorOfStringsToArrayOfCharArrays(userHistoryVec,&userHistory);
                int historySize = userHistoryVec.size();
                std::string cmd;
                history = fopen("./history.txt", "w+");
                for(int j=0; j<historySize; j++)
                {
                    cmd.assign(userHistory[j]);
                    cmd.append("\n");
                    fputs(cmd.c_str(), history);
                }
                fclose(history);
                break;
            }
            if (command_list[0].compare("history")==0)
            {
                vectorOfStringsToArrayOfCharArrays(userHistoryVec,&userHistory);
                int historySize = userHistoryVec.size();
                if(command_list.size()>1 && command_list[1].compare("clear")==0)
                {

                    userHistoryVec.clear();
                    freeArrayOfCharArrays(userHistory,historySize);
                }
                else if(command_list.size()>1)
                {
                    bool isInt = true;
                    char argInt[command_list[1].size()];
                    int j;
                    for (j=0; j<command_list[1].size();j++)
                    {
                        if (!isdigit(command_list[1][j]))
                        {
                            isInt = false;
                        }
                        argInt[j]=command_list[1][j];
                    }
                    if (isInt && atoi(argInt) > 0)
                    {
                        int start = atoi(argInt) + 1;
                        for (j = historySize-start; j<historySize-1; j++){
                            printf("%d: %s\n", j+1, userHistory[j]);
                        }
                    }
                    else 
                    {
                        printf("Error: history expects an integer > 0 (or 'clear')\n");
                    }
                }
                else if (command_list.size()==1) {
                    int j;
                    for (j = 0; j<historySize-1; j++){
                        printf("%d: %s\n", j+1, userHistory[j]);
                    }
                }
                
            }
            else {
            
            bool isValid = false;
            bool lenCheck = false;
            int j;
            std::string commandPath;

            //this section implments the '.' and '/' commands

            if(strlen(command_list_exec[0])>=1)
            {
                lenCheck=true;
            }

            if(lenCheck && (command_list_exec[0][0] == '.' || command_list_exec[0][0] == '/'))
                {
                    if(fs::exists(command_list_exec[0])==1)
                    {
                        isValid=true;
                        commandPath = command_list_exec[0];
                    }
                }
            //
            else
            {

                for (j = 0; j < os_path_list.size(); j++)
                {   
                    std::string path = os_path_list[j];
                    std::string exPath = path + "/" + command_list_exec[0];
                    if (fs::exists(exPath)==1)
                    {
                        isValid = true;
                        commandPath = exPath;
                    }
                }
            }
            const char * commandConChar = command.c_str();
            const char * commandPathChar = commandPath.c_str();
            if (isValid)
            {

                int child = fork();
                if (child == 0){
                    execv(commandPathChar,command_list_exec);
                } else {
                    int status;
                    waitpid(child, &status, 0);
                }
            
                freeArrayOfCharArrays(command_list_exec, command_list.size() + 1);
            
            } 
            else
            {
                std::cout << command;
                std::cout << ": Error command not found\n";
            }
            }

        }



    }
    

    return 0;
}

/*
   text: string to split
   d: character delimiter to split `text` on
   result: vector of strings - result will be stored here
*/
void splitString(std::string text, char d, std::vector<std::string>& result)
{
    enum states { NONE, IN_WORD, IN_STRING } state = NONE;

    int i;
    std::string token;
    result.clear();
    for (i = 0; i < text.length(); i++)
    {
        char c = text[i];
        switch (state) {
            case NONE:
                if (c != d)
                {
                    if (c == '\"')
                    {
                        state = IN_STRING;
                        token = "";
                    }
                    else
                    {
                        state = IN_WORD;
                        token = c;
                    }
                }
                break;
            case IN_WORD:
                if (c == d)
                {
                    result.push_back(token);
                    state = NONE;
                }
                else
                {
                    token += c;
                }
                break;
            case IN_STRING:
                if (c == '\"')
                {
                    result.push_back(token);
                    state = NONE;
                }
                else
                {
                    token += c;
                }
                break;
        }
    }
    if (state != NONE)
    {
        result.push_back(token);
    }
}

/*
   list: vector of strings to convert to an array of character arrays
   result: pointer to an array of character arrays when the vector of strings is copied to
*/
void vectorOfStringsToArrayOfCharArrays(std::vector<std::string>& list, char ***result)
{
    int i;
    int result_length = list.size() + 1;
    *result = new char*[result_length];
    for (i = 0; i < list.size(); i++)
    {
        (*result)[i] = new char[list[i].length() + 1];
        strcpy((*result)[i], list[i].c_str());
    }
    (*result)[list.size()] = NULL;
}

/*
   array: list of strings (array of character arrays) to be freed
   array_length: number of strings in the list to free
*/
void freeArrayOfCharArrays(char **array, size_t array_length)
{
    int i;
    for (i = 0; i < array_length; i++)
    {
        if (array[i] != NULL)
        {
            delete[] array[i];
        }
    }
    delete[] array;
}
