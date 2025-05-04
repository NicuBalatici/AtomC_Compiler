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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winfinite-recursion"

bool expr()
{
    Token* start = iTk;

	if (exprAssign()) {
		return true;
	}

	iTk = start;
	return false;
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
            else
            {
                tkerr("Invalid ASSIGN syntax.");
            }
        }
    }
    // else
    // {
    //     tkerr("Missing = before statement.");
    // }
    iTk = start;

	if (exprOr()) {
		return true;
	}

	iTk = start;
	return false;
}

bool exprOrPrim() {
	if (consume(OR)) {
		if (exprAnd()) {
			if (exprOrPrim()) {
				return true;
			}
		} else {
			tkerr("invalid || expression");
		}
	}

	return true;
}

bool exprOr()
{
    Token* start = iTk;

	if (exprAnd())
    {
		if (exprOrPrim())
        {
			return true;
		}
	}

	iTk = start;
	return false;
}

bool exprAndPrim() {
	if (consume(AND))
    {
		if (exprEq())
        {
			if (exprAndPrim())
            {
				return true;
			}
		} else {
			tkerr("invalid && expression");
		}
	}

	return true;
}

bool exprAnd()
{
    Token* start = iTk;

	if (exprEq())
    {
		if (exprAndPrim())
        {
			return true;
		}
	}

	iTk = start;
	return false;
}

bool exprEqPrim()
{
	if (consume(EQUAL))
    {
		if (exprRel())
        {
			if (exprEqPrim())
            {
				return true;
			}
		} else {
			tkerr("invalid == expression");
		}
	}

	if (consume(NOTEQ))
    {
		if (exprRel())
        {
			if (exprEqPrim())
            {
				return true;
			}
		} else {
			tkerr("invalid != expression");
		}
	}

	return true;
}

bool exprEq()
{
    Token* start = iTk;

	if (exprRel()) {
		if (exprEqPrim()) {
			return true;
		}
	}

	iTk = start;
	return false;
}

bool exprRelPrim() {
	if (consume(LESS)) {
		if (exprAdd()) {
			if (exprRelPrim()) {
				return true;
			}
		} else {
			tkerr("invalid < expression");
		}
	}

	if (consume(LESSEQ)) {
		if (exprAdd()) {
			if (exprRelPrim()) {
				return true;
			}
		} else {
			tkerr("invalid <= expression");
		}
	}

	if (consume(GREATER)) {
		if (exprAdd()) {
			if (exprRelPrim()) {
				return true;
			}
		} else {
			tkerr("invalid > expression");
		}
	}

	if (consume(GREATEREQ)) {
		if (exprAdd()) {
			if (exprRelPrim()) {
				return true;
			}
		} else {
			tkerr("invalid >= expression");
		}
	}

	return true;
}

bool exprRel() {
	Token* start = iTk;

	if (exprAdd()) {
		if (exprRelPrim()) {
			return true;
		}
	}

	iTk = start;
	return false;
}

bool exprAddPrim() {
	if (consume(ADD)) {
		if (exprMul()) {
			if (exprAddPrim()) {
				return true;
			}
		} else {
			tkerr("invlaid '+' expression");
		}
	}

	if (consume(SUB)) {
		if (exprMul()) {
			if (exprAddPrim()) {
				return true;
			}
		} else {
			tkerr("invalid '-' expression");
		}
	}

	return true;
}

bool exprAdd() {
	Token* start = iTk;

	if (exprMul()) {
		if (exprAddPrim()) {
			return true;
		}
	}

	iTk = start;
	return false;
}

bool exprMulPrim() {
	if (consume(MUL)) {
		if (exprCast()) {
			if (exprMulPrim()) {
				return true;
			}
		} else {
			tkerr("invalid * expression");
		}
	}

	if (consume(DIV)) {
		if (exprCast()) {
			if (exprMulPrim()) {
				return true;
			}
		} else {
			tkerr("invalid / expression");
		}
	}

	return true;
}

