#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lex.h"
#include "macros.h"
#include "hash.h"
#include "symbol.h"
#include "umalloc.h"
#include "keyword.h"

struct lex lex;
static int peek; 
static FILE *input_file;

void
set_file(FILE *file)
{
	return_if_fail(file != NULL);
	
	input_file = file;
}

void
skip_comment(void)
{
	while(TRUE) {
		peek = fgetc(input_file);
		if (peek == '\n')
			break;
	}
}

token_t
get_next_token(void)
{
	int used, len, neg;
	double number, mult;
	int power_temp, power;
	char *id, *string;
	struct symbol *symbol;
	struct keyword *keyword;
	int tmp;

	neg = 0;
	id  = NULL;

	while (TRUE) {
		switch(peek) {
		case 0: 
		case ' ':
		case '\t':
			peek = fgetc(input_file);
			continue;
		case '#':
			skip_comment();
			break;
		default:
			break;
		}

		break;
	}

	switch (lex.token) {
	case TOKEN_ID:
		if (lex.id != NULL) 
			ufree(lex.id);
		break;
	case TOKEN_STRING:	
		if (lex.string != NULL)
			ufree(lex.string);
		break;
	default:
		break;
	}
	
	memset(&lex, 0, sizeof(lex));
	
	if (isalpha(peek) || peek == '_') {
		used = len = 0;
		do {
			if ((used + 1) >= len) {
				len += 64;
				id   = urealloc(id, len);
			}

			id[used++] = peek;
			peek = fgetc(input_file);

		} while (isalnum(peek) || peek == '_');

		id[used++] = 0;
			
		keyword = keyword_table_lookup(id);

		if (keyword != NULL) {
			lex.token = keyword->token;
			ufree(id);
			return lex.token;
		}
		
		lex.id = ustrdup(id);
		lex.token = TOKEN_ID;
		
		ufree(id);	
		return TOKEN_ID;	
	}
	

	switch (peek) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
do_number:	
		switch(peek) {
		case '-':
			neg = 1;
		case '+':
			peek = fgetc(input_file);
			break;
		}

		number  = 0.0;
		power_temp = power = 0;

		while (isdigit(peek)) {
			number *= 10;
			number += peek - '0';
			peek = fgetc(input_file); 
		}
		
		if (peek == '.') {
			peek = fgetc(input_file);
			while(isdigit(peek)) {
				number *= 10;
				number += peek - '0';
				peek = fgetc(input_file);
				power_temp--;
			}
		} 
	
		if (peek == 'e' || peek == 'E') {
			int exp_neg = 0;
			
			peek = fgetc(input_file);
			switch(peek) {
			case '-':
				peek = fgetc(input_file);
				exp_neg++;
				break;
			case '+':
				peek = fgetc(input_file);
				break;
			}
			
			while(isdigit(peek)) {
				power *= 10;
				power += peek - '0';
				peek   = fgetc(input_file);
			}
			
			power = (exp_neg) ? -power : power;	
	
		}
		
		power += power_temp;
		power_temp =  (power < 0) ? -power : power;
		
		mult = 10.0;
		while(power_temp) {
			if (power_temp & 0x1) {
				if (power < 0)
					number /= mult;
				else
					number *= mult;	
			}
			mult *= mult;
			power_temp >>= 1;
		}
		
		lex.real  = (neg) ? -number : number;
		lex.token = TOKEN_DOUBLE;

		return TOKEN_DOUBLE;
	case '+':
		peek = 0;
		lex.token = TOKEN_PLUS;
		return TOKEN_PLUS;
	case '-':
		tmp = fgetc(input_file);
		if (isdigit(tmp)) {
			ungetc(tmp, input_file);
			goto do_number;
		}
		peek = 0;
		lex.token = TOKEN_MINUS;
		return TOKEN_MINUS;
	case '*':
		peek = 0;
		lex.token = TOKEN_ASTERIK;
		return TOKEN_ASTERIK;
	case '/':
		peek = 0;
		lex.token = TOKEN_SLASH;
		return TOKEN_SLASH;
	case '(':
		peek = 0;
		lex.token = TOKEN_LPARENTH;
		return TOKEN_LPARENTH;
	case ')':
		peek = 0;
		lex.token = TOKEN_RPARENTH;
		return TOKEN_RPARENTH;
	case ',':
		peek = 0;
		lex.token = TOKEN_COMMA;
		return TOKEN_COMMA;
	case '!':
		peek = fgetc(input_file);
		if (peek == '=') {
			peek = 0;
			lex.token = TOKEN_NE;
			return TOKEN_NE;
		}
		lex.token = TOKEN_NOT;
		return TOKEN_NOT;
	case '&':
		peek = fgetc(input_file);
		if (peek == '&') {
			peek = 0;
			lex.token = TOKEN_AND;
			return TOKEN_AND;
		}
		lex.token = TOKEN_UNKNOWN;
		return TOKEN_UNKNOWN;
	case '|':
		peek = fgetc(input_file);
		if (peek == '|') {
			peek = 0;
			lex.token = TOKEN_OR;
			return TOKEN_OR;
		}
		lex.token = TOKEN_UNKNOWN;
		return TOKEN_UNKNOWN;
	case '=':
		peek = fgetc(input_file);
		if (peek == '=') {
			peek = 0;
			lex.token = TOKEN_EQ;
			return TOKEN_EQ;
		}
		lex.token = TOKEN_EQUALITY;
		return TOKEN_EQUALITY;
	case '>':
		peek = fgetc(input_file);
		if (peek == '=') {
			peek = 0;
			lex.token = TOKEN_GE;
			return TOKEN_GE;
		}
		lex.token = TOKEN_GT;
		return TOKEN_GT;
	case '<':
		peek = fgetc(input_file);
		if (peek == '=') {
			peek = 0;
			lex.token = TOKEN_LE;
			return TOKEN_LE;	
		}
		lex.token = TOKEN_LT;
		return TOKEN_LT;
	case '{':
		peek = 0;
		lex.token = TOKEN_LBRACE;
		return TOKEN_LBRACE;
	case '}':
		peek = 0;
		lex.token = TOKEN_RBRACE;
		return TOKEN_RBRACE;
	case '[':
		peek = 0;
		lex.token = TOKEN_LBRACKET;
		return TOKEN_LBRACKET;
	case ']':
		peek = 0;
		lex.token = TOKEN_RBRACKET;
		return TOKEN_RBRACKET;
	case ';':
		peek = 0;
		lex.token = TOKEN_SEMICOLON;
		return TOKEN_SEMICOLON;	
	case '"':
		string = NULL;
		used = len = 0;
		peek = fgetc(input_file);

		do {
			if (peek == '\n') {
				if (string != NULL)
					ufree(string);

				lex.token = TOKEN_UNKNOWN;
				return TOKEN_UNKNOWN;	
			}
			
			if ((used + 1) >= len) {
				len += 4;
				string = urealloc(string, len);
			}
			
			string[used++] = peek;

			peek = fgetc(input_file);
				
		} while(peek != '"');	
		
		string[used] = '\0';

		peek = 0;
		lex.string = ustrdup(string); 
		lex.token  = TOKEN_STRING;
		ufree(string);
		return TOKEN_STRING;
	case '^':
		peek = 0;
		lex.token = TOKEN_CARET;
		return TOKEN_CARET;	
	case '\n':
		peek = 0;
		lex.token = TOKEN_EOL;
		return TOKEN_EOL;
	case EOF:
		peek = 0;
		lex.token = TOKEN_EOF;
		return TOKEN_EOF;
	}

	peek = 0;
	lex.token = TOKEN_UNKNOWN;
	return TOKEN_UNKNOWN;	
}

