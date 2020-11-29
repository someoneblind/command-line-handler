#include <iostream>
#include <string>
#include <string.h>
#include <vector>
#include <fstream>
#include <memory>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "complementer.h"

#define DEVNAME_NAME    "devname"
#define UARTSPEED_NAME  "uartspeed"
#define INPUTTYPE_NAME  "inputtype"

#define COMMAND_QUIT "\\quit"
#define SAVE_FILE_PATH "history.log"

#define KEY_UP 65
#define KEY_DOWN 66
#define KEY_RIGHT 67
#define KEY_LEFT 68

using namespace std;

struct winsize w;

void commandClean(const string &command, unsigned int position)
{
    cout << "\x1B[G";

    for (unsigned int i = 0; i < ((command.size() / w.ws_col) - (position / w.ws_col)); ++i) {
        cout << "\x1B[B";
    }

    for (unsigned int i = 0; i < (command.size() / w.ws_col); ++i) {
        cout << "\x1B[K";
        cout << "\x1B[A";
    }
    cout << "\x1B[K";
}

void commandCout(const string &command, unsigned int position)
{
    cout << command;

    if (position) {
        cout << "\x1B[G";
        for (unsigned int i = 0; i < ((position - 1) / w.ws_col); ++i) {
            cout << "\x1B[A";
        }

        for (unsigned int i = 0; i < (position / w.ws_col); ++i) {
            cout << "\x1B[B";
        }
    }

    cout << "\x1B[" << (position % w.ws_col) + 1 << "G";
}

set<string> getDevnames()
{
    set<string> uartdevs;

    using FilePtr = unique_ptr<FILE, decltype(&::pclose)>;
    string cmd("/bin/ls ");
    cmd.append("/dev/tty*");

    FilePtr ls(::popen(cmd.c_str(), "r"), ::pclose);
    if (ls == nullptr)
        return set<string>();

    vector<char> buf(1024);
    string ret;
    while (::fgets(buf.data(), static_cast<int>(buf.size()), ls.get())) {
        ret.append(buf.data());
    }

    string delimiter = "\n";
    size_t pos = 0;
    string token;
    while ((pos = ret.find(delimiter)) != std::string::npos) {
        token = ret.substr(0, pos);
        ret.erase(0, pos + delimiter.length());
        uartdevs.insert(token);
    }

    return uartdevs;
}

