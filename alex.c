#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "utils.h"
#include "parser.h"
#include "ad.h"
#include "at.h"
#include "vm.h"

int main(void)
{
    // char *pch = loadFile("tests/testat.c");
    // Token *tokens = tokenize(pch);

    //showTokens(tokens);
    //showTokensTerminal(tokens);
    pushDomain();
    vmInit();
    // parse(tokens);
    // showDomain(symTable, "global");
    Instr *testCode=genTestProgram2();
    run(testCode);
    while (symTable) {
        dropDomain();
    }
    
    // free(pch);
    return 0;
}