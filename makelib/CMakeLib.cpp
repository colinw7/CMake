#include <CFile.h>
#include <COSUser.h>
#include <CStrUtil.h>
#include <map>
#include <iostream>

struct CMakeLibDep {
  typedef std::vector<std::string> LibList;

  std::string  name;
  LibList      libs;
  bool         used;

  CMakeLibDep() { }

  CMakeLibDep(const std::string &name1, const std::vector<std::string> &libs1) :
   name(name1), libs(libs1) {
  }

  void print(std::ostream &os);
};

struct CLibDepNode {
  typedef std::map<std::string,CLibDepNode *> DependencyMap;

  CLibDepNode(const std::string &name1) :
   name(name1) {
  }

  std::string   name;
  DependencyMap up_dependencies;
  DependencyMap down_dependencies;

  void removeUp();
  void removeDown();
};

struct CLibDepTree {
  typedef std::map<std::string,CLibDepNode *> NodeMap;

  std::string name;
  NodeMap     nodes;

  CLibDepTree(const std::string &name1) :
   name(name1) {
  }

  CLibDepNode *getNode(const std::string &name);

  void addDependency(CLibDepNode *node, const std::string &name);

  void printDependencies();
};

class CMakeLib {
 private:
  typedef std::map<std::string,CMakeLibDep> DependencyMap;

  DependencyMap dependencies;

 public:
  CMakeLib();

  bool initDependencies();

  void addDependency(const std::string &name, const std::vector<std::string> &libs);

  bool genDependencies(const std::string &name);
  bool genSubDependencies(CLibDepTree &tree, const std::string &name);
};

int
main(int argc, char **argv)
{
  CMakeLib make_lib;

  if (! make_lib.initDependencies())
    exit(1);

  for (int i = 1; i < argc; ++i)
    make_lib.genDependencies(argv[i]);

  return 0;
}

CMakeLib::
CMakeLib()
{
}

bool
CMakeLib::
initDependencies()
{
  std::string home;

  if (! COSUser::getUserHome(home))
    home = ".";

  CFile file(home + "/.makelib");

  if (! file.exists())
    return false;

  std::string line;

  while (file.readLine(line)) {
    line = CStrUtil::stripSpaces(line);

    if (line == "" || line[0] == '#')
      continue;

    std::vector<std::string> fields;

    CStrUtil::addFields(line, fields, ":");

    uint num_fields = fields.size();

    if (num_fields != 2)
      continue;

    std::vector<std::string> words;

    CStrUtil::addWords(fields[1], words);

    addDependency(fields[0], words);
  }

  return true;
}

void
CMakeLib::
addDependency(const std::string &name, const std::vector<std::string> &libs)
{
  CMakeLibDep dep(name, libs);

  dependencies[name] = dep;
}

bool
CMakeLib::
genDependencies(const std::string &name)
{
  CLibDepTree tree(name);

  DependencyMap::iterator p1 = dependencies.begin();
  DependencyMap::iterator p2 = dependencies.end  ();

  for ( ; p1 != p2; ++p1)
    (*p1).second.used = false;

  genSubDependencies(tree, name);

  tree.printDependencies();

  return true;
}

bool
CMakeLib::
genSubDependencies(CLibDepTree &tree, const std::string &name)
{
  DependencyMap::iterator p = dependencies.find(name);

  if (p == dependencies.end()) {
    std::cerr << "Warning: dependencies for " << name << " not found" << std::endl;
    return false;
  }

  CMakeLibDep &dep = (*p).second;

  if (dep.used)
    return true;

  dep.used = true;

  CLibDepNode *node = tree.getNode(name);

  uint num_libs = dep.libs.size();

  for (uint i = 0; i < num_libs; ++i)
    tree.addDependency(node, dep.libs[i]);

  for (uint i = 0; i < num_libs; ++i)
    genSubDependencies(tree, dep.libs[i]);

  return true;
}

void
CMakeLibDep::
print(std::ostream &os)
{
  std::cerr << name << ":";

  uint num_libs = libs.size();

  for (uint i = 0; i < num_libs; ++i)
    os << " " << libs[i];

  os << std::endl;
}

