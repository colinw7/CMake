#include <CMake.h>
#include <CFileParse.h>
#include <CFile.h>
#include <CStrParse.h>
#include <COSProcess.h>
#include <iostream>

int
main(int argc, char **argv)
{
  CMake make;

  using Rules = std::vector<std::string>;
  using Vars  = std::vector<std::string>;

  std::string filename;
  std::string includeDir;
  Rules       rules;
  Vars        printVars;
  bool        processArgs = true;

  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-' && processArgs) {
      std::string opt = &argv[i][1];

      if      (opt == "f" || opt == "-file") {
        ++i;

        if (i < argc)
          filename = argv[i];
      }
      else if (opt == "I" || opt == "-include-dir") {
        ++i;

        if (i < argc)
          includeDir = argv[i];
      }
      else if (opt == "quiet")
        make.setQuiet(true);
      else if (opt == "debug")
        make.setDebug(true);
      else if (opt == "print") {
        ++i;

        if (i < argc)
          printVars.push_back(argv[i]);
      }
      else if (opt == "-") {
        processArgs = false;
      }
      else {
        std::cerr << "Invalid option '-" << opt << "'\n";
      }
    }
    else {
      rules.push_back(argv[i]);
    }
  }

  if (filename == "") {
    std::vector<std::string> filenames {"makefile", "Makefile"};

    for (const auto &name : filenames) {
      if (CFile::exists(name)) {
        filename = name;
        break;
      }
    }
  }

  make.processFile(filename);

  if (! printVars.empty()) {
    for (const auto &var : printVars) {
      CMake::Variable *var1 = make.getVariable(var);

      std::cout << var1->name << "=" << var1->value << "\n";
    }
  }

  if (! rules.empty()) {
    for (const auto &rule : rules)
      make.make(rule);
  }

  if (printVars.empty() && rules.empty())
    make.make();
}

//---

CMake::
CMake()
{
}

