#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<stdarg.h>

#include "utils.h"
#include "parser.h"
#include "lexer.h"
#include "ad.h"

Token *iTk;
Token *consumedTk;
Symbol* owner = NULL;

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
			tkerr("Missing or invalid expression after >.");
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

// bool exprCast() {
//     Token *start = iTk;

//     if (consume(LPAR)) {
//         Type t;  // Declare a Type variable to pass to typeBase

//         if (typeBase(&t)) {
//             if (consume(RPAR)) {
//                 if (exprCast()) {
//                     // Optionally, store that this is a cast and attach type info
//                     return true;
//                 } else {
//                     tkerr("Invalid expression after type cast.");
//                 }
//             } else {
//                 tkerr("Missing ')' after type in cast expression.");
//             }
//         } else {
//             tkerr("Invalid type in cast expression.");
//         }
//     }

//     iTk = start;

//     if (exprUnary()) {
//         return true;
//     }

//     iTk = start;
//     return false;
// }

bool exprCast() {
    Token *start = iTk;

    if (consume(LPAR)) {
        Type t;
        if (typeBase(&t)) {
            arrayDecl(&t); // optional, so no need to check return value
            if (consume(RPAR)) {
                if (exprCast()) {
                    // Optionally mark this node as a cast with type `t`
                    return true;
                } else {
                    tkerr("Expected expression after cast type.");
                }
            } else {
                tkerr("Expected ')' after type in cast expression.");
            }
        } else {
            tkerr("Expected type name after '(' in cast expression.");
        }
        iTk = start; // backtrack if cast fails
        return false;
    }

    // Try the alternative: exprUnary
    if (exprUnary()) return true;

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
						tkerr("invalid or missing expression after ,");
					}
				}
			} else {
				tkerr("invalid or missing expression after (");
			}

			if (consume(RPAR)) {
				return true;
			} else {
				tkerr("missing ) in expression");
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
    // printf("consume(%s) ",tkCodeName(code));

    if(iTk->code==code)
    {
        consumedTk=iTk;
        iTk=iTk->next;
        // printf(" => consumed\n");
        return true;
    }

    // printf(" => found %s\n",tkCodeName(iTk->code));
    return false;
}

// bool structDef() {
// 	Token* start = iTk;
// 	if (consume(STRUCT)) {
// 		if (consume(ID)) {
// 			if (consume(LACC)) {
// 				while (varDef());

// 				if (consume(RACC)) {
// 					if (consume(SEMICOLON)) {
// 						return true;
// 					} else {
// 						tkerr("missing ; after }");
// 					}
// 				} else {
// 					tkerr("missing } in struct");
// 				}
// 			} else {
// 				if (iTk->code == ID) {
// 					iTk = start;
// 					return false;
// 				}
// 				tkerr("missing { after identifer in struct");
// 			}
// 		} else {
// 			tkerr("invalid or missing indentifier after struct");
// 		}
// 	}

// 	iTk = start;
// 	return false;
// }

bool structDef() {
    Token* start = iTk;

    if (consume(STRUCT)) {
        Token* tkName = iTk;
        if (consume(ID)) {
            // Semantic: Check for redefinition
            Symbol* s = findSymbolInDomain(symTable, tkName->text);
            if (s) tkerr("symbol redefinition: %s", tkName->text);
            
            // Semantic: Add struct symbol
            s = addSymbolToDomain(symTable, newSymbol(tkName->text, SK_STRUCT));
            s->type.tb = TB_STRUCT;
            s->type.s = s;
            s->type.n = -1;
            pushDomain();
            owner = s;

            if (consume(LACC)) {
                while (varDef());

                if (consume(RACC)) {
                    if (consume(SEMICOLON)) {
                        // Semantic: cleanup
                        owner = NULL;
                        dropDomain();
                        return true;
                    } else {
                        tkerr("missing ; after }");
                    }
                } else {
                    tkerr("missing } in struct");
                }
                owner = NULL; // Ensure cleanup even on error
                dropDomain();
            } else {
                if (iTk->code == ID) {
                    iTk = start;
                    return false;
                }
                tkerr("missing { after identifier in struct");
            }
        } else {
            tkerr("invalid or missing identifier after struct");
        }
    }

    iTk = start;
    return false;
}

// bool varDef()
// {
//     Token *start = iTk;
//     if (typeBase())
//     {
//         if(consume(ID))
//         {
//             if(arrayDecl()){}
//             if (consume(SEMICOLON))
//             {
//                 return true;
//             }
//             else
//             {
//                 tkerr("Missing semicolon ; after variable declaration.");
//             }
//         }
//         else
//         {
//             tkerr("Expected identifier after type.");
//         }
//     }
//     // else
//     // {
//     //     tkerr("Invalid syntax");
//     // }
//     iTk = start;
//     return false;
// }

