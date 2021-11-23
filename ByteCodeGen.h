#include <iostream>
using namespace std;
enum INSTR_TYPE {
    INSTR_EOF,
    INSTR_MOV,
    INSTR_PRINT_REG,
    INSTR_MOVQ,
    INSTR_ADD,
    INSTR_SUB,
    INSTR_MUL,
    INSTR_EQ,
    INSTR_NEQ,
    INSTR_LE,//TEST!!
    INSTR_GR,//TODO
    INSTR_LEQ,//TODO
    INSTR_GRQ,//TODO
    INSTR_JMP_IF,
    INSTR_JMP,
    INSTR_LOAD32,
    INSTR_STORE32,
    INSTR_MULQ,
    INSTR_ADDQ,
};
enum REG {
    EA0 = 0,
    EA1,
    EA2,
    EA3,
    EA4,
    EA5,
    EA6,
    EA7,
    COND,
};
struct ByteCode{
private:
    char byte, bytex, bytexx;
    FILE* file;
    int id=0;
public:
    void close() {
        if (file) {
            byte=INSTR_EOF;
            fwrite(&byte, 1, 1, file);
            fclose(file);
            file=nullptr;
        }
    }
    void i_movq(char reg, int32_t data) {
        cout << "movq " << int(reg) << ' ' << data << '\n';
        byte=INSTR_MOVQ;
        fwrite(&byte, 1, 1, file);
        fwrite(&reg, 1, 1, file);
        fwrite(&data, 4, 1, file);
        id+=6;
    }
    void i_mulq(char left, int32_t right) {
        cout << "mulq " << int(left) << ' ' << right << '\n';
        byte=INSTR_MULQ;
        fwrite(&byte, 1, 1, file);
        fwrite(&left, 1, 1, file);
        fwrite(&right, 4, 1, file);
        id+=6;
    }
    void i_addq(char left, int32_t right) {
        cout << "addq " << int(left) << ' ' << right << '\n';
        byte=INSTR_ADDQ;
        fwrite(&byte, 1, 1, file);
        fwrite(&left, 1, 1, file);
        fwrite(&right, 4, 1, file);
        id+=6;
    }
    void i_store32(int32_t data, char reg) {
        cout << "store32 " << int(data) << ' ' << int(reg) << '\n';
        byte=INSTR_STORE32;
        fwrite(&byte, 1, 1, file);
        fwrite(&data, 4, 1, file);
        fwrite(&reg, 1, 1, file);
        id+=6;
    }
    ByteCode(const char* file_name) { file=fopen(file_name, "wb");}
    ~ByteCode() {close();}
};
