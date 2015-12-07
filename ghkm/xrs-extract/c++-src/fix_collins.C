#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <sys/resource.h>

#include "Tree.h"
#include "TreeNode.h"
#include "defs.h"

using namespace mtrule;

int main(int argc, char *argv[]) {
    std::string format = "collins";
    // Perform the conversion line by line:
    std::string line;
    while(getline(std::cin,line)) {
        // Parse tree:
        Tree t(line,format);
        TreeNode *root = t.get_root();
        // Fix collins parses:
        t.fix_collins_punc();
        root->dump_collins_tree(std::cout);
    }
    return 0;
}
