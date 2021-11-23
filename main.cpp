#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <stdio.h>
#include <unordered_map>
#include <stack>
#include "ByteCode.h"
using namespace std;

enum Trap {
	TRAP_OK,
	TRAP_UNKNOWN,
	TRAP_UNKNOWN_TOKEN,
	TRAP_UNEXPECTED_TOKEN,
};
struct ProgramInfo {
	Trap res;
	string errorlog;
	string warnings;
};
/*
* var <name>=<number>;
* <function>(arg1, arg2, argn);    function call
* if (<var>==<number>) block end
*/

enum VAR_TYPE {
	TYPE_INT,
	TYPE_NOP,
	TYPE_UNKNOWN,
};
enum TOKEN_TYPE
{
	TOKEN_COLON,
	TOKEN_SEMICOLON,
	TOKEN_UNKNOWN,
	TOKEN_INT_NUMBER,
	TOKEN_IDENTIFIER,
	TOKEN_PLUS,
	TOKEN_MUL,
	TOKEN_MINUS,
	TOKEN_PRINT,
	TOKEN_INT,
	TOKEN_EOF,
	TOKEN_COMMENT,
	TOKEN_IF,
	TOKEN_ELSE,
	TOKEN_EQ,
	TOKEN_RCB,
	TOKEN_LCB
};
struct Token {
	TOKEN_TYPE type;
	string data;
	Token(TOKEN_TYPE Type = TOKEN_UNKNOWN, const string& Data = "") :type(Type), data(Data) {}
	std::string ToString() {
		switch (type) {
		case TOKEN_COLON: return "Token :";
		case TOKEN_SEMICOLON: return "Token ;";
		case TOKEN_UNKNOWN: return "Token?";
		case TOKEN_INT_NUMBER: return "Token integer: ";
		case TOKEN_IDENTIFIER: return "Token identifier: %s";
		case TOKEN_PLUS: return "Token +";
		case TOKEN_MUL: return "Token *";
		case TOKEN_MINUS: return "Token -";
		case TOKEN_INT: return "Token int";
		case TOKEN_EOF: return "Token EOF";
		case TOKEN_COMMENT: return "Token #comment";
		case TOKEN_IF: return "Token if";
		case TOKEN_ELSE: return "Token else";
		case TOKEN_EQ: return "Token =";
		case TOKEN_RCB: return "Token }";
		case TOKEN_LCB: return "Token {";
		}
		return "!!compiler token error";
	}
	~Token() {}
};
class Parser {
	class Lexer {
		char lastchar = ' ';
		std::string tempx;
		FILE* file;
		char __next() { return fgetc(file); }
	public:
		Lexer(const char* file_name) { file = fopen(file_name, "r"); }
		Token next_tok() {
			while (lastchar == ' ' || lastchar == '\n') { lastchar = __next(); }
			if (isalpha(lastchar) || lastchar == '_') { // identifier: [a-zA-Z][a-zA-Z0-9]*
				tempx = "";
				do {
					tempx += lastchar;
					lastchar = __next();
				} while (isalnum(lastchar) || lastchar == '_');
				if (tempx == "if") return Token(TOKEN_IF);
				if (tempx == "else") return Token(TOKEN_ELSE);
				if (tempx == "int") return Token(TOKEN_INT);
				return Token(TOKEN_IDENTIFIER, tempx.c_str());
			}
			if (isdigit(lastchar)) { // Number: [0-9]+
				tempx = "";
				do {
					tempx += lastchar;
					lastchar = __next();
				} while (isdigit(lastchar));
				return Token(TOKEN_INT_NUMBER, tempx.c_str());
			}
			if (lastchar == '#') {
				do
					lastchar = __next();
				while (lastchar != EOF && lastchar != '\n' && lastchar != '\r');

				if (lastchar != EOF) return next_tok();
			}

			// Check for end of file.  Don't eat the EOF.
			if (lastchar == EOF)
				return Token(TOKEN_EOF);

			char ThisChar = lastchar;
			lastchar = __next();
			if (ThisChar == '+') return Token(TOKEN_PLUS);
			if (ThisChar == '-') return Token(TOKEN_MINUS);
			if (ThisChar == '{') return Token(TOKEN_LCB);
			if (ThisChar == '}') return Token(TOKEN_RCB);
			if (ThisChar == '=') return Token(TOKEN_EQ);
			if (ThisChar == ';') return Token(TOKEN_SEMICOLON);
			if (ThisChar == '*') return Token(TOKEN_MUL);
			return Token(TOKEN_UNKNOWN, &ThisChar);
		}
		~Lexer() { fclose(file); }
	};
	struct ParseException {
		string errors;
		Trap res;
		string warnings;
		ParseException(Trap Res = TRAP_OK, const string& log = "") :res(Res), errors(log) {}
		~ParseException() {}
	};
	struct m_Var {
	public:
		string name;
		int pos;
		VAR_TYPE type;
		m_Var(string Name, int Pos, VAR_TYPE Type = TYPE_NOP) :name(Name), type(Type), pos(Pos) {}
		m_Var() {}
		//m_Var(const m_Var& other):pos(other.pos), type(other.type) {}
		//void operator=(const m_Var& other) { pos = other.pos; type = other.type; }
	};
	void h_assert(bool cond, Trap err = TRAP_UNKNOWN, const string& log = "") { if (!cond) { throw ParseException(err, log); } };
	Token f, x;
	string temps, temp_name, temp_val;
	int tempi = 0;
	int alloc_count = 0;
	std::vector<m_Var> vars;
	std::vector<int32_t> locals;
	ByteCode code;
public:
	Lexer lex;
	Parser(const char* from, const char* to) :code(to), lex(from) { locals.push_back(0); }
	void parse_from_file(ProgramInfo& program) {
	    bool m_running=false;
		//cur.type = VAR_TYPE::TYPE_NOP;
		do {
			try {
				x = lex.next_tok();
				switch (x.type) {
				case TOKEN_LCB:
					locals.push_back(0);
					break;
				case TOKEN_RCB:
					tempi = locals[locals.size() - 1];
					locals.pop_back();
					for (int i = 0; i < tempi; ++i) {
						alloc_count -= 4;
						vars.pop_back();
					}
					break;
				case TOKEN_INT:
					x = lex.next_tok();
					h_assert(x.type == TOKEN_IDENTIFIER);
					temp_name = x.data;
					x = lex.next_tok();
					if (x.type == TOKEN_SEMICOLON) {
						for (int i = 0; i < vars.size(); ++i) { h_assert(vars[i].name != temp_name); }
						locals[locals.size() - 1]++;
						vars.push_back(m_Var(temp_name, alloc_count, VAR_TYPE::TYPE_INT));
						alloc_count += 4;
						break;
					}
					h_assert(x.type == TOKEN_EQ);
					f = lex.next_tok();
					h_assert(f.type == TOKEN_INT_NUMBER);
					code.i_movq(EA0, atoi(f.data.c_str()));
					m_running=true;
                    while (m_running) {
                        x = lex.next_tok();
                        switch (x.type) {
                        case TOKEN_INT_NUMBER:
                            if (f.type==TOKEN_MUL) code.i_mulq(EA0, atoi(x.data.c_str()));
                            else if (f.type==TOKEN_PLUS) code.i_addq(EA0, atoi(x.data.c_str()));
                            else h_assert(false);
                            break;
                        case TOKEN_IDENTIFIER:
                            h_assert(false);
                        case TOKEN_MINUS:
                            break;
                        case TOKEN_PLUS:
                            break;
                        case TOKEN_MUL:
                            break;
                        case TOKEN_SEMICOLON:
                            m_running=false;
                            break;
                        default:
                            h_assert(false);
                        }
                        f=x;
                    }
					for (int i = 0; i < vars.size(); ++i) { h_assert(vars[i].name != temp_name); }
					locals[locals.size() - 1]++;
					vars.push_back(m_Var(temp_name, alloc_count, VAR_TYPE::TYPE_INT));

					//code.i_movq(EA0, atoi(temp_val.c_str()));
					code.i_store32(alloc_count, EA0);
					alloc_count += 4;
					break;
				case TOKEN_IDENTIFIER:
					temp_name = x.data;
					tempi = -1;
					for (int i = 0; i < vars.size(); ++i) { if (vars[i].name == temp_name) { tempi = i; } }
					h_assert(tempi != -1);
					x = lex.next_tok();
					h_assert(x.type == TOKEN_EQ);
					x = lex.next_tok();
					h_assert(x.type == TOKEN_INT_NUMBER);
					x = lex.next_tok();
					h_assert(x.type == TOKEN_SEMICOLON);
					code.i_movq(EA0, atoi(x.data.c_str()));
					code.i_store32(vars[tempi].pos, EA0);
					break;
				case TOKEN_EOF:
					h_assert(false, TRAP_OK, "0 errors");
					break;
				case TOKEN_COMMENT:
					break;
				default:
					h_assert(false, TRAP_UNKNOWN_TOKEN, "lexer: unknown " + x.ToString());
					break;
				}
			}
			catch (ParseException x) {
				program.errorlog = x.errors;
				program.res = x.res;
				return;
			}
		} while (true);
	}
};
int main()
{
	Parser pars("my.code", "any.sb");
	ProgramInfo p; pars.parse_from_file(p);
	if (p.res != TRAP_OK) cout << "Trap " << p.res << "! " << p.errorlog << '\n';
	else cout << "OK";
	return 0;
}
