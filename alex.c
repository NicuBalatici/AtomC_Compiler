#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "utils.h"
#include "parser.h"
#include "ad.h"
#include "at.h"

int main(void)
{
    char *pch = loadFile("tests/testat.c");
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