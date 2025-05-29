#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "utils.h"
#include "parser.h"
#include "ad.h"
#include "at.h"
#include "vm.h"
#include "gc.h"

int main(void)
{
    char *pch = loadFile("tests/testgc.c");
    Token *tokens = tokenize(pch);

    showTokens(tokens);
    // showTokensTerminal(tokens);
    pushDomain();
    vmInit();
    parse(tokens);
    showDomain(symTable, "global");
    // Instr *testCode=genTestProgram2();
    // run(testCode);
    // while (symTable) {
    //     dropDomain();
    // }
    
    Symbol *symMain=findSymbolInDomain(symTable,"main");
    if(!symMain)err("missing main function");
    Instr *entryCode=NULL;
    addInstr(&entryCode,OP_CALL)->arg.instr=symMain->fn.instr;
    addInstr(&entryCode,OP_HALT);
    run(entryCode);

    free(pch);
    return 0;
}