#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <stdio.h>
#include <unordered_map>
#include <stack>
using namespace std;

enum INSTR_TYPE {
	INSTR_NOP=0,
	INSTR_MOV,
	INSTR_ADD,
	INSTR_EOF,
};
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
struct CodeGen {
private:
	void _write_begin(const string& stack_size) {
		data_seg =
			"Exit:														\n"
			"	invoke  ExitProcess, 0									\n"
			"section '.data' data readable writeable                     \n"
			"	conTitle    db 'Console', 0					\n"
			"	STD_INP_HNDL  dd - 10					\n"
			"	STD_OUTP_HNDL dd - 11					\n"
			"section '.bss' readable writeable		\n"
			"	hStdIn      dd ?						\n"
			"	hStdOut     dd ?						\n"
			"	mem db "+stack_size+" dup(? )						\n";
		end_seg =
			"section '.idata' import data readable	\n"
			"	library kernel, 'KERNEL32.DLL'			\n"
			"											\n"
			"	import kernel, \\						\n"
			"	SetConsoleTitleA, 'SetConsoleTitleA', \\	\n"
			"	GetStdHandle, 'GetStdHandle', \\		\n"
			"	WriteConsoleA, 'WriteConsoleA', \\		\n"
			"	ReadConsoleA, 'ReadConsoleA', \\		\n"
			"	ExitProcess, 'ExitProcess'				\n";
		fprintf(fout, 
			"format PE Console 4.0\n"
			"entry Start\n"
			"include 'win32a.inc'\n"
			"section '.text' code readable executable\n"
			"Start :\n"
			"	invoke SetConsoleTitleA, conTitle								\n"
			"	test eax, eax												\n"
			"	jz Exit														\n"
			"	invoke GetStdHandle, [STD_OUTP_HNDL]						\n"
			"	mov[hStdOut], eax											\n"
			"	invoke GetStdHandle, [STD_INP_HNDL]							\n"
			"	mov[hStdIn], eax											\n");
	}
public:
	string data_seg;
	string end_seg;
	string temp;
	FILE* fout;

	CodeGen(const char* name) { fout = fopen(name, "w"); _write_begin("1000"); }
	void i_mov(const char* left, int32_t right) { fprintf(fout, "	mov % s, % d\n", left, right); }
	void i_mov(const char* left, const char* right) { fprintf(fout, "	mov %s, %s\n", left, right); }

	void i_store32(int32_t off, const char* right) { fprintf(fout, "	mov dword [mem+%d], %s\n", off, right); }

	void i_add(const char* left, int32_t right) { fprintf(fout, "	add %s, %d\n", left, right); }
	void i_add(const char* left, const char* right) { fprintf(fout, "	add %s, %s\n", left, right); }

	void i_mul(const char* left, int32_t right) { fprintf(fout, "	imul %s, %d\n", left, right); }
	void i_mul(const char* left, const char* right) { fprintf(fout, "	imul %s, %s\n", left, right); }

	void i_eq(const char* left, const char* right) {fprintf(fout, "    cmp %s, %s\n", left, right); }
	void i_eq(const char* left, int32_t right) {fprintf(fout, "    cmp %s, %d\n", left, right); }
	void i_if() { fprintf(fout, "    jz label_0000000000\n"); }


	//void i_load32 (char reg, int32_t addr) { fprintf(fout, "	mov %s, [%d]\n", reg, addr); }
	//void i_load32(char reg, char addr) { fprintf(fout, "	mov %s, [%d]\n", reg, addr); }
	int32_t get_ptr() { return ftell(fout); }
	void i_end_if(int32_t from, int32_t to) {
		int now = ftell(fout);
		fseek(fout, from, SEEK_SET);
		temp = to_string(to);
		while (temp.size() < 10) { temp += " "; }
		fprintf(fout, "    jz label_%s\n", temp.c_str());//WRONG
		fseek(fout, now, SEEK_SET);
		fprintf(fout, "label_%d:\n", to);
	}

	void close() { if (fout == nullptr) return; fprintf(fout, "%s\n%s\n", data_seg.c_str(), end_seg.c_str()); fclose(fout); }
	~CodeGen() { close(); }
};
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
		int pos;
		VAR_TYPE type;
		m_Var(int Pos, VAR_TYPE Type = TYPE_NOP) :type(Type), pos(Pos) {}
		m_Var() {}
		//m_Var(const m_Var& other):pos(other.pos), type(other.type) {}
		//void operator=(const m_Var& other) { pos = other.pos; type = other.type; }
	};
	void h_assert (bool cond, Trap err = TRAP_UNKNOWN, const string& log = "") {if (!cond) { throw ParseException(err, log); }};
	void print_vars() {for (auto it : vars) { cout << it.first << '\n'; }};
	void push_expr_to_stack(Token x) {
		f=lex.next_tok();
		if (f.type == TOKEN_INT_NUMBER || f.type == TOKEN_IDENTIFIER) { h_assert(x.type == TOKEN_PLUS || x.type == TOKEN_MUL || x.type == TOKEN_MINUS, TRAP_UNEXPECTED_TOKEN); }
		else if (f.type == TOKEN_PLUS || f.type == TOKEN_MINUS || f.type==TOKEN_MUL) { h_assert(x.type == TOKEN_INT_NUMBER || x.type == TOKEN_IDENTIFIER, TRAP_UNEXPECTED_TOKEN); }
		
		if (f.type == TOKEN_INT_NUMBER) { temp_stack1[temp_stack1c++] = f; push_expr_to_stack(f); }
		else if (f.type == TOKEN_IDENTIFIER) { temp_stack1[temp_stack1c++] = f; push_expr_to_stack(f); }
		else if (f.type == TOKEN_PLUS) { temp_stack2[temp_stack2c++] = "+"; push_expr_to_stack(f); }
		else if (f.type == TOKEN_MUL) { temp_stack2[temp_stack2c++] = "*"; push_expr_to_stack(f); }
		else if (f.type == TOKEN_MINUS) {
			if (x.type == TOKEN_IDENTIFIER || x.type == TOKEN_INT_NUMBER) { temp_stack2[temp_stack2c++] = "+"; }
			int now = temp_stack1c;
			push_expr_to_stack(f);
			temp_stack1[now].data = "-" + temp_stack1[now].data;
		}
		else if (f.type == TOKEN_SEMICOLON) { return; }
		else h_assert(false, TRAP_UNEXPECTED_TOKEN);
	}
	Token f;
	std::vector<Token> temp_stack1;
	std::vector<string> temp_stack2;
	string temps, temp_name, temp_val;
	int tempi = 0;
	int temp_stack1c=0;
	int temp_stack2c=0;
	std::unordered_map<string, m_Var> vars;
	std::unordered_map<string, int> op_order;
	CodeGen code;