bool
CMake::
processFile(const std::string &filename, bool silent)
{
  if (isDebug())
    std::cerr << "Process " << filename << "\n";

  CFile file(filename);

  if (! file.open(CFileBase::Mode::READ)) {
    if (! silent) std::cerr << "Failed to open '" << filename << "'\n";
    return false;
  }

  using Lines = std::vector<std::string>;

  Lines lines;

  while (! file.eof()) {
    std::string line;

    while (! file.eof()) {
      int c = file.getC();

      if (c == EOF)
        break;

      if    (c == '\\') {
        if (! file.eof()) {
          int c1 = file.getC();

          if (c1 == '\n')
            continue;
          else {
            line += char(c);
            line += char(c1);
          }
        }
        else
          line += char(c);
      }
      else if (c == '\n')
        break;
      else
        line += char(c);
    }

    lines.push_back(line);

    if (isDebug())
      std::cerr << "Line: " << line << "\n";
  }

  for (auto &line : lines) {
    CStrParse parse(line);

    if (parse.isChar('\t')) {
      if (! isBlockActive())
        continue;

      parse.skipChar();

      auto value = parse.getAt();

      CStrParse parse1(value);

      parse1.skipSpace();

      bool silent1 = false;
      bool ignore1 = false;

      if      (parse1.isChar('@')) {
        parse1.skipChar();

        silent1 = true;
      }
      else if (parse1.isChar('-')) {
        parse1.skipChar();

        ignore1 = true;
      }

      auto value1 = parse1.getAt();

      if (rule_) {
        if (isDebug())
          std::cerr << "ADD RULE " << rule_->lhs() << " : " << value1 << "\n";

        auto *cmd = rule_->addCmd(value1);

        cmd->setSilent(silent1);
        cmd->setIgnore(ignore1);
      }
      else {
        std::cerr << "NO CURRENT RULE: " << line << "\n";
      }

      continue;
    }
    else {
      rule_ = nullptr;
    }

    parse.skipSpace();

    if (parse.eof())
      continue;

    // skip comment line
    if (parse.isChar('#'))
      continue;

    if (parse.isAlpha() || parse.isOneOf(".")) {
      std::string name;

      while (! parse.eof() && (parse.isAlnum() || parse.isOneOf("_-.")))
        name += parse.readChar();

      // define variable
      // define variable =
      // define variable :=
      // define variable ::=
      // define variable +=
      // define variable ?=
      if      (name == "define") {
        std::cerr << "TODO: " << line << "\n";
      }
      // endef
      else if (name == "endef") {
        std::cerr << "TODO: " << line << "\n";
      }
      // undefine variable
      else if (name == "undefine") {
        std::cerr << "TODO: " << line << "\n";
      }
      // ifdef variable
      else if (name == "ifdef") {
        parse.skipSpace();

        auto value = parse.getAt();

        value = replaceVariables(value);

        bool b = isVariable(value);

        startBlock(b);
      }
      // ifndef variable
      else if (name == "ifndef") {
        parse.skipSpace();

        auto value = parse.getAt();

        value = replaceVariables(value);

        bool b = ! isVariable(value);

        startBlock(b);
      }
      // ifeq (a,b)
      // ifeq "a" "b"
      // ifeq 'a' 'b'
      else if (name == "ifeq") {
        std::cerr << "TODO: " << line << "\n";
      }
      // ifneq (a,b)
      // ifneq "a" "b"
      // ifneq 'a' 'b'
      else if (name == "ifneq") {
        std::cerr << "TODO: " << line << "\n";
      }
      // else
      else if (name == "else") {
        std::cerr << "TODO: " << line << "\n";
      }
      // endif
      else if (name == "endif") {
        endBlock();
      }
      // include file
      // -include file
      // sinclude file
      else if (name == "include" || name == "-include" || name == "sinclude") {
        bool silent1 = (name != "include");

        parse.skipSpace();

        auto value = parse.getAt();

        value = replaceVariables(value);

        if (isDebug())
          std::cerr << "INCLUDE: " << value << "\n";

        Words files;

        stringToFiles(value, files);

        for (const auto &file1 : files)
          processFile(file1, silent1);
      }
      // override variable-assignment
      else if (name == "override") {
        std::cerr << "TODO: " << line << "\n";
      }
      // export
      // export variable
      // export variable-assignmentxport
      else if (name == "export") {
        std::cerr << "TODO: " << line << "\n";
      }
      // unexport variable
      else if (name == "unexport") {
        std::cerr << "TODO: " << line << "\n";
      }
      // unexport variable
      else if (name == "unexport") {
        std::cerr << "TODO: " << line << "\n";
      }
      // private variable-assignment
      else if (name == "private") {
        std::cerr << "TODO: " << line << "\n";
      }
      // vpath pattern path
      // vpath pattern
      // vpath
      else if (name == "vpath") {
        std::cerr << "TODO: " << line << "\n";
      }
      else {
        if (! isBlockActive())
          continue;

        parse.skipSpace();

        // set name value
        if      (parse.isChar('=')) {
          parse.skipChar();

          parse.skipSpace();

          auto value = parse.getAt();

          defineVariable(name, value, /*deferred*/true);
        }
        // set deferred name value
        else if (parse.isString("?=")) {
          parse.skipChar(2);

          parse.skipSpace();

          auto value = parse.getAt();

          defineVariable(name, value, /*deferred*/true);
        }
        // set name value
        else if (parse.isString(":=")) {
          parse.skipChar(2);

          parse.skipSpace();

          auto value = parse.getAt();

          value = replaceVariables(value);

          defineVariable(name, value, /*deferred*/false);
        }
        // set deferred name value
        else if (parse.isString("::=")) {
          parse.skipChar(3);

          parse.skipSpace();

          auto value = parse.getAt();

          value = replaceVariables(value);

          defineVariable(name, value, /*deferred*/false);
        }
        // append to name value
        else if (parse.isString("+=")) {
          parse.skipChar(2);

          parse.skipSpace();

          auto value = parse.getAt();

          bool deferred = false;

          if (isVariable(name)) {
          }

          if (! deferred)
            value = replaceVariables(value);

          defineVariable(name, value, deferred);
        }
        // set name value
        else if (parse.isString("!=")) {
          parse.skipChar(2);

          parse.skipSpace();

          auto value = parse.getAt();

          value = replaceVariables(value);

          defineVariable(name, value, /*deferred*/false);
        }
        // rule
        else if (parse.isChar(':')) {
          parse.skipChar();

          parse.skipSpace();

          name = replaceVariables(name);

          auto value = parse.getAt();

          Words rwords;

          stringToWords(value, rwords);

          rule_ = defineRule(name, rwords);
        }
        else {
          std::cerr << "BAD LINE: " << line << "\n";
        }
      }
    }
    else {
      std::string lhs;

      while (! parse.eof() && ! parse.isChar(':'))
        lhs += parse.readChar();

      if (parse.isChar(':')) {
        parse.skipChar();

        parse.skipSpace();

        auto rhs = parse.getAt();

        lhs = replaceVariables(lhs);

        Words rwords;

        stringToWords(rhs, rwords);

        rule_ = defineRule(lhs, rwords);
      }
      else {
        std::cerr << "BAD LINE: " << line << "\n";
      }
    }
  }

  return true;
}

