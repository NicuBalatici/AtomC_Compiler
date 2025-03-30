#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<stdarg.h>

#include "utils.h"
#include "parser.h"
#include "lexer.h"

Token *iTk;
Token *consumedTk; 

void tkerr(const char *fmt,...)
{
	fprintf(stderr,"error in line %d: ",iTk->line);
	va_list va;
	va_start(va,fmt);
	vfprintf(stderr,fmt,va);
	va_end(va);
	fprintf(stderr,"\n");
	exit(EXIT_FAILURE);
}

void parse(Token *tokens)
{
    iTk = tokens;
    if (!unit())
    {
        err("Error: Syntax analysis failed.\n");
    }
}

bool expr()
{
    return exprAssign();
}

bool exprAssign()
{
    Token *start = iTk;
    if (exprUnary())
    {
        if(consume(ASSIGN))
        {
            if(exprAssign())
            {
                return true;
            }
            // else
            // {
            //     tkerr("Invalid syntax.");
            // }
        }
        else
        {
            tkerr("Wrong =.");
        }
    }
    // else
    // {
    //     tkerr("Missing = before statement.");
    // }
    iTk = start;
    return exprOr();
}

bool exprOr()
{
    Token *start = iTk;

    if (exprOr())
    {
        if(consume(OR))
        {
            if(exprAnd())
            {
                return true;
            }
            // else
            // {
            //     tkerr("Invalid syntax.");
            // }
        }
        else
        {
            tkerr("Missing | before syntax.");
        }
    }
    // else
    // {
    //     tkerr("Missing || before statement.");
    // }

    iTk = start;
    return exprAnd();
}

bool exprAnd()
{
    Token *start = iTk;

    if (exprAnd())
    {
        if(consume(AND))
        {
            if(exprEq())
            {
                return true;
            }
            // else
            // {
            //     tkerr("Invalid syntax.");
            // }
        }
        else
        {
            tkerr("Missing & mefore statement.");
        }
    }
    // else
    // {
    //     tkerr("Missing && before statement.");
    // }

    iTk = start;
    return exprEq();
}

bool exprEq()
{
    Token *start = iTk;

    if (exprEq())
    {
        if(consume(EQUAL))
        {
            if(exprRel())
            {
                return true;
            }
            // else
            // {
            //     tkerr("Invalid syntax.");
            // }
        }
        else
        {
            tkerr("Missing = before statement.");
        }
    }
    // else
    // {
    //     tkerr("Invalid syntax.");
    // }

    if (exprEq())
    {
        if(consume(NOTEQ))
        {
            if(exprRel())
            {
                return true;
            }
            // else
            // {
            //     tkerr("Invalid syntax.");
            // }
        }
        else
        {
            tkerr("Missing != before statement.");
        }
    }
    // else
    // {
    //     tkerr("Invalid syntax.");
    // }

    iTk = start;
    return exprRel();
}

bool exprRel()
{
    Token *start = iTk;

    if (exprRel())
    {
        if(consume(LESS))
        {
            if(exprAdd())
            {
                return true;
            }
            // else
            // {
            //     tkerr("Invalid expression.");
            // }
        }
        else
        {
            tkerr("Missing < before statement.");
        }
    }

    if (exprRel())
    {
        if(consume(LESSEQ))
        {
            if(exprAdd())
            {
                return true;
            }
            // else
            // {
            //     tkerr("Invalid expression.");
            // }
        }
        else
        {
            tkerr("Missing <= before statement.");
        }
    }

    if (exprRel())
    {
        if(consume(GREATER))
        {
            if(exprAdd())
            {
                return true;
            }
            // else
            // {
            //     tkerr("Invalid expression.");
            // }
        }
        else
        {
            tkerr("Missing > before statement.");
        }
    }

    if (exprRel())
    {
        if(consume(GREATEREQ))
        {
            if(exprAdd())
            {
                return true;
            }
            // else
            // {
            //     tkerr("Invalid expression.");
            // }
        }
        else
        {
            tkerr("Missing => before expression.");
        }
    }

    iTk = start;
    return exprAdd();
}

