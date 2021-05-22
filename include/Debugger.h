//
// Created by alexcons on 22/05/2021.
//

#ifndef DEBUGGER_H
#define DEBUGGER_H


#include <string>

class Debugger {
public:
    Debugger (std::string prog_name, pid_t pid) : _prog_name{std::move(prog_name)}, _pid{pid} {}

    static void launchProcess(const char *prog_name, pid_t pid);
    void run();

private:
    std::string _prog_name;
    pid_t _pid;

    void handle(const std::string& cmd);

    void continue_cmd();
};


#endif //DEBUGGER_H
