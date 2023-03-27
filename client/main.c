#include "graphql_ast.h"
#include "parser.tab.h"

int main() {
    astNode* tree;
    int code = yyparse(&tree);
    if (code == 0) {
        printNode(tree, 0);
    }
    destroyNode(tree);
	return 0;
}
