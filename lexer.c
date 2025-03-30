#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include"utils.h"
#include "parser.h"
#include"lexer.h"

Token *tokens; // Linked list of tokens
Token *lastTk; // Last token in list
int line = 1;  // Current line number

//Adds a token to the list
Token *addTk(int code) 
{
    Token *tk = (Token *)safeAlloc(sizeof(Token));
    tk->code = code;
    tk->line = line;
    tk->next = NULL;
    
    if (lastTk) {lastTk->next = tk;}
    else {tokens = tk;}

    lastTk = tk;
    return tk;
}

char *extract(const char *begin, const char *end)
{
    size_t len = end - begin;
    char *text = (char *)safeAlloc(len + 1);
    memcpy(text, begin, len);
    text[len] = '\0';
    return text;
}

Token *tokenize(const char *pch) {
    const char *start;
    Token *tk;

    for (;;) 
    {
        switch (*pch) 
        {
            case ' ': case '\t': pch++; break;
            case '\r':
                if (pch[1] == '\n') pch++;
            case '\n':
                line++;
                pch++;
                break;
            case '\0': addTk(END); return tokens;

            // Delimiters
            case ',': addTk(COMMA); pch++; break;
            case ';': addTk(SEMICOLON); pch++; break;
            case '(': addTk(LPAR); pch++; break;
            case ')': addTk(RPAR); pch++; break;
            case '[': addTk(LBRACKET); pch++; break;
            case ']': addTk(RBRACKET); pch++; break;
            case '{': addTk(LACC); pch++; break;
            case '}': addTk(RACC); pch++; break;
            
            // Operators
            case '+': addTk(ADD); pch++; break;
            case '-': addTk(SUB); pch++; break;
            case '*': addTk(MUL); pch++; break;
            case '/':
                if (pch[1] == '/')
                {  
                    pch += 2;  // Skip "//"
                    while (*pch != '\n' && *pch != '\0') pch++;  // Ignore rest of line
                    continue;  // Restart loop to process next token
                }
                else
                {
                    addTk(DIV); // If it's just '/', treat it as a division operator
                    pch++;
                    break;            
                }
                    
            case '.': addTk(DOT); pch++; break;
            case '!':
                if (pch[1] == '=') { addTk(NOTEQ); pch += 2;}
                else{ addTk(NOT); pch++;}
                break;

            case '=':
                if (pch[1] == '=') { addTk(EQUAL); pch += 2; }
                else { addTk(ASSIGN); pch++;}
                break;
        
            case '<':
                if (pch[1] == '=') { addTk(LESSEQ); pch += 2;}
                else { addTk(LESS); pch++;}
                break;
            
            case '>':
                if (pch[1] == '=') { addTk(GREATEREQ); pch += 2;}
                else { addTk(GREATER); pch++; }
                break;
                
            case '&':
                if (pch[1] == '&') { addTk(AND); pch += 2; }
                else { err("Ai uitat al doilea &.");}
                break;

            case '|':
                if (pch[1] == '|') { addTk(OR); pch += 2; }
                else { err("Ai uitat al doilea |.");}
                break;
            
            case '0' ... '9':
            {
                start = pch;
				while (isdigit(*pch)) pch++;
				if ((isalpha(*pch) && strchr("eE.", *pch) == NULL))
                {
					err("invalid number format: %c (%d)",*pch,*pch);
				}
                
				if (*pch == '.')
                {
					pch++;
					while (isdigit(*pch)) pch++;
					if (*pch == 'e' || *pch == 'E') pch++;
					if (*pch == '+' || *pch == '-') pch++;
					while (isdigit(*pch)) pch++;
					if (isalpha(*pch) || strchr(",([", *pch)) {
						err("invalid floating number format: %c (%d)",*pch,*pch);
					}
					tk = addTk(DOUBLE);
					char* extracted = extract(start, pch);
					tk->d = atof(extracted);
					free(extracted);
				}
                else if (*pch == 'e' || *pch == 'E')
                {
					pch++;
					if (*pch == '+' || *pch == '-') pch++;
					while (isdigit(*pch)) pch++;
					if (isalpha(*pch) || strchr(",([", *pch)) {
						err("invalid floating point number format: %c (%d)",*pch,*pch);
					}
					tk = addTk(DOUBLE);
					char* extracted = extract(start, pch);
					tk->d = atof(extracted);
					free(extracted);
				}
                else
                {
					tk = addTk(INT);
					char* extracted = extract(start, pch);
					tk->i = atoi(extracted);
					free(extracted);
				}
				break;
            }

			case '"': //String literal
				start = ++pch; //Skip the opening quote
				while (*pch != '"' && *pch != '\0')
                {
					if (*pch == '\n') line++; //Handle multi-line string error
					pch++;
				}
				if (*pch == '\0') err("Ai uitat ghilimelele de final.");
				
				tk = addTk(STRING);
				tk->text = extract(start, pch);//Extract the string
				pch++; //Skip closing quote
				break;

            case '\'':
				if (pch[2] != '\'') err("invalid character format: %c (%d)",*pch,*pch);
				tk = addTk(CHAR);
				tk->c = pch[1];
				pch+=3;
				break;

            default:
                if (isalpha(*pch) || *pch == '_') {
                    for (start = pch++; isalnum(*pch) || *pch == '_'; pch++);
                    char *text = extract(start, pch);
                    
                    if      (!strcmp(text, "char")) addTk(TYPE_CHAR);
                    else if (!strcmp(text, "double")) addTk(TYPE_DOUBLE);
                    else if (!strcmp(text, "else")) addTk(ELSE);
                    else if (!strcmp(text, "if")) addTk(IF);
                    else if (!strcmp(text, "int")) addTk(TYPE_INT);
                    else if (!strcmp(text, "return")) addTk(RETURN);
                    else if (!strcmp(text, "struct")) addTk(STRUCT);
                    else if (!strcmp(text, "void")) addTk(VOID);
                    else if (!strcmp(text, "while")) addTk(WHILE);
                    else {
                        tk = addTk(ID);
                        tk->text = text;
                    }
                }
                else err("invalid char: %c (%d)", *pch, *pch);
        }
    }
}