bool varDef() {
    Token *start = iTk;
    Type t;

    if (typeBase(&t)) { // typeBase fills in the Type t
        Token* tkName = iTk;

        if (consume(ID)) {
            if (arrayDecl(&t)) {
                if (t.n == 0)
                    tkerr("a vector variable must have a specified dimension");
            }

            if (consume(SEMICOLON)) {
                // Semantic: Check for redefinition
                Symbol* var = findSymbolInDomain(symTable, tkName->text);
                if (var)
                    tkerr("symbol redefinition: %s", tkName->text);

                var = newSymbol(tkName->text, SK_VAR);
                var->type = t;
                var->owner = owner;
                addSymbolToDomain(symTable, var);

                if (owner) {
					switch (owner->kind) {
						case SK_FN:
							var->varIdx = symbolsLen(owner->fn.locals);
							addSymbolToList(&owner->fn.locals, dupSymbol(var));
							break;
				
						case SK_STRUCT:
							var->varIdx = typeSize(&owner->type);
							addSymbolToList(&owner->structMembers, dupSymbol(var));
							break;
				
						case SK_VAR:
						case SK_PARAM:
							// These kinds are not expected here, so just ignore or warn
							// Optional: log a warning or handle if needed
							break;
					}
				} else {
					var->varMem = safeAlloc(typeSize(&t));
				}

                return true;
            } else {
                tkerr("Missing semicolon ; after variable declaration.");
            }
        } else {
            tkerr("Expected identifier after type.");
        }
    }

    iTk = start;
    return false;
}

// bool fnDef() {
//     Token* start = iTk;
//     Type t;

//     bool isVoid = false;

//     if (consume(VOID)) {
//         isVoid = true;
//         t.tb = TB_VOID;
//         t.n = -1;
//     } else if (typeBase(&t)) {
//         // Type t is filled by typeBase
//     } else {
//         iTk = start;
//         return false;
//     }

//     Token* tkName = iTk;
//     if (consume(ID)) {
//         // Semantic: check for redefinition
//         Symbol* fn = findSymbolInDomain(symTable, tkName->text);
//         if (fn)
//             tkerr("symbol redefinition: %s", tkName->text);

//         // Create new function symbol
//         fn = newSymbol(tkName->text, SK_FN);
//         fn->type = t;
//         addSymbolToDomain(symTable, fn);

//         owner = fn;
//         fn->fn.inParams = NULL;
//         fn->fn.locals = NULL;
//         pushDomain();

//         if (consume(LPAR)) {
//             if (fnParam()) {
//                 while (consume(COMMA)) {
//                     if (!fnParam())
//                         tkerr("invalid or missing parameter after ','");
//                 }
//             }

//             if (consume(RPAR)) {
//                 if (stmCompound()) {
//                     // End of function definition
//                     owner = NULL;
//                     dropDomain();
//                     return true;
//                 } else {
//                     tkerr("invalid or missing statement in function body");
//                 }
//             } else {
//                 tkerr("missing ')' in function definition");
//             }
//         } else {
//             tkerr("missing '(' after function name");
//         }

//         owner = NULL;
//         dropDomain();  // Cleanup even on error
//     } else {
//         tkerr("missing or invalid identifier in function definition");
//     }

//     iTk = start;
//     return false;
// }