bool exprMul() {
	Token* start = iTk;

	if (exprCast()) {
		if (exprMulPrim()) {
			return true;
		}
	}

	iTk = start;
	return false;
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
                else
                {
                    tkerr("Invalid syntax in cast expression.");
                }
            }
            else
            {
                tkerr("Missing ) before syntax.");
            }
        }
        else
        {
            tkerr("Invalid syntax in cast expression.");
        }
    }

    iTk = start;

	if (exprUnary()) {
		return true;
	}

	iTk = start;
	return false;
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
        else
        {
            tkerr("Invalid syntax after '-'.");
        }
    }

    if (consume(NOT))
    {
        if(exprUnary())
        {
            return true;
        }
        else
        {
            tkerr("Invalid syntax after '!'.");
        }
    }
   
    iTk = start;

	if (exprPostfix()) {
		return true;
	}

    iTk = start;
    return exprPostfix();
}


bool exprPrimary() {
	Token* start = iTk;

	if (consume(ID)) {
		if (consume(LPAR)) {
			if (expr()) {
				while (consume(COMMA)) {
					if (expr()) {
					} else {
						tkerr("invalid or missing expression after ','");
					}
				}
			} else {
				tkerr("invalid or missing expression after '('");
			}

			if (consume(RPAR)) {
				return true;
			} else {
				tkerr("missing ')' in expression");
			}
		}

		return true;
	}

	iTk = start;

	if (consume(INT) || consume(DOUBLE) || consume(CHAR) || consume(STRING)) {
		return true;
	}

	iTk = start;

	if (consume(LPAR)) {
		if (expr()) {
			if (consume(RPAR)) {
				return true;
			} else {
				tkerr("missing ')' after expression");
			}
		} else {
			tkerr("invalid or missing expression after '('");
		}
	}

	iTk = start;
	return false;
}

bool exprPostfixPrim() {
	Token* start = iTk;

	if (consume(LBRACKET)) {
		if (expr()) {
			if (consume(RBRACKET)) {
				if (exprPostfixPrim()) {
					return true;
				}
			} else {
				tkerr("missing '(' in expression");
			}
		} else {
			tkerr("missing or invalid expression after '('");
		}
	}

	iTk = start;

	if (consume(DOT)) {
		if (consume(ID)) {
			if (exprPostfixPrim()) {
				return true;
			}
		} else {
			tkerr("missing or invalid identifier after '.'");
		}
	}

	return true;
}