void showTokens(const Token *tokens)
{
    FILE *file = fopen("lista-de-atomi-REZOLVATA.txt", "w");
    
    if (!file)
    {
        perror("Error opening file");
        return;
    }

    static const char *tokenNames[] = 
    {
        "ID", "TYPE_CHAR", "TYPE_DOUBLE", "ELSE", "IF", "TYPE_INT", "RETURN",
        "STRUCT", "VOID", "WHILE", "COMMA", "END", "SEMICOLON", "LPAR", "RPAR",
        "LBRACKET", "RBRACKET", "LACC", "RACC", "ADD", "SUB", "MUL", "DIV",
        "DOT", "AND", "OR", "NOT", "ASSIGN", "EQUAL", "NOTEQ", "LESS", "LESSEQ",
        "GREATER", "GREATEREQ", "INT", "DOUBLE", "CHAR", "STRING"
    };

    for (const Token *tk = tokens; tk; tk = tk->next) 
    {
        if (tk->code >= 0 && tk->code <= STRING)
        {
            fprintf(file, "%d\t%s", tk->line, tokenNames[tk->code]);
        } 
        else
        {
            fprintf(file, "%d\tUNKNOWN(%d)", tk->line, tk->code);
        }

        switch (tk->code) 
        {
            case ID:
            // case STRING:
            //     fprintf(file, ":%s", tk->text);
            //     break;
            // case INT:
            //     fprintf(file, ":%d", tk->i);
            //     break;
            // case DOUBLE:
            //     fprintf(file, ":%.2f", tk->d);
            //     break;
            // case CHAR:
            //     fprintf(file, ":%c", tk->c);
            //     break;
        }

        fprintf(file, "\n");
    }
    free((void *)tokens);
    fclose(file);
}