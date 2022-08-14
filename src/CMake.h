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

  bool isQuiet() const { return quiet_; }
  void setQuiet(bool b) { quiet_ = b; }

  bool isDebug() const { return debug_; }
  void setDebug(bool b) { debug_ = b; }

  bool processFile(const std::string &filename, bool silent=false);

  void make();
  void make(const std::string &name);

  Variable *getVariable(const std::string &name) const;

 private:
  class Rule;

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

  class Rule {
   public:
    Rule(const std::string &lhs="", const Words &rwords=Words()) :
     lhs_(lhs), rwords_(rwords) {
    }

   ~Rule() {
      for (auto &cmd : cmds_)
        delete cmd;
    }

    Cmd *addCmd(const std::string &cmdStr) {
      auto *cmd = new Cmd(cmdStr);

      cmds_.push_back(cmd);

      return cmd;
    }

    const std::string &lhs() const { return lhs_; }

    const Words &rwords() { return rwords_; }

    const Cmds &cmds() { return cmds_; }

    bool isPhony() const { return phony_; }
    void setPhony(bool b) { phony_ = b; }

    //---

    void print() const {
      std::cerr << lhs() << ":";

      for (const auto &word : rwords_)
        std::cerr << " " << word;

      std::cerr << "\n";

      for (const auto &cmd : cmds_) {
        std::cerr << "\t";

        cmd->print();
      }
    }

   private:
    std::string lhs_;
    Words       rwords_;
    Cmds        cmds_;
    bool        phony_ { false };

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
