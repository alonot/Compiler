
/******************Compile*********************** */

/**
 * In case of ste its responsibility of handler to give the address of ste via a register(it may be fp, sp also)
 */
typedef struct __reg_promise RegPromise;

/**Register Table 
 * States:
 * occupied + ste
 * occupied + offset + ste
 * occupied + neg_expr
*/
typedef struct __reg {
    char* name;
    lli offset; // set to NO_ENTRY if value is directly in the register, else give offset to actual address. 
    int occupied; // whether occupied or not. 
    RegPromise* reg_promise;
} Register;

/**
 * Essential Idea is that when evaluating an expression we may not be
 * confirm whether this is actually the instructions we want to 
 * write hence stack.
 * An expressions adds one more String* to the stack and send to 
 * left child. Then it adds one more and sends to right child.
 * now if required
 * If every function ensure that the stack will contain no extra elements
 */
typedef struct __instr {
    Stack* instructions_stack; 
    Register* extra_regs[32];
    int extra_reg_count;
    int curr_size;
    int gen_code;
    int call_window_size; // store the max size of window required for a function call
    String* return_label; 
} Instructions_info;

typedef struct STEntry STEntry;


typedef struct __reg_promise {
    Register* reg;
    lli* immediate;
    int offset; // if actual register had a register then its value will be stored in this in case of spill
    int loc; // is NO_ENTRY, else location from sp (if reg is NULL)
    STEntry* ste; // if set to NULL then this is result of an expression and should be saved on the stack
    String* label; // label value
} RegPromise;

int compile(Node* node);

RegPromise* get_ste(Node* node, SymbolTable* symt, Instructions_info* instructions_info);

RegPromise* get_expression(Node* node, SymbolTable* symt, Instructions_info* instructions_info) ;

RegPromise* get_free_register_promise(Instructions_info* instructions_info);

RegPromise* init_reg_promise(Register* reg);


/************************************************ */
#define instr_out(fmt, ...) fprintf(yyout, "\t" fmt "\n", ##__VA_ARGS__)

#define instr_out_no_tab(fmt, ...) fprintf(yyout, fmt "\n", ##__VA_ARGS__)

#define NO_ENTRY -1

#define MAX_32_BIT 32767

#define DEF32 0xFFFF

#define max(a,b) (a) > (b) ? (a) : (b);

#ifndef __DECLARATION__
#define __DECLARATION__
extern FILE* yyout;
extern FILE* debug;

extern Register* zero_reg, * sp_reg, * fp_reg;
extern RegPromise* zero_reg_promise, * sp_reg_promise, * fp_reg_promise;
extern Queue* register_usage_queue;

#endif


void __loadw(Instructions_info* instr_info,Register* rd, Register* rs1) ;

void __storew(Instructions_info* instr_info,Register* rs1, Register* rs2) ;

RegPromise* get_immediate(Instructions_info* instr_info, lli immediate) ;

void addiu(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, lli immediate) ;

void sltiu(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, lli immediate) ;

void slti(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, lli immediate) ;

void xori(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, lli immediate) ;

void andi(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, lli immediate) ;

void ori(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, lli immediate) ;

void sllv(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, RegPromise* rs2);

void subiu(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, lli immediate) ;

void addu(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, RegPromise* rs2) ;

void and(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, RegPromise* rs2) ;

void or(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, RegPromise* rs2) ;

void xor(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, RegPromise* rs2) ;

void subu(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, RegPromise* rs2) ;

void slt(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, RegPromise* rs2) ;

void sltu(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, RegPromise* rs2);

void nor(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, RegPromise* rs2);

void mult(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1) ;

void mflo(Instructions_info* instr_info,RegPromise* rd) ;

void mfhi(Instructions_info* instr_info,RegPromise* rd) ;

void mips_div(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1) ;

void loadw(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1) ;

void move(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1) ;

void storew(Instructions_info* instr_info,RegPromise* rs1, RegPromise* rs2) ;

void sll(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, lli immediate) ;

void reload_reg(RegPromise* reg_promise, Instructions_info* instr_info);


/************ */


void assign_reg_promise(RegPromise* p1, RegPromise* p2);

void __free_regpromise(RegPromise* reg_promise);

void write_instr(Instructions_info* instr_info, const char* format, ...);

void save_n_free(Register* reg, Instructions_info* instructions_info);

void write_instr_val(String* instr_val, int tab, const char* format, ...);