public:
	Lexer lex;
	Parser(const char* from, const char* to):code(to), lex(from), temp_stack1(1000), temp_stack2(1000) {
		op_order["+"] = 20;
		op_order["*"] = 30;
	}
	void parse_from_file(ProgramInfo& program) {
		Token x;
		int alloc_count = 0;
		do {
			try {
				x = lex.next_tok();
				switch (x.type) {
				case TOKEN_INT:
					x = lex.next_tok();
					h_assert(x.type == TOKEN_IDENTIFIER, TRAP_UNKNOWN, "TODO!");
					temp_name = x.data;
					x = lex.next_tok();
					//int <identifier>; VARIABLE DECLARATION
					if (x.type == TOKEN_SEMICOLON) {
						h_assert(vars.find(temp_name) == vars.end(), TRAP_UNKNOWN, "not implemented!");
						vars[temp_name] = m_Var(alloc_count, TYPE_INT);
						alloc_count += 4;
						break;
					}
					//int <identifier>=VARIABLE DECLARATION AND ASSIGMENT
					h_assert(x.type == TOKEN_EQ, TRAP_UNKNOWN, "TODO!");//=
					//parse expr
					temp_stack1c = 0; temp_stack2c = 0;
					push_expr_to_stack(Token(TOKEN_PLUS));
					for (int i = 0; i < temp_stack2c; ++i) {
						for (int j = i+1; j < temp_stack2c; ++j) {
							if (op_order[temp_stack2[i]] < op_order[temp_stack2[j]]) {
								swap(temp_stack2[i], temp_stack2[j]);
								swap(temp_stack1[i], temp_stack1[j]);
								swap(temp_stack1[i+1], temp_stack1[j+1]);
							}
						}
					}
					code.i_mov("eax", temp_stack1[0].data.c_str());
					for (int i=0;i<temp_stack2c;++i) {
						if (temp_stack2[i] == "+") code.i_add("eax", temp_stack1[i + 1].data.c_str());
						else if (temp_stack2[i] == "*") code.i_mul("eax", temp_stack1[i + 1].data.c_str());
					}
					h_assert(vars.find(temp_name) == vars.end(), TRAP_UNKNOWN, "not implemented!");
					vars[temp_name] = m_Var(alloc_count, TYPE_INT);
					code.i_store32(alloc_count, "eax");
					alloc_count += 4;
					break;
				case TOKEN_INT_NUMBER:
					break;
				case TOKEN_IDENTIFIER:
					//TODO add identifiers
					temp_name = x.data;
					h_assert(vars.find(temp_name) != vars.end(), TRAP_UNKNOWN, "TODO!");
					x = lex.next_tok();
					h_assert(x.type == TOKEN_EQ, TRAP_UNKNOWN, "not implemented!");
					temp_stack1c = 0; temp_stack2c = 0;
					push_expr_to_stack(Token(TOKEN_PLUS));
					for (int i = 0; i < temp_stack2c; ++i) {
						for (int j = i + 1; j < temp_stack2c; ++j) {
							if (op_order[temp_stack2[i]] < op_order[temp_stack2[j]]) {
								swap(temp_stack2[i], temp_stack2[j]);
								swap(temp_stack1[i], temp_stack1[j]);
								swap(temp_stack1[i + 1], temp_stack1[j + 1]);
							}
						}
					}
					code.i_mov("eax", temp_stack1[0].data.c_str());
					for (int i = 0; i < temp_stack2c; ++i) {
						if (temp_stack2[i] == "+") code.i_add("eax", temp_stack1[i + 1].data.c_str());
						else if (temp_stack2[i] == "*") code.i_mul("eax", temp_stack1[i + 1].data.c_str());
					}
					code.i_store32(vars[temp_name].pos, "eax");
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
	Parser pars("my.code", "any.asm");
	ProgramInfo p; pars.parse_from_file(p);
	if (p.res != TRAP_OK) cout << "Trap " << p.res << "! " << p.errorlog << '\n';
	return 0;
}