bool fnDef() {
    Token* start = iTk;
    Type t;

    // ( typeBase[&t] | VOID {t.tb=TB_VOID;} )
    if (consume(VOID)) {
        t.tb = TB_VOID;
        t.n = -1;
    } else if (!typeBase(&t)) {
        iTk = start;
        return false;
    }

    // ID[tkName]
    Token* tkName = iTk;
    if (!consume(ID)) {
		iTk = start;
		return false;
	}
	
	if (!consume(LPAR)) {
		iTk = start;
		return false;
	}

    // Symbol check: redefinition
    Symbol* fn = findSymbolInDomain(symTable, tkName->text);
    if (fn) {
        tkerr("Symbol redefinition: %s", tkName->text);
    }

    // Create function symbol
    fn = newSymbol(tkName->text, SK_FN);
    fn->type = t;
    fn->fn.locals = NULL;
    fn->fn.params = NULL;
    addSymbolToDomain(symTable, fn);

    // Set function context
    owner = fn;
    pushDomain();

    // ( fnParam ( , fnParam )* )?
    if (fnParam()) {
        while (consume(COMMA)) {
            if (!fnParam()) {
                tkerr("Missing or invalid parameter after ','.");
            }
        }
    }

    // RPAR
    if (!consume(RPAR)) {
        tkerr("Missing ')' after parameter list.");
    }

    // stmCompound[false] (function body)
    if (!stmCompound(false)) {
        tkerr("Missing or invalid function body.");
    }

    // Cleanup domain/scope
    dropDomain();
    owner = NULL;

    return true;
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

// bool typeBase()
// {
//     Token* start = iTk;
//     if(consume(TYPE_INT)){
//         return true;
//     }
//     if(consume(TYPE_DOUBLE)){
//         return true;
//     }
//     if(consume(TYPE_CHAR)){
//         return true;
//     }
//     if(consume(STRUCT)){
//         if(consume(ID)){
//             return true;
//         }
//     }

//     iTk = start;
//     return false;
// }

bool typeBase(Type* t) {
    Token* start = iTk;
    t->n = -1;  // default dimension

    if (consume(TYPE_INT)) {
        t->tb = TB_INT;
        return true;
    }

    if (consume(TYPE_DOUBLE)) {
        t->tb = TB_DOUBLE;
        return true;
    }

    if (consume(TYPE_CHAR)) {
        t->tb = TB_CHAR;
        return true;
    }

    if (consume(STRUCT)) {
        Token* tkName = iTk;
        if (consume(ID)) {
            t->tb = TB_STRUCT;
            t->s = findSymbol(tkName->text);
            if (!t->s)
                tkerr("undefined struct: %s", tkName->text);
            return true;
        } else {
            tkerr("missing identifier after 'struct'");
        }
    }

    iTk = start;
    return false;
}

// bool arrayDecl()
// {
//     Token *start = iTk;
//     if (consume(LBRACKET))
//     {
//         if(consume(INT)){}
//         if (consume(RBRACKET)) {
//             return true;
//         }else{
//             tkerr("Missing ] before statement.");
//         }
//     }
//     // else
//     // {
//     //     tkerr("Missing [ before statement.");
//     // }
//     iTk = start;
//     return false;
// }

bool arrayDecl(Type *t){
	if(consume(LBRACKET)){
		if(consume(INT)){
			Token *tkSize=consumedTk;
			t->n=tkSize->i;
		}else{
			t->n=0; // array fara dimensiune: int v[]
		}
		if(consume(RBRACKET)){
			return true;
		}
		else tkerr("missing ] or invalid expression inside [...]");
	}
	return false;
}

// bool fnParam()
// {
//     Token *start=iTk; 
//     if(typeBase())
//     {
//         if(consume(ID))
//         {
//             if(arrayDecl()){ 
//             }
//             return true;
// 		}
//         else
//         {
//             tkerr("invalid or missing indentifier in function parameter.");
//         }
//     }
//     iTk=start;
//     return false;
// }

// bool fnParam() {
//     Token* start = iTk;
//     Type t;

//     if (typeBase(&t)) {
//         Token* tkName = iTk;

//         if (consume(ID)) {
//             if (arrayDecl(&t)) {
//                 if (t.n == 0)
//                     tkerr("a vector parameter must have a specified dimension");
//             }

//             // Semantic: check for redefinition
//             Symbol* param = findSymbolInDomain(symTable, tkName->text);
//             if (param)
//                 tkerr("symbol redefinition: %s", tkName->text);

//             param = newSymbol(tkName->text, SK_VAR);
//             param->type = t;
//             param->owner = owner;

//             // Link to function's local variable list (as parameter)
//             param->varIdx = symbolsLen(owner->fn.locals);
//             addSymbolToDomain(symTable, param);
//             addSymbolToList(&owner->fn.locals, dupSymbol(param));

//             return true;
//         } else {
//             tkerr("invalid or missing identifier in function parameter.");
//         }
//     }

//     iTk = start;
//     return false;
// }

bool fnParam() {
    Token* start = iTk;
    Type t;

    if (typeBase(&t)) {
        Token* tkName = iTk;

        if (consume(ID)) {
            if (arrayDecl(&t)) {
                if (t.n == 0)
                    tkerr("a vector parameter must have a specified dimension");
            }

            // Semantic actions
            Symbol* param = findSymbolInDomain(symTable, tkName->text);
            if (param)
                tkerr("symbol redefinition: %s", tkName->text);

            param = newSymbol(tkName->text, SK_PARAM);
            param->type = t;
            param->owner = owner;
            param->paramIdx = symbolsLen(owner->fn.params);

            addSymbolToDomain(symTable, param);
            addSymbolToList(&owner->fn.params, dupSymbol(param));

            return true;
        } else {
            tkerr("invalid or missing identifier in function parameter.");
        }
    }

    iTk = start;
    return false;
}

// bool stm()
// {
//     Token* start = iTk;

// 	if (stmCompound()) {
// 		return true;
// 	}

// 	iTk = start;

// 	if (consume(IF)) {
// 		if (consume(LPAR)) {
// 			if (expr()) {
// 				if (consume(RPAR)) {
// 					if (stm()) {
// 						if (consume(ELSE)) {
// 							if (stm()) {
// 								return true;
// 							} else {
// 								tkerr("missing or invalid statement in ELSE branch");
// 							}
// 						}
// 						return true;
// 					} else {
// 						tkerr("invalid or missing satement in IF statement");
// 					}
// 				} else {
// 					tkerr("missing ) after expression in IF");
// 				}
// 			} else {
// 				tkerr("missing or invalid expression in IF");
// 			}
// 		} else {
// 			tkerr("missing ( after IF");
// 		}
// 	}

// 	iTk = start;

// 	if (consume(WHILE)) {
// 		if (consume(LPAR)) {
// 			if (expr()) {
// 				if (consume(RPAR)) {
// 					if (stm()) {
// 						return true;
// 					} else {
// 						tkerr("invalid or missing statement in WHILE statement");
// 					}
// 				} else {
// 					tkerr("missing ) in WHILE statement");
// 				}
// 			} else {
// 				tkerr("missing or invalid expression in WHILE statement");
// 			}
// 		} else {
// 			tkerr("missing ( in WHILE satement");
// 		}
// 	}

// 	iTk = start;

// 	if (consume(RETURN)) {
// 		if (expr()){}
// 		if (consume(SEMICOLON)) {
// 			return true;
// 		} else {
// 			tkerr("missing ';' after RETURN statement");
// 		}
// 	}

// 	iTk = start;

// 	if (consume(SEMICOLON)) {
// 		return true;
// 	}

// 	if (expr()) {
// 		if (consume(SEMICOLON)) {
// 			return true;
// 		} else {
// 			tkerr("missing ';' after statement");
// 		}
// 	}

// 	iTk = start;
// 	return false;
// }

bool stm() {
    Token* start = iTk;

    // Compound statement (introduces a new scope)
    if (stmCompound(true)) {
        return true;
    }

    // IF-ELSE statement
    if (consume(IF)) {
        if (consume(LPAR)) {
            if (expr()) {
                if (consume(RPAR)) {
                    if (stm()) {
                        if (consume(ELSE)) {
                            if (stm()) {
                                return true;
                            } else {
                                tkerr("Missing or invalid statement in ELSE branch.");
                            }
                        }
                        return true; // IF without ELSE is valid
                    } else {
                        tkerr("Missing or invalid statement in IF.");
                    }
                } else {
                    tkerr("Missing ')' after expression in IF.");
                }
            } else {
                tkerr("Missing or invalid expression in IF.");
            }
        } else {
            tkerr("Missing '(' after IF.");
        }
        iTk = start; return false; // Backtrack after IF failure
    }

    // WHILE statement
    if (consume(WHILE)) {
        if (consume(LPAR)) {
            if (expr()) {
                if (consume(RPAR)) {
                    if (stm()) {
                        return true;
                    } else {
                        tkerr("Missing or invalid statement in WHILE.");
                    }
                } else {
                    tkerr("Missing ')' after expression in WHILE.");
                }
            } else {
                tkerr("Missing or invalid expression in WHILE.");
            }
        } else {
            tkerr("Missing '(' after WHILE.");
        }
        iTk = start; return false;
    }

    // RETURN statement (with optional expression)
    if (consume(RETURN)) {
        expr(); // optional
        if (consume(SEMICOLON)) {
            return true;
        } else {
            tkerr("Missing ';' after RETURN.");
            iTk = start; return false;
        }
    }

    // Empty statement (just a semicolon)
    if (consume(SEMICOLON)) {
        return true;
    }

    // Expression statement
    if (expr()) {
        if (consume(SEMICOLON)) {
            return true;
        } else {
            tkerr("Missing ';' after expression.");
            iTk = start; return false;
        }
    }

    // If all fails, backtrack and return false
    iTk = start;
    return false;
}

// bool stmCompound()
// {
//     Token* start = iTk;

// 	if (consume(LACC)) {
// 		while (true) {
// 			if (varDef()) {} 
// 			else if (stm()) {}
// 			else {
// 				break;
// 			}
// 		}
// 		if (consume(RACC)) {
// 			return true;
// 		} else {
// 			tkerr("missing '}' after statement");
// 		}
// 	}

// 	iTk = start;
// 	return false;
// }

bool stmCompound(bool newDomain) {
    Token* start = iTk;

    if (consume(LACC)) {
        if (newDomain)
            pushDomain();

        while (true) {
            if (varDef()) {
                // handled inside varDef
            } else if (stm()) {
                // handled inside stm
            } else {
                break;
            }
        }

        if (consume(RACC)) {
            if (newDomain)
                dropDomain();
            return true;
        } else {
            tkerr("missing '}' after compound statement");
        }

        // Only drop if we had pushed
        if (newDomain)
            dropDomain();
    }

    iTk = start;
    return false;
}

#pragma GCC diagnostic pop

//LA IMPLEMENTAREA MEA NU E CEVA BINE