bool exprAdd()
{
    Token *start = iTk;
    
    if (exprAdd())
    {
        if(consume(ADD))
        {
            if(exprMul())
            {
                return true;
            }
            // else
            // {
            //     tkerr("Invalid expression.");
            // }
        }
        else
        {
            tkerr("Missing + before statement.");
        }
    }

    if (exprAdd())
    {
        if(consume(SUB))
        {
            if(exprMul())
            {
                return true;
            }
            // else
            // {
            //     tkerr("Invalid expression.");
            // }
        }
        else
        {
            tkerr("Missing - before statement.");
        }
    }

    iTk = start;
    return exprMul();
}

bool exprMul()
{
    Token *start = iTk;

    if (exprMul())
    {
        if(consume(MUL))
        {
            if(exprCast())
            {
                return true;
            }
            // else
            // {
            //     tkerr("invalid expression.");
            // }
        }
        else
        {
            tkerr("Missing * before statement.");
        }
    }

    if (exprMul())
    {
        if(consume(DIV))
        {
            if(exprCast())
            {
                return true;
            }
            // else
            // {
            //     tkerr("invalid expression.");
            // }
        }
        else
        {
            tkerr("Missing * before statement.");
        }
    }

    iTk = start;
    return exprCast();
}

bool exprCast()
{
    Token *start = iTk;
    if (consume(LPAR))
    {
        if(typeBase())
        {
            if(consume(RPAR))
            {
                if(exprCast())
                {
                    return true;
                }
                // else
                // {
                //     tkerr("Invalid syntax.");
                // }
            }
            else
            {
                tkerr("Missing ) before syntax.");
            }
        }
        // else
        // {
        //    tkerr("Invalid syntax.");
        // }
    }
    else
    {
        tkerr("Missing ( before syntax.");
    }
    iTk = start;
    return exprUnary();
}

bool exprUnary()
{
    Token *start = iTk;
    if (consume(SUB))
    {
        if(exprUnary())
        {
            return true;
        }
        // else
        // {
        //     tkerr("Invalid syntax.");
        // }
    }
    else
    {
        tkerr("Missing - before statement.");
    }

    if (consume(NOT))
    {
        if(exprUnary())
        {
            return true;
        }
        // else
        // {
        //     tkerr("Invalid syntax.");
        // }
    }
    else
    {
        tkerr("Missing ! before statement.");
    }
    iTk = start;
    return exprPostfix();
}

bool exprPrimary()
{
    Token *start = iTk;

    if (consume(ID))
    {
        if (consume(LPAR))
        { 
            if (expr())
            {
                while (consume(COMMA) && expr());
            }
            if (!consume(RPAR))
            {
                tkerr("Expected ')' after function call arguments.");
            }
        }
        else
        {
            tkerr("Missing ( before statement.");
        }
        return true;
    }
    //INT DOUBLE CHAR STRING
    if (consume(INT))
    {
        return true;
    }
    else
    {
        tkerr("Error. It should be INT.");
    }

    if (consume(DOUBLE))
    {
        return true;
    }
    else
    {
        tkerr("Error. It should be DOUBLE.");
    }

    if (consume(CHAR))
    {
        return true;
    }
    else
    {
        tkerr("Error. It should be CHAR.");
    }

    if (consume(STRING))
    {
        return true;
    }
    else
    {
        tkerr("Error. It should be STRING.");
    }

    // if (consume(LPAR))
    // {
    //     if (!expr())
    //     {
    //         err("Expected expression inside parentheses");
    //     }
    //     if (!consume(RPAR))
    //     {
    //         tkerr("Expected closing ) after expression");
    //     }
    //     return true;
    // }

    iTk = start;
    return false;
}

