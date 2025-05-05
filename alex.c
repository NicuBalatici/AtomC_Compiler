#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "utils.h"
#include "parser.h"
#include "ad.h"

int main(void)
{
    char *pch = loadFile("tests/testad.c");
    Token *tokens = tokenize(pch);

    //showTokens(tokens);
    //showTokensTerminal(tokens);
    pushDomain();

    parse(tokens);
    
    showDomain(symTable, "global");
    while (symTable) {
        dropDomain();
    }
    
    free(pch);
    return 0;
}