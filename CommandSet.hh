#ifndef COMMANDSET_HH
#define COMMANDSET_HH

#include "Command.hh"
#include <vector>

class CommandSet{
public:
  std::vector<Command> commands;

  CommandSet() {};
  CommandSet(Command);
  CommandSet(std::vector<Command>);
};

inline CommandSet::CommandSet(Command cmd){
  commands.push_back(cmd);
}

inline CommandSet::CommandSet(std::vector<Command> cmds){
  commands = cmds;
}

inline std::ostream& operator<<(std::ostream &strm, const CommandSet &a) {
  for (auto & command : a.commands){
    strm << command << std::endl;
  }
  return strm;
}


#endif