bool exprPostfix()
{
    Token *start = iTk;

    if (exprPrimary())
    {
        return true;
    }
    // else
    // {
    //     tkerr("Invalid syntax.");
    // }

    if (exprPostfix())
    {
        if(consume(LBRACKET))
        {
            if(expr())
            {
                if(consume(RBRACKET))
                {
                    return true;
                }
                // else
                // {
                //     tkerr("Missing ] before statement.");
                // }
            }
            // else
            // {
            //     tkerr("Invalid syntax.");
            // }
        }
        else
        {
            tkerr("Missing [ before statement.");
        }
    }
    // else
    // {
    //     tkerr("Invalid syntax.");
    // }

    if (exprPostfix())
    {
        if(consume(DOT))
        {
            if(consume(ID))
            {
                return true;
            }
            else
            {
                tkerr("Invalid ID.");
            }
        }
        else
        {
            tkerr("Missing . before statement.");
        }
    }
    // else
    // {
    //     tkerr("invlaid syntax.");
    // }
    
    iTk = start;
    return false;
}

char *tkCodeName(int code)
{
    switch (code) {
        case ASSIGN: return "ASSIGN";
        case OR: return "OR";
        case AND: return "AND";
        case EQUAL: return "EQUAL";
        case NOTEQ: return "NOTEQ";
        case LESS: return "LESS";
        case LESSEQ: return "LESSEQ";
        case GREATER: return "GREATER";
        case GREATEREQ: return "GREATEREQ";
        case ADD: return "ADD";
        case SUB: return "SUB";
        case MUL: return "MUL";
        case DIV: return "DIV";
        case LPAR: return "LPAR";
        case RPAR: return "RPAR";
        case LBRACKET: return "LBRACKET";
        case RBRACKET: return "RBRACKET";
        case DOT: return "DOT";
        case COMMA: return "COMMA";
        case SEMICOLON: return "SEMICOLON";
        case ID: return "ID";
        case INT: return "INT";
        case DOUBLE: return "DOUBLE";
        case CHAR: return "CHAR";
        case STRING: return "STRING";
        case STRUCT: return "STRUCT";
        case IF: return "IF";
        case ELSE: return "ELSE";
        case WHILE: return "WHILE";
        case RETURN: return "RETURN";
        case VOID: return "VOID";
        case TYPE_INT: return "TYPE_INT";
        case TYPE_DOUBLE: return "TYPE_DOUBLE";
        case TYPE_CHAR: return "TYPE_CHAR";
        case END: return "END";
        default: return "UNKNOWN";
    }
}

bool consume(int code)
{
    printf("consume(%s) ",tkCodeName(code));

    if(iTk->code==code)
    {
        consumedTk=iTk;
        iTk=iTk->next;
        printf(" => consumed\n");
        return true;
    }

    printf(" => found %s\n",tkCodeName(iTk->code));
    return false;
}

bool structDef()
{
    if (consume(STRUCT))
    {
        if(consume(ID))
        {
            if(consume(LACC))
            {
                while (varDef());
                return consume(RACC) && consume(SEMICOLON);
            }
            else
            {
                tkerr("Missing { before statement.");
            }
        }
        else
        {
            tkerr("Invalid syntax.");
        }
    }
    else
    {
        tkerr("Invalid STRUCT type.");
    }
    return false;
}

bool varDef()
{
    Token *start = iTk;
    if (typeBase())
    {
        if(consume(ID))
        {
            arrayDecl();
            if (consume(SEMICOLON))
            {
                return true;
            }
            else
            {
                tkerr("Missing ; at the end od the line.");
            }
        }
        else
        {
            tkerr("invalid ID");
        }
    }
    // else
    // {
    //     tkerr("Invalid syntax");
    // }
    iTk = start;
    return false;
}

bool fnDef()
{
    Token *start = iTk;
    if (typeBase())
    {
        if(consume(ID))
        {
            if(consume(LPAR))
            {
                fnParam();
                while (consume(COMMA) && fnParam());
                if (consume(RPAR))
                {
                    if(stmCompound())
                    {
                        return true;
                    }
                    else
                    {
                        tkerr("Invalid syntax.");
                    }
                }
                else
                {
                    tkerr("Missing ) before statement.");
                }
            }
            else
            {
                tkerr("Missing ( before statement.");
            }
        }
        else
        {
            tkerr("Missing ID");
        }
    }
    // else
    // {
    //     tkerr("Invalid syntax.");
    // }

    if (consume(VOID))
    {
        if(consume(ID))
        {
            if(consume(LPAR))
            {
                fnParam();
                while (consume(COMMA) && fnParam());
                if (consume(RPAR))
                {
                    if(stmCompound())
                    {
                        return true;
                    }
                    // else
                    // {
                    //     tkerr("Invalid syntax.");
                    // }
                }
                else
                {
                    tkerr("Missing ) before statement.");
                }
            }
            else
            {
                tkerr("Missing ( before statement.");
            }
        }
        else
        {
            tkerr("Missing ID");
        }
    }
    else
    {
        tkerr("Invalid VOID syntax.");
    }

    iTk = start;
    return false;
}