bool exprPostfix() {
	Token* start = iTk;

	if (exprPrimary()) {
		if (exprPostfixPrim()) {
			return true;
		}
	}

	iTk = start;
	return false;
}
char *tkCodeName(int code)
{
    switch (code)
    {
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

bool structDef() {
	Token* start = iTk;
	if (consume(STRUCT)) {
		if (consume(ID)) {
			if (consume(LACC)) {
				while (varDef());

				if (consume(RACC)) {
					if (consume(SEMICOLON)) {
						return true;
					} else {
						tkerr("missing ';' after '}'");
					}
				} else {
					tkerr("missing '}' in struct");
				}
			} else {
				if (iTk->code == ID) {
					iTk = start;
					return false;
				}
				tkerr("missing '{' after identifer in struct");
			}
		} else {
			tkerr("invalid or missing indentifier after struct");
		}
	}

	iTk = start;
	return false;
}

bool varDef()
{
    Token *start = iTk;
    if (typeBase())
    {
        if(consume(ID))
        {
            if(arrayDecl()){}
            if (consume(SEMICOLON))
            {
                return true;
            }
            else
            {
                tkerr("Missing semicolon ';' after variable declaration.");
            }
        }
        else
        {
            tkerr("Expected identifier after type.");
        }
    }
    // else
    // {
    //     tkerr("Invalid syntax");
    // }
    iTk = start;
    return false;
}

bool fnDef() {
	Token* start = iTk;

	if (typeBase() || consume(VOID)) {
		if (consume(ID)) {
			if (consume(LPAR)) {
				if (fnParam()) {
					while (consume(COMMA)) {
						if (fnParam()) {
						} else {
							tkerr("invalid or missing parameter after ','");
						}
					}
				}
				if (consume(RPAR)) {
					if (stmCompound()) {
						return true;
					} else {
						tkerr("invalid or missing statement in function definition");
					}
				} else {
					tkerr("missing ')' in function definition");
				}
			}
		} else {
			tkerr("missing or invalid identifier in function definition");
		}
	}

	iTk = start;
	return false;
}


bool unit()
{
    while(true)
    { 
        if(structDef()){}
        else if(fnDef()){}
        else if(varDef()){}
        else {
            break;
        }
    }
    
    if(consume(END)){
        return true;
    }
    return true;
}

bool typeBase()
{
    Token* start = iTk;
    if(consume(TYPE_INT)){
        return true;
    }
    if(consume(TYPE_DOUBLE)){
        return true;
    }
    if(consume(TYPE_CHAR)){
        return true;
    }
    if(consume(STRUCT)){
        if(consume(ID)){
            return true;
        }
    }

    iTk = start;
    return false;
}

bool arrayDecl()
{
    Token *start = iTk;
    if (consume(LBRACKET))
    {
        if(consume(INT)){}
        if (consume(RBRACKET)) {
            return true;
        }else{
            tkerr("Missing ] before statement.");
        }
    }
    // else
    // {
    //     tkerr("Missing [ before statement.");
    // }
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
            if(arrayDecl()){ 
            }
            return true;
		}
        else
        {
            tkerr("invalid or missing indentifier in function parameter.");
        }
    }
    iTk=start;
    return false;
}

bool stm()
{
    Token* start = iTk;

	if (stmCompound()) {
		return true;
	}

	iTk = start;

	if (consume(IF)) {
		if (consume(LPAR)) {
			if (expr()) {
				if (consume(RPAR)) {
					if (stm()) {
						if (consume(ELSE)) {
							if (stm()) {
								return true;
							} else {
								tkerr("missing or invalid statement in ELSE branch");
							}
						}
						return true;
					} else {
						tkerr("invalid or missing satement in IF statement");
					}
				} else {
					tkerr("missing ')' after expression in IF");
				}
			} else {
				tkerr("missing or invalid expression in IF");
			}
		} else {
			tkerr("missing '(' after 'if'");
		}
	}

	iTk = start;

	if (consume(WHILE)) {
		if (consume(LPAR)) {
			if (expr()) {
				if (consume(RPAR)) {
					if (stm()) {
						return true;
					} else {
						tkerr("invalid or missing statement in WHILE statement");
					}
				} else {
					tkerr("missing ')' in WHILE statement");
				}
			} else {
				tkerr("missing or invalid expression in WHILE statement");
			}
		} else {
			tkerr("missing '(' in WHILE satement");
		}
	}

	iTk = start;

	if (consume(RETURN)) {
		if (expr()){}
		if (consume(SEMICOLON)) {
			return true;
		} else {
			tkerr("missing ';' after RETURN statement");
		}
	}

	iTk = start;

	if (consume(SEMICOLON)) {
		return true;
	}

	if (expr()) {
		if (consume(SEMICOLON)) {
			return true;
		} else {
			tkerr("missing ';' after statement");
		}
	}

	iTk = start;
	return false;
}

bool stmCompound()
{
    Token* start = iTk;

	if (consume(LACC)) {
		while (true) {
			if (varDef()) {} 
			else if (stm()) {}
			else {
				break;
			}
		}
		if (consume(RACC)) {
			return true;
		} else {
			tkerr("missing '}' after statement");
		}
	}

	iTk = start;
	return false;
}

#pragma GCC diagnostic pop

//LA IMPLEMENTAREA MEA NU E CEVA BINE