void
CMake::
stringToFiles(const std::string &str, Words &words)
{
  // TODO: skip comment text
  // TODO: search include path (if not absolute)

  CStrParse parse(str);

  parse.skipSpace();

  while (! parse.eof()) {
    std::string word;

    while (! parse.eof() && ! parse.isSpace())
      word += parse.readChar();

    if (word == "")
      continue;

    std::string word1 = replaceVariables(word);

    words.push_back(word1);

    parse.skipSpace();
  }
}

void
CMake::
stringToWords(const std::string &str, Words &words)
{
  CStrParse parse(str);

  parse.skipSpace();

  while (! parse.eof()) {
    std::string word;

    while (! parse.eof() && ! parse.isSpace())
      word += parse.readChar();

    if (word == "")
      continue;

    std::string word1 = replaceVariables(word);

    words.push_back(word1);

    parse.skipSpace();
  }
}

void
CMake::
make()
{
  if (! defRule())
    return;

  make(defRule());
}

void
CMake::
make(const std::string &name)
{
  Rule *rule = getRule(name);

  if (rule)
    make(rule);
}

void
CMake::
make(Rule *rule)
{
  if (isDebug())
    std::cerr << "MAKE " << rule->lhs() << "\n";

  rule->print();

  if (outOfDate(rule->lhs(), rule->rwords())) {
    for (const auto &rword : rule->rwords()) {
      auto *rule1 = getRule(rword);

      if (rule1)
        make(rule1);
    }

    for (const auto &cmd : rule->cmds()) {
      std::string cmd1 = replaceVariables(cmd->cmd());

      exec(cmd1, cmd->isSilent());
    }
  }
}

bool
CMake::
outOfDate(const std::string &lhs, const Words &rwords) const
{
  if (isDebug()) {
    std::cerr << "CHECK OUT OF DATE: " << lhs;

    for (const auto &rword : rwords)
      std::cerr << " " << rword;

    std::cerr << "\n";
  }

  // if rhs files don't exist then need build
  for (const auto &rword : rwords) {
    if (! CFile::exists(rword)) {
      if (isDebug())
        std::cerr << "NOT EXIST : " << rword << "\n";

      return true;
    }
  }

  // if lhs file doesn't exist then need build
  if (! CFile::exists(lhs)) {
    if (isDebug())
      std::cerr << "NOT EXIST : " << lhs << "\n";

    return true;
  }

  // get lhs time
  int t1 = CFile::getMTime(lhs);

  // if any file on rhs is new than rhs then need build
  for (const auto &rword : rwords) {
    int t2 = CFile::getMTime(rword);

    if (t1 < t2) {
      if (isDebug())
        std::cerr << "NEWER : " << rword << "\n";

      return true;
    }
  }

  return false;
}

