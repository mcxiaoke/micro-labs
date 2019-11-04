#ifndef __CHIP_COMMANDS_H__
#define __CHIP_COMMANDS_H__

#include <Arduino.h>
#include <algorithm>
#include <functional>
#include <map>
#include <vector>

using std::string;
using std::vector;

using CMD_HANDLER_FUNC = std::function<void(std::vector<string>)>;

struct Command {
  const string name;
  const string desc;
  CMD_HANDLER_FUNC handler;

  string toString() const;
};

struct CommandParam {
  const string name;
  // name == args[0]
  vector<string> args;

  string toString() const;
};

class CommandManager {
 public:
  bool handle(CommandParam& param);
  void addCommand(Command* cmd);
  void addCommand(const char* name, const char* desc, CMD_HANDLER_FUNC handler);
  void addCommand(string& name, string& desc, CMD_HANDLER_FUNC handler);
  void removeCommand(Command* cmd);
  void removeCommand(string& name);
  vector<Command*> getCommands();
  string getHelpDoc();
  void setDefaultHandler(CMD_HANDLER_FUNC handler);

 private:
  CMD_HANDLER_FUNC _defaultHandler = nullptr;
  std::map<std::string, Command> _handlers;
  void _addHandler(Command* cmd);
  CMD_HANDLER_FUNC _getHandler(const string& name);
};

#endif