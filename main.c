#include <stdbool.h>
#include "parser.tab.h"

extern int yydebug;

int main() {
    if (YYDEBUG == 1) {
        yydebug = 1;
    }
	yyparse();
	return 0;
}
