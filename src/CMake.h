#ifndef CMake_H
#define CMake_H

#include <string>
#include <vector>
#include <map>
#include <iostream>

class CMake {
 public:
  struct Variable {
    std::string name;
    std::string value;
    bool        deferred { false };

    Variable(const std::string &name="", const std::string &value="", bool deferred=false) :
     name(name), value(value), deferred(deferred) {
    }
  };

 public:
  CMake();

  void setQuiet(bool b) { quiet_ = b; }
  void setDebug(bool b) { debug_ = b; }

  bool processFile(const std::string &filename, bool silent=false);

  void make();
  void make(const std::string &name);

  Variable *getVariable(const std::string &name) const;

 private:
  struct Rule;

  using Words = std::vector<std::string>;

 private:
  void stringToFiles(const std::string &str, Words &words);
  void stringToWords(const std::string &str, Words &words);

  std::string replaceVariables(const std::string &str) const;

  Variable *defineVariable(const std::string &name, const std::string &value, bool deferred);

  bool isVariable(const std::string &name) const;

  Rule *defineRule(const std::string &lhs, const Words &rwords);

  Rule *getRule(const std::string &name) const;

  Rule *defRule() const { return defRule_; }

  void startBlock(bool b);
  void endBlock();

  bool isBlockActive() const;

  void make(Rule *rule);

  bool outOfDate(const std::string &lhs, const Words &rwords) const;

  bool exec(const std::string &cmd, bool silent=false);

 private:
  using Variables = std::map<std::string,Variable *>;

  class Cmd {
   public:
    Cmd(const std::string &cmd="", bool silent=false) :
     cmd_(cmd), silent_(silent) {
    }

    const std::string &cmd() const { return cmd_; }

    bool isSilent() const { return silent_; }
    void setSilent(bool b) { silent_ = b; }

    bool isIgnore() const { return ignore_; }
    void setIgnore(bool b) { ignore_ = b; }

    void print() const {
      std::cerr << "\t";

      if (silent_) std::cerr << "@";
      if (ignore_) std::cerr << "-";

      std::cerr << cmd_ << "\n";
    }

   private:
    std::string cmd_;
    bool        silent_ { false };
    bool        ignore_ { false };
  };

  using Cmds = std::vector<Cmd *>;

  struct Rule {
    std::string lhs;
    Words       rwords;
    Cmds        cmds;
    bool        phony { false };

    Rule(const std::string &lhs="", const Words &rwords=Words()) :
     lhs(lhs), rwords(rwords) {
    }

   ~Rule() {
      for (auto &cmd : cmds)
        delete cmd;
    }

    Cmd *addCmd(const std::string &cmdStr) {
      Cmd *cmd = new Cmd(cmdStr);

      cmds.push_back(cmd);

      return cmd;
    }

    bool isPhony() const { return phony; }
    void setPhony(bool b) { phony = b; }

    void print() const {
      std::cerr << lhs << ":";

      for (const auto &word :rwords)
        std::cerr << " " << word;

      std::cerr << "\n";

      for (const auto &cmd : cmds) {
        std::cerr << "\t";

        cmd->print();
      }
    }
  };

  using Rules = std::map<std::string,Rule *>;

  struct Block {
    Block(bool active=false) :
     active(active) {
    }

    bool active { false };
  };

  using Blocks = std::vector<Block>;

  bool      quiet_ { false };
  bool      debug_ { false };
  Variables variables_;
  Rules     rules_;
  Rule*     rule_    { nullptr };
  Rule*     defRule_ { nullptr };
  Blocks    blocks_;
};

#endif