bool unit()
{
    for(;;)
    { 
        if(structDef()){}
        else if(fnDef()){}
        else if(varDef()){}
        else break;
    }
    
    if(consume(END))
    {
        return true;
    }
    else
    {
        tkerr("Invalid EOF or \\0");
    }
    return true;
}

bool typeBase()
{
    if(consume(TYPE_INT))
    {
        return true;
    }
    else
    {
        tkerr("Incorrect declaration type (Should be INT).");        
    }
    if(consume(TYPE_DOUBLE))
    {
        return true;
    }
    else
    {
        tkerr("Incorrect declaration type (Should be DOUBLE).");        
    }
    if(consume(TYPE_CHAR))
    {
        return true;
    }
    else
    {
        tkerr("Incorrect declaration type (Should be CHAR).");        
    }
    if(consume(STRUCT))
    {
        if(consume(ID))
        {
            return true;
        }
        else
        {
            tkerr("Invalid format.");
        }
    }
    else
    {
        tkerr("Incorrect declaration type (Should be STRUCT).");        
    }
    return false;
}

bool arrayDecl()
{
    Token *start = iTk;
    if (consume(LBRACKET))
    {
        consume(INT);
        return consume(RBRACKET);
    }
    else
    {
        tkerr("Missing [ before statement.");
    }
    iTk = start;
    return false;
}

bool fnParam()
{
    Token *start=iTk; 
    if(typeBase())
    {
        if(consume(ID))
        {
            if(arrayDecl())
            { 
                return true;
            }
            // else
            // {
            //     tkerr("Invalid syntax.");
            // }
        }
        else
        {
            tkerr("Invalid ID.");
        }
    }
    // else
    // {
    //     tkerr("Invalid syntax.");
    // }
    iTk=start;
    return false;
}

bool stm()
{
    if (stmCompound()) return true;
    
    if (consume(IF))
    {
        if(consume(LPAR))
        {
            if(expr())
            {
                if(consume(RPAR))
                {
                    if(stm())
                    {
                        consume(ELSE) && stm();
                        return true;
                    }
                    else
                    {
                        tkerr("Missing or invalid expression in else statement.");
                    }
                }
                else
                {
                    tkerr("Missing or invalid expression in ) statement.");
                }
            }
            // else
            // {
            //     tkerr("Invalid expression.");
            // }
        }
        else
        {
            tkerr("Missing ( before statement.");
        }
    }

    if (consume(WHILE))
    {
        if(consume(LPAR))
        {
            if(expr())
            {
                if(consume(RPAR))
                {
                    if(stm())
                    {
                        return true;
                    }
                    else
                    {
                        tkerr("Missing or invalid expression in else statement.");
                    }
                }
                else
                {
                    tkerr("Missing ) before statement.");
                }
            }
            else
            {
                tkerr("Invalid statement.");
            }
        }
        else
        {
            tkerr("Missing ( before statement.");
        }
    }

    if (consume(RETURN))
    {
        expr();
        return consume(SEMICOLON);
    }
    else
    {
        tkerr("Invalid RETURN.");
    }

    if (expr()) 
    {
        return consume(SEMICOLON);
    }
    else
    {
        tkerr("Invalid expression.");
    }
    return consume(SEMICOLON);
}

bool stmCompound()
{
    if (consume(LACC))
    {
        while (varDef() || stm());
        return consume(RACC);
    }
    else
    {
        tkerr("Missing { before statement.");
    }
    return false;
}