int main()
{
    cout << "\x1B[32;1m";

    set<string> voidSet = {} ;
    set<string> commands = { "set", "get", "send", "quit" };
    set<string> params = { "devname", "uartspeed", "inputtype" };
    set<string> devnames = getDevnames();
    set<string> uartspeeds = { "300", "600", "1200", "2400", "4800", "9600", "19200",
                               "38400", "57600", "115200", "230400", "460800", "921600" };
    set<string> inputtypes = { "ascii", "hex", "dec" };

    Complementer complementer;

    string command;
    string lastCommand;
    char symbol;

    unsigned int position = 0;

    vector<string> commandLines;
    unsigned int commandsIt = 0;

    ifstream iSaveFile;
    ofstream oSaveFile;

    struct termios oldt, newt;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    iSaveFile.open(SAVE_FILE_PATH, fstream::out);

    if (iSaveFile.is_open()) {

        string s;
        while (getline(iSaveFile, s)) {
            commandLines.push_back(s);
        }
        iSaveFile.close();

        commandsIt = static_cast<unsigned int>(commandLines.size());
    }

    while (1) {

        symbol = static_cast<char>(getchar());

        ioctl(0, TIOCGWINSZ, &w);

        if ((symbol == '\t') && (command[0] == '\\')) {

            if (count(command.begin(), command.end(), ' ') == 0)
                complementer.setSet(commands);
            else if ((count(command.begin(), command.end(), ' ') == 1)
                     && ((command.substr(1, 3) == "set")
                         || (command.substr(1, 3) == "get")))
                complementer.setSet(params);
            else if ((count(command.begin(), command.end(), ' ') == 2)
                     && (command.substr(command.find_first_of(" ") + 1, strlen(DEVNAME_NAME)) == DEVNAME_NAME))
                complementer.setSet(devnames);
            else if ((count(command.begin(), command.end(), ' ') == 2)
                     && (command.substr(command.find_first_of(" ") + 1, strlen(UARTSPEED_NAME)) == UARTSPEED_NAME))
                complementer.setSet(uartspeeds);
            else if ((count(command.begin(), command.end(), ' ') == 2)
                     && (command.substr(command.find_first_of(" ") + 1, strlen(INPUTTYPE_NAME)) == INPUTTYPE_NAME))
                complementer.setSet(inputtypes);
            else
                complementer.setSet(voidSet);

            cout  << endl;

            complementer.setInput(command.substr(command.find_last_of("\\ ") + 1));

            if (complementer.getHints().size() > 0) {
                for (const string& hint : complementer.getHints())
                    cout << hint << "    ";
                cout  << endl;

                if (command[command.size() - 1] != ' ') {
                    command += complementer.getHint(command.substr(1)).substr(command.size() - command.find_last_of("\\ ") - 1);
                    position = static_cast<unsigned int>(command.size());
                }
            }

            if (complementer.getHints().size() == 1) {
                command += ' ';
                position++;
                commandCout(command, position);
            }
            else {
                commandCout(command, position);
            }

        }
        else if ((symbol == '\t') && command.empty()) {
            command = '\\';
            position++;
            commandCout(command, position);
        }
        else if (symbol == '\n') {

            if (!command.empty()) {
                commandClean(command, position);
                commandCout(command, static_cast<unsigned int>(command.size()));
            }

            if (command.size() > 0) {

                if (commandsIt < commandLines.size())
                    commandLines.erase(commandLines.begin() + commandsIt);

                command += '\n';
                commandLines.push_back(command.substr(0, command.size() - 1));

                position = 0;
            }

            if (!command.find(COMMAND_QUIT)) {
                oSaveFile.open(SAVE_FILE_PATH);
                for (string command : commandLines)
                    oSaveFile.write(string(command + '\n').c_str(), static_cast<streamsize>(command.size() + 1));
                oSaveFile.close();

                command.clear();
                cout << static_cast<char>(symbol);
                break;
            }
            else {
                command.clear();
                cout << static_cast<char>(symbol);
            }

            commandsIt = static_cast<unsigned int>(commandLines.size());

        }
        else if ((symbol == '\b') || (symbol == 127)) {
            if (position > 0) {
                commandClean(command, position);

                command.erase(position - 1, 1);

                position--;

                commandCout(command, position);
            }
        }
        else if (symbol == 0x1b) {
            symbol = static_cast<char>(getchar());

            if (symbol == 0x5b) {
                symbol = static_cast<char>(getchar());

                switch((symbol)) {
                case KEY_UP: {
                    if (commandsIt < commandLines.size())
                        commandLines.at(commandsIt) = command;
                    if (commandsIt == commandLines.size())
                        lastCommand = command;
                    if (commandsIt > 0)
                        commandsIt--;
                    break;
                }
                case KEY_DOWN: {
                    if (commandsIt < commandLines.size())
                        commandLines.at(commandsIt) = command;
                    if (commandsIt < commandLines.size())
                        commandsIt++;
                    break;
                }
                case KEY_RIGHT: {
                    if (position < command.size()) {
                        if ((((position + 1) % w.ws_col) > 0) && (((position + 1) % w.ws_col) < w.ws_col)) {
                            cout << "\x1B[C";
                        }
                        else {
                            cout << "\x1B[B";
                            cout << "\x1B[G";
                        }
                        position++;
                    }
                    break;
                }
                case KEY_LEFT: {
                    if (position > 0) {
                        if ((position % w.ws_col) > 0) {
                            cout << "\x1B[D";
                        }
                        else {
                            cout << "\x1B[A";
                            cout << "\x1B[" << w.ws_col << "G";
                        }
                        position--;
                    }
                    break;
                }
                default: {
                    break;
                }
                }

                if ((symbol == KEY_UP) || (symbol == KEY_DOWN)) {
                    commandClean(command, position);

                    if (commandsIt < commandLines.size()) {
                        command = commandLines[commandsIt];
                    }
                    else {
                        command.clear();
                        command = lastCommand;
                    }

                        position = static_cast<unsigned int>(command.size());
                        commandCout(command, position);
                }

            }
        }
        else if (symbol >= 0x20) {
            commandClean(command, position);

            command.insert(position, string(1, symbol).c_str());
            position++;

            commandCout(command, position);
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    return 0;
}