bool
CMake::
exec(const std::string &cmd, bool silent)
{
  if (isDebug())
    std::cerr << "EXEC : " << cmd << "\n";

  if (! silent)
    std::cerr << cmd << "\n";

  return COSProcess::executeCommand(cmd);
}

std::string
CMake::
replaceVariables(const std::string &str) const
{
  std::string str1;

  CStrParse parse(str);

  while (! parse.eof()) {
    if (parse.isChar('$')) {
      parse.skipChar();

      if (parse.isChar('(')) {
        parse.skipChar();

        std::string var;

        while (! parse.eof() && ! parse.isChar(')'))
          var += parse.readChar();

        if (parse.isChar(')'))
          parse.skipChar();

        if (isVariable(var)) {
          CMake::Variable *var1 = getVariable(var);

          str1 += var1->value;
        }
        else
          str1 += "$(" + var + ")";
      }
      else {
        str1 += '$';
        str1 += parse.readChar();
      }
    }
    else
      str1 += parse.readChar();
  }

  return str1;
}

CMake::Variable *
CMake::
defineVariable(const std::string &name, const std::string &value, bool deferred)
{
  if (isDebug())
    std::cerr << "DEFINE VAR: " << name << "=" << value << "\n";

  auto p = variables_.find(name);

  if (p == variables_.end()) {
    Variable *var = new Variable(name, value, deferred);

    p = variables_.insert(p, Variables::value_type(name, var));
  }

  Variable *var = (*p).second;

  var->value    = value;
  var->deferred = deferred;

  return var;
}

bool
CMake::
isVariable(const std::string &name) const
{
  auto p = variables_.find(name);

  if (p != variables_.end())
    return true;

  auto ptr = getenv(name.c_str());

  if (ptr)
    return true;

  return false;
}

CMake::Variable *
CMake::
getVariable(const std::string &name) const
{
  auto p = variables_.find(name);

  if (p != variables_.end())
    return (*p).second;

  auto ptr = getenv(name.c_str());

  if (ptr) {
    CMake *th = const_cast<CMake *>(this);

    return th->defineVariable(name, ptr, false);
  }

  return nullptr;
}

CMake::Rule *
CMake::
defineRule(const std::string &lhs, const Words &rwords)
{
  // .PHONY
  // .SUFFIXES
  // .DEFAULT
  // .PRECIOUS
  // .INTERMEDIATE
  // .SECONDARY
  // .SECONDEXPANSION
  // .DELETE_ON_ERROR
  // .IGNORE
  // .LOW_RESOLUTION_TIME
  // .SILENT
  // .EXPORT_ALL_VARIABLES
  // .NOTPARALLEL
  // .ONESHELL
  // .POSIX

  if (lhs == ".PHONY") {
    for (const auto &rword : rwords) {
      Rule *rule = getRule(rword);

      if (rule)
        rule->setPhony(true);
    }

    return nullptr;
  }

  auto *rule = new Rule(lhs, rwords);

  if (isDebug()) {
    std::cerr << "DEFINE RULE: "; rule->print();
  }

  rules_[lhs] = rule;

  if (lhs[0] != '.') {
    if (! defRule_)
      defRule_ = rule;
  }

  return rule;
}

CMake::Rule *
CMake::
getRule(const std::string &name) const
{
  if (isDebug())
    std::cerr << "GET RULE: " << name << "\n";

  auto p = rules_.find(name);

  if (p != rules_.end())
    return (*p).second;

  if (isDebug())
    std::cerr << "not found\n";

  return nullptr;
}

void
CMake::
startBlock(bool b)
{
  blocks_.push_back(Block(b));
}

void
CMake::
endBlock()
{
  if (blocks_.empty()) {
    std::cerr << "No block\n";
    return;
  }

  blocks_.pop_back();
}

bool
CMake::
isBlockActive() const
{
  if (blocks_.empty())
    return true;

  return blocks_.back().active;
}