CLibDepNode *
CLibDepTree::
getNode(const std::string &name)
{
  NodeMap::iterator p = nodes.find(name);

  if (p != nodes.end())
    return (*p).second;

  CLibDepNode *node = new CLibDepNode(name);

  nodes[name] = node;

  return node;
}

void
CLibDepTree::
addDependency(CLibDepNode *node, const std::string &name)
{
  CLibDepNode *node1 = getNode(name);

  node ->down_dependencies[node1->name] = node1;
  node1->up_dependencies  [node ->name] = node ;
}

void
CLibDepTree::
printDependencies()
{
  std::list<std::string> lnames;
  std::list<std::string> rnames;

  while (true) {
    bool match = false;

    do {
      NodeMap::iterator p1 = nodes.begin();
      NodeMap::iterator p2 = nodes.end  ();

      match = false;

      while (p1 != p2) {
        CLibDepNode *node = (*p1).second;

        bool match1 = false;

        if (node->up_dependencies.empty()) {
          lnames.push_back(node->name);

          node->removeDown();

          match1 = true;
        }

        if (node->down_dependencies.empty()) {
          rnames.push_front(node->name);

          node->removeUp();

          match1 = true;
        }

        if (match1) {
          nodes.erase(p1);

          p1 = nodes.begin();
          p2 = nodes.end  ();

          match = true;
        }
        else
          ++p1;
      }
    }
    while (match);

    if (nodes.empty())
      break;

    CLibDepNode       *max_node = NULL;
    uint               max_num  = 0;
    NodeMap::iterator  max_p;

    NodeMap::iterator p1 = nodes.begin();
    NodeMap::iterator p2 = nodes.end  ();

    for ( ; p1 != p2; ++p1) {
      CLibDepNode *node = (*p1).second;

      uint num1 = node->up_dependencies  .size();
      uint num2 = node->down_dependencies.size();

      uint num = num1 + num2;

      if (max_node == NULL || num > max_num) {
        max_node = node;
        max_num  = num;
        max_p    = p1;
      }
    }

    std::cerr << "Removing Circular dependency for " << max_node->name << std::endl;

    lnames.push_back (max_node->name);
    rnames.push_front(max_node->name);

    max_node->removeDown();
    max_node->removeUp  ();

    nodes.erase(max_p);
  }

  uint w = 0;

  std::cout << "-l" << name;

  w += name.size() +  2;

  std::list<std::string>::iterator p1 = lnames.begin();
  std::list<std::string>::iterator p2 = lnames.end  ();

  for ( ; p1 != p2; ++p1) {
    uint len1 = (*p1).size();

    if (w > 0 && w + len1 > 74) {
      std::cout << " \\" << std::endl;

      w = 0;
    }

    if (w > 0) {
      std::cout << " ";

      ++w;
    }

    std::cout << "-l" << (*p1);

    w += len1 + 2;
  }

  p1 = rnames.begin();
  p2 = rnames.end  ();

  for ( ; p1 != p2; ++p1) {
    uint len1 = (*p1).size();

    if (w > 0 && w + len1 > 74) {
      std::cout << " \\" << std::endl;

      w = 0;
    }

    if (w > 0) {
      std::cout << " ";

      ++w;
    }

    std::cout << "-l" << (*p1);

    w += len1 + 2;
  }

  if (w > 0)
    std::cout << std::endl;
}

void
CLibDepNode::
removeUp()
{
  DependencyMap::iterator p1 = up_dependencies.begin();
  DependencyMap::iterator p2 = up_dependencies.end  ();

  for ( ; p1 != p2; ++p1) {
    CLibDepNode *node = (*p1).second;

    DependencyMap::iterator p = node->down_dependencies.find(name);

    if (p != node->down_dependencies.end())
      node->down_dependencies.erase(p);
  }
}

void
CLibDepNode::
removeDown()
{
  DependencyMap::iterator p1 = down_dependencies.begin();
  DependencyMap::iterator p2 = down_dependencies.end  ();

  for ( ; p1 != p2; ++p1) {
    CLibDepNode *node = (*p1).second;

    DependencyMap::iterator p = node->up_dependencies.find(name);

    if (p != node->up_dependencies.end())
      node->up_dependencies.erase(p);
  }
}
