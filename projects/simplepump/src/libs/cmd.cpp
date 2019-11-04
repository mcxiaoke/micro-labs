#include "cmd.h"

string Command::toString() const {
  string s("/");
  s.append(name).append(" - ").append(desc);
  return s;
}

string CommandParam::toString() const {
  string s("{");
  for (auto const& arg : args) {
    s.append(" ").append(arg);
  }
  s.append("}");
  s.erase(1,1);
  return s;
}

bool CommandManager::handle(CommandParam& param) {
  auto handler = _getHandler(param.name);
  if (handler != nullptr) {
    handler(param.args);
    return true;
  } else {
    if (_defaultHandler != nullptr) {
      _defaultHandler(param.args);
      return true;
    }
  }
  return false;
}

void CommandManager::addCommand(Command* cmd) {
  _addHandler(cmd);
}

void CommandManager::addCommand(const char* name,
                                const char* desc,
                                CMD_HANDLER_FUNC handler) {
  Command cmd{name, desc, handler};
  _addHandler(&cmd);
}

void CommandManager::addCommand(string& name,
                                string& desc,
                                CMD_HANDLER_FUNC handler) {
  Command cmd{name, desc, handler};
  _addHandler(&cmd);
}

void CommandManager::removeCommand(Command* cmd) {
  _handlers.erase(cmd->name);
}

void CommandManager::removeCommand(string& name) {
  _handlers.erase(name);
}

vector<Command*> CommandManager::getCommands() {
  vector<Command*> vs;
  vs.reserve(_handlers.size());
  for (auto& kvp : _handlers) {
    vs.push_back(&(kvp.second));
  }
  return vs;
}

string CommandManager::getHelpDoc() {
  string s("Commands: \n");
  for (auto const& kvp : _handlers) {
    auto const cmd = kvp.second;
    s.append(cmd.toString()).append("\n");
  }
  return s;
}

void CommandManager::setDefaultHandler(CMD_HANDLER_FUNC handler) {
  _defaultHandler = handler;
}

void CommandManager::_addHandler(Command* cmd) {
  _handlers.insert(std::pair<std::string, Command>(cmd->name, *cmd));
}

CMD_HANDLER_FUNC CommandManager::_getHandler(const string& name) {
  auto it = _handlers.find(name);
  return it != _handlers.end() ? it->second.handler : nullptr;
}