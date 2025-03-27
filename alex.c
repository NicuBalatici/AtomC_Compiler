#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "utils.h"
#include "parser.h"

int main(void)
{
    char *pch = loadFile("tests/testparser.c");
    Token *tokens = tokenize(pch);

    showTokens(tokens);
    parse(tokens);
    printf("Parsare completa.\n");
    
    free(pch);
    return 0;
}