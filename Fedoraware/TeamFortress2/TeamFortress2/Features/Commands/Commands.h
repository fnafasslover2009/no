#pragma once
#include <functional>

#include "../../SDK/SDK.h"

using CommandCallback = std::function<void(std::deque<std::string>)>;

class CCommands {
	std::unordered_map<std::string, CommandCallback> CommandMap;

public:
	void Init();
	bool Run(const std::string& cmd, const std::deque<std::string>& args);
	void Register(const std::string& name, CommandCallback callback);
};

#define RETURN_ADDRESS_CMD(name) \
    static bool bInitCmd = false;\
    static std::map<void*, bool> returnAddresses;\
    if (!bInitCmd)\
    {\
        F::Commands.Register("print_" ##name, [](const std::deque<std::string>& args)\
                             {\
                                 for (auto& retaddr : returnAddresses)\
                                 {\
                                     I::Cvar->ConsolePrintf("%x\n", retaddr.first);\
                                 }\
                             });\
\
        bInitCmd = true;\
    }\
\
    auto retaddr = _ReturnAddress();\
    if (returnAddresses.find(retaddr) == returnAddresses.end())\
    {\
        returnAddresses[retaddr] = true;\
    }\


ADD_FEATURE(CCommands, Commands)