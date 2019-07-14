/**
 * ics-lab5
 * yuyajie
 * 517021910851
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "y64asm.h"

line_t *line_head = NULL;
line_t *line_tail = NULL;
int lineno = 0;

#define err_print(_s, _a ...) do { \
  if (lineno < 0) \
    fprintf(stderr, "[--]: "_s"\n", ## _a); \
  else \
    fprintf(stderr, "[L%d]: "_s"\n", lineno, ## _a); \
} while (0);


int64_t vmaddr = 0;    /* vm addr */

/* register table */
const reg_t reg_table[REG_NONE] = {
    {"%rax", REG_RAX, 4},
    {"%rcx", REG_RCX, 4},
    {"%rdx", REG_RDX, 4},
    {"%rbx", REG_RBX, 4},
    {"%rsp", REG_RSP, 4},
    {"%rbp", REG_RBP, 4},
    {"%rsi", REG_RSI, 4},
    {"%rdi", REG_RDI, 4},
    {"%r8",  REG_R8,  3},
    {"%r9",  REG_R9,  3},
    {"%r10", REG_R10, 4},
    {"%r11", REG_R11, 4},
    {"%r12", REG_R12, 4},
    {"%r13", REG_R13, 4},
    {"%r14", REG_R14, 4}
};
const reg_t* find_register(char *name)
{
    int i;
    for (i = 0; i < REG_NONE; i++)
        if (!strncmp(name, reg_table[i].name, reg_table[i].namelen))
            return &reg_table[i];
    return NULL;
}


/* instruction set */
instr_t instr_set[] = {
    {"nop", 3,   HPACK(I_NOP, F_NONE), 1 },
    {"halt", 4,  HPACK(I_HALT, F_NONE), 1 },
    {"rrmovq", 6,HPACK(I_RRMOVQ, F_NONE), 2 },
    {"cmovle", 6,HPACK(I_RRMOVQ, C_LE), 2 },
    {"cmovl", 5, HPACK(I_RRMOVQ, C_L), 2 },
    {"cmove", 5, HPACK(I_RRMOVQ, C_E), 2 },
    {"cmovne", 6,HPACK(I_RRMOVQ, C_NE), 2 },
    {"cmovge", 6,HPACK(I_RRMOVQ, C_GE), 2 },
    {"cmovg", 5, HPACK(I_RRMOVQ, C_G), 2 },
    {"irmovq", 6,HPACK(I_IRMOVQ, F_NONE), 10 },
    {"rmmovq", 6,HPACK(I_RMMOVQ, F_NONE), 10 },
    {"mrmovq", 6,HPACK(I_MRMOVQ, F_NONE), 10 },
    {"addq", 4,  HPACK(I_ALU, A_ADD), 2 },
    {"subq", 4,  HPACK(I_ALU, A_SUB), 2 },
    {"andq", 4,  HPACK(I_ALU, A_AND), 2 },
    {"xorq", 4,  HPACK(I_ALU, A_XOR), 2 },
    {"jmp", 3,   HPACK(I_JMP, C_YES), 9 },
    {"jle", 3,   HPACK(I_JMP, C_LE), 9 },
    {"jl", 2,    HPACK(I_JMP, C_L), 9 },
    {"je", 2,    HPACK(I_JMP, C_E), 9 },
    {"jne", 3,   HPACK(I_JMP, C_NE), 9 },
    {"jge", 3,   HPACK(I_JMP, C_GE), 9 },
    {"jg", 2,    HPACK(I_JMP, C_G), 9 },
    {"call", 4,  HPACK(I_CALL, F_NONE), 9 },
    {"ret", 3,   HPACK(I_RET, F_NONE), 1 },
    {"pushq", 5, HPACK(I_PUSHQ, F_NONE), 2 },
    {"popq", 4,  HPACK(I_POPQ, F_NONE),  2 },

    {".byte", 5, HPACK(I_DIRECTIVE, D_DATA), 1 },
    {".word", 5, HPACK(I_DIRECTIVE, D_DATA), 2 },
    {".long", 5, HPACK(I_DIRECTIVE, D_DATA), 4 },
    {".quad", 5, HPACK(I_DIRECTIVE, D_DATA), 8 },
    {".pos", 4,  HPACK(I_DIRECTIVE, D_POS), 0 },
    {".align", 6,HPACK(I_DIRECTIVE, D_ALIGN), 0 },
    {NULL, 1,    0   , 0 } //end
};

instr_t *find_instr(char *name)
{
    int i;
    for (i = 0; instr_set[i].name; i++)
	if (strncmp(instr_set[i].name, name, instr_set[i].len) == 0)
	    return &instr_set[i];
    return NULL;
}

/* symbol table (don't forget to init and finit it) */
symbol_t *symtab = NULL;
/*
 * find_symbol: scan table to find the symbol
 * args
 *     name: the name of symbol
 *
 * return
 *     symbol_t: the 'name' symbol
 *     NULL: not exist
 */
symbol_t *find_symbol(char *name)
{
    for (symbol_t *sym = symtab->next;sym&&sym->name; sym = sym->next)
    {   
	    if (strncmp(sym->name, name, strlen(sym->name)) == 0)
	        return sym;
    }
    return NULL;
}

/*
 * add_symbol: add a new symbol to the symbol table
 * args
 *     name: the name of symbol
 *
 * return
 *     0: success
 *     -1: error, the symbol has exist
 */
int add_symbol(char *name)
{
    /* check duplicate */
    if (find_symbol(name)) {
        err_print("Dup symbol:%s", name);
        return -1;
    }
    symbol_t *sym = symtab;
    symbol_t *symbol = malloc(sizeof(symbol_t));
    memset(symbol, 0, sizeof(symbol_t));
    symbol->addr = vmaddr;
    symbol->name = name;
    symbol->next = NULL;
    while (sym->next){sym=sym->next;}
    sym->next = symbol;
    return 0;
}

/* relocation table (don't forget to init and finit it) */
reloc_t *reltab = NULL;

/* macro for parsing y64 assembly code */
#define IS_DIGIT(s) ((*(s)>='0' && *(s)<='9') || *(s)=='-' || *(s)=='+')
#define IS_LETTER(s) ((*(s)>='a' && *(s)<='z') || (*(s)>='A' && *(s)<='Z'))
#define IS_COMMENT(s) (*(s)=='#')
#define IS_REG(s) (*(s)=='%')
#define IS_IMM(s) (*(s)=='$')

#define IS_BLANK(s) (*(s)==' ' || *(s)=='\t')
#define IS_END(s) (*(s)=='\0')

#define SKIP_BLANK(s) do {  \
  while(!IS_END(s) && IS_BLANK(s))  \
    (s)++;    \
} while(0);

/* return value from different parse_xxx function */
typedef enum { PARSE_ERR=-1, PARSE_REG, PARSE_DIGIT, PARSE_SYMBOL, 
    PARSE_MEM, PARSE_DELIM, PARSE_INSTR, PARSE_LABEL} parse_t;
/*
 * parse_delim: parse an expected delimiter token (e.g., ',')
 * args
 *     ptr: point to the start of string
 *
 * return
 *     PARSE_DELIM: success, move 'ptr' to the first char after token
 *     PARSE_ERR: error, the value of 'ptr' and 'delim' are undefined
 */
parse_t parse_delim(char **ptr, char delim)
{
    /* skip the blank and check */
    SKIP_BLANK(*ptr);
    if (*ptr == NULL||IS_BLANK(*ptr) || IS_END(*ptr) ||**ptr != delim)
    {
        err_print("Invalid '%c'", delim);
        return PARSE_ERR;
    } 
    /* set 'ptr' */
    (*ptr)++;
    
    return PARSE_DELIM;
}
/*
 * parse_instr: parse an expected data token (e.g., 'rrmovq')
 * args
 *     ptr: point to the start of string
 *     inst: point to the inst_t within instr_set
 *
 * return
 *     PARSE_INSTR: success, move 'ptr' to the first char after token,
 *                            and store the pointer of the instruction to 'inst'
 *     PARSE_ERR: error, the value of 'ptr' and 'inst' are undefined
 */
parse_t parse_instr(char **ptr, instr_t *instr, int64_t *imm, char *symbol_name, bin_t* y64bin)
{
    /* skip the blank */
    SKIP_BLANK(*ptr);
    if (*ptr == NULL||IS_BLANK(*ptr) || IS_END(*ptr) || !IS_LETTER(*ptr)) return PARSE_ERR;
    /* find_instr and check end */
    sscanf(*ptr, "%s", instr->name);
    *ptr += strlen(instr->name);
    if ((instr = find_instr(instr->name)) == NULL) return PARSE_ERR;
    /* set 'ptr' and 'inst' */
    regid_t *ra = malloc(sizeof(regid_t));
    regid_t *rb = malloc(sizeof(regid_t));
    y64bin->addr = vmaddr;
    y64bin->codes[0] = instr->code;
    switch (instr->code)
    {
        case HPACK(I_NOP, F_NONE):
        case HPACK(I_HALT, F_NONE):
        case HPACK(I_RET, F_NONE):
            y64bin->bytes = 1;
            break;
        case HPACK(I_RRMOVQ, F_NONE):
        case HPACK(I_RRMOVQ, C_LE):
        case HPACK(I_RRMOVQ, C_L):
        case HPACK(I_RRMOVQ, C_E):
        case HPACK(I_RRMOVQ, C_NE):
        case HPACK(I_RRMOVQ, C_GE):
        case HPACK(I_RRMOVQ, C_G):
        case HPACK(I_ALU, A_ADD):
        case HPACK(I_ALU, A_SUB):
        case HPACK(I_ALU, A_AND):
        case HPACK(I_ALU, A_XOR):            
            y64bin->bytes = 2;
            SKIP_BLANK(*ptr);
            if (*ptr == NULL||IS_BLANK(*ptr) || IS_END(*ptr))
             return PARSE_ERR;
            
            if (parse_reg(ptr, ra) == PARSE_ERR ||
            parse_delim(ptr, ',') == PARSE_ERR|| 
            parse_reg(ptr, rb) == PARSE_ERR) return PARSE_ERR;
           
            y64bin->codes[1] = HPACK(*ra, *rb);
            break;
        case HPACK(I_PUSHQ, F_NONE):
        case HPACK(I_POPQ, F_NONE):
            y64bin->bytes = 2;
            SKIP_BLANK(*ptr);
            if (*ptr == NULL||IS_BLANK(*ptr) || IS_END(*ptr))
             return PARSE_ERR;
            if (parse_reg(ptr, ra) == PARSE_ERR) return PARSE_ERR;
            y64bin->codes[1] = HPACK(*ra, REG_NONE);
            break;
        case HPACK(I_JMP, C_YES):
        case HPACK(I_JMP, C_LE):
        case HPACK(I_JMP, C_L):
        case HPACK(I_JMP, C_E):
        case HPACK(I_JMP, C_NE):
        case HPACK(I_JMP, C_GE):
        case HPACK(I_JMP, C_G):
        case HPACK(I_CALL, F_NONE):
            y64bin->bytes = 9;
            SKIP_BLANK(*ptr);
            if (*ptr == NULL||IS_BLANK(*ptr) || IS_END(*ptr)) return PARSE_ERR;
            if (!IS_LETTER(*ptr)&&**ptr!='0') {
                err_print("Invalid DEST");
                return PARSE_ERR;
            }
            if (parse_data(ptr, symbol_name, imm) == PARSE_ERR)
            return PARSE_ERR;
            break;
        case HPACK(I_IRMOVQ, F_NONE):
            y64bin->bytes = 10;
            SKIP_BLANK(*ptr);
            if (*ptr == NULL||IS_BLANK(*ptr) || IS_END(*ptr)) return PARSE_ERR;
            if (parse_imm(ptr, symbol_name, imm) == PARSE_ERR ||
            parse_delim(ptr, ',') == PARSE_ERR||
            parse_reg(ptr, rb) == PARSE_ERR) return PARSE_ERR;
             
            y64bin->codes[1] = HPACK(REG_NONE, *rb);
            break;
        case HPACK(I_RMMOVQ, F_NONE):
            y64bin->bytes = 10;
            SKIP_BLANK(*ptr);
            if (*ptr == NULL||IS_BLANK(*ptr) || IS_END(*ptr))
             return PARSE_ERR;
            if (parse_reg(ptr, ra) == PARSE_ERR ||
            parse_delim(ptr, ',') == PARSE_ERR||
            parse_mem(ptr, imm, rb) == PARSE_ERR) return PARSE_ERR;
            y64bin->codes[1] = HPACK(*ra, *rb);
            break;
        case HPACK(I_MRMOVQ, F_NONE):
            y64bin->bytes = 10;
            SKIP_BLANK(*ptr);
            if (*ptr == NULL||IS_BLANK(*ptr) || IS_END(*ptr)) return PARSE_ERR;
            if (parse_mem(ptr, imm, ra) == PARSE_ERR ||
            parse_delim(ptr, ',') == PARSE_ERR||
            parse_reg(ptr, rb) == PARSE_ERR) return PARSE_ERR;
            
            y64bin->codes[1] = HPACK(*rb, *ra);
            break;            
    }
    return PARSE_INSTR;
}



/*
 * parse_reg: parse an expected register token (e.g., '%rax')
 * args
 *     ptr: point to the start of string
 *     regid: point to the regid of register
 *
 * return
 *     PARSE_REG: success, move 'ptr' to the first char after token, 
 *                         and store the regid to 'regid'
 *     PARSE_ERR: error, the value of 'ptr' and 'regid' are undefined
 */
parse_t parse_reg(char **ptr, regid_t *regid)
{
    /* skip the blank and check */
    SKIP_BLANK(*ptr);
    if (*ptr == NULL||IS_BLANK(*ptr) || IS_END(*ptr) || **ptr != '%') return PARSE_ERR;
    /* find register */
    char* name = malloc(sizeof(char));
    sscanf(*ptr, "%s", name);
    if(strchr(name, ',')!=NULL)
    *strchr(name, ',')='\0';
    if(strchr(name, ' ')!=NULL)
    *strchr(name, ' ')='\0';
    if(strchr(name, ')')!=NULL)
    *strchr(name, ')')='\0';
    while (!IS_DIGIT(name+strlen(name)-1)
    &&!IS_LETTER(name+strlen(name)-1))
    name[strlen(name)-1] = '\0';
    *ptr += strlen(name);
    
    const reg_t *rt = find_register(name);
    
    if (rt == NULL) {
        err_print("Invalid REG");
        return PARSE_ERR;
    }
    else *regid = rt->id;
    /* set 'ptr' and 'regid' */
    return PARSE_REG;
}

/*
 * parse_symbol: parse an expected symbol token (e.g., 'Main')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *
 * return
 *     PARSE_SYMBOL: success, move 'ptr' to the first char after token,
 *                               and allocate and store name to 'name'
 *     PARSE_ERR: error, the value of 'ptr' and 'name' are undefined
 */
parse_t parse_symbol(char **ptr, char **name)
{
    /* set 'ptr' and 'name' */
    /* skip the blank and check */
    SKIP_BLANK(*ptr);
    if (*ptr == NULL||IS_BLANK(*ptr) ||
     IS_END(*ptr)||!IS_LETTER(*ptr)) return PARSE_ERR;
    sscanf(*ptr, "%s", *name);
        if(strchr(*name, ',')!=NULL)
    *strchr(*name, ',')='\0';
    if(strchr(*name, ' ')!=NULL)
    *strchr(*name, ' ')='\0';
    while (!IS_LETTER(*name+strlen(*name)-1)&&!IS_DIGIT(*name+strlen(*name)-1))
        (*name)[strlen(*name)-1] = '\0';
    *ptr += strlen(*name);
    return PARSE_SYMBOL;
}

/*
 * parse_digit: parse an expected digit token (e.g., '0x100')
 * args
 *     ptr: point to the start of string
 *     value: point to the value of digit
 *
 * return
 *     PARSE_DIGIT: success, move 'ptr' to the first char after token
 *                            and store the value of digit to 'value'
 *     PARSE_ERR: error, the value of 'ptr' and 'value' are undefined
 */
parse_t parse_digit(char **ptr, int64_t *value)
{
    /* skip the blank and check */
    SKIP_BLANK(*ptr);
    if (*ptr == NULL||IS_BLANK(*ptr) || IS_END(*ptr)) 
        return PARSE_ERR;
    if (!IS_DIGIT(*ptr)) return PARSE_ERR;
    /* calculate the digit, (NOTE: see strtoll()) */
    char **endptr = malloc(sizeof(char*));
    if ((**ptr=='0'&&*(*ptr+1)=='x')||(**ptr=='-'&&*(*ptr+1)=='0'))
    {
        strtoll(*ptr, endptr, 16);
        sscanf(*ptr,"%lx",value);
    }
    else *value = strtoll(*ptr, endptr, 10);
    *ptr = *endptr;
    /* set 'ptr' and 'value' */
    return PARSE_DIGIT;

}

/*
 * parse_imm: parse an expected immediate token (e.g., '$0x100' or 'STACK')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *     value: point to the value of digit
 *
 * return
 *     PARSE_DIGIT: success, the immediate token is a digit,
 *                            move 'ptr' to the first char after token,
 *                            and store the value of digit to 'value'
 *     PARSE_SYMBOL: success, the immediate token is a symbol,
 *                            move 'ptr' to the first char after token,
 *                            and allocate and store name to 'name' 
 *     PARSE_ERR: error, the value of 'ptr', 'name' and 'value' are undefined
 */
parse_t parse_imm(char **ptr, char *name, int64_t *value)
{   
    /* skip the blank and check */
    SKIP_BLANK(*ptr);
    /* if IS_DIGIT, then parse the digit */
    if (*ptr == NULL||IS_BLANK(*ptr) || IS_END(*ptr)) 
    {
        err_print("Invalid Immediate");
        return PARSE_ERR;
    }
    if (**ptr == '$') {
        (*ptr)++;
        if (parse_digit(ptr, value) == PARSE_ERR) 
        {
            err_print("Invalid Immediate");
            return PARSE_ERR;
        }
        return PARSE_DIGIT; 
    }/* if IS_LETTER, then parse the symbol */
    else if (IS_LETTER(*ptr)) {
        if (parse_symbol(ptr, &name) == PARSE_ERR)
         return PARSE_ERR;
        return PARSE_SYMBOL; 
    }
    else
    {
        err_print("Invalid Immediate");
        return PARSE_ERR;
    }
}

/*
 * parse_mem: parse an expected memory token (e.g., '8(%rbp)')
 * args
 *     ptr: point to the start of string
 *     value: point to the value of digit
 *     regid: point to the regid of register
 *
 * return
 *     PARSE_MEM: success, move 'ptr' to the first char after token,
 *                          and store the value of digit to 'value',
 *                          and store the regid to 'regid'
 *     PARSE_ERR: error, the value of 'ptr', 'value' and 'regid' are undefined
 */
parse_t parse_mem(char **ptr, int64_t*value, regid_t *regid)
{
    /* skip the blank and check */
    SKIP_BLANK(*ptr);
    if (*ptr == NULL||IS_BLANK(*ptr) || IS_END(*ptr)) return PARSE_ERR;
    /* calculate the digit and register, (ex: (%rbp) or 8(%rbp)) */
    if (**ptr == '(') {
        (*ptr)++;
        *value = 0;
    } else if (IS_DIGIT(*ptr)||**ptr=='-') {
       
        parse_digit(ptr, value);
        if (**ptr != '(') {
            return PARSE_ERR;
            err_print("Invalid MEM");
        }
        (*ptr)++;
    } else
    /* set 'ptr', 'value' and 'regid' */
    return PARSE_ERR;
    if (parse_reg(ptr, regid) == PARSE_ERR || **ptr != ')') 
    {
        err_print("Invalid MEM");
        return PARSE_ERR;
    }
    *ptr += 1;
    return PARSE_MEM;
}
/*
 * ta: parse an expected data token (e.g., '0x100' or 'array')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *     value: point to the value of digit
 *
 * return
 *     PARSE_DIGIT: success, data token is a digit,
 *                            and move 'ptr' to the first char after token,
 *                            and store the value of digit to 'value'
 *     PARSE_SYMBOL: success, data token is a symbol,
 *                            and move 'ptr' to the first char after token,
 *                            and allocate and store name to 'name' 
 *     PARSE_ERR: error, the value of 'ptr', 'name' and 'value' are undefined
 */
parse_t parse_data(char **ptr, char *name, int64_t *value)
{
    /* skip the blank and check */
    
    SKIP_BLANK(*ptr);
   
    /* if IS_DIGIT, then parse the digit */
    if (*ptr == NULL||IS_BLANK(*ptr) || IS_END(*ptr))
         return PARSE_ERR;
    if (IS_DIGIT(*ptr)) {
        if (parse_digit(ptr, value) == PARSE_ERR) 
        return PARSE_ERR;
        return PARSE_DIGIT; 
    }/* if IS_LETTER, then parse the symbol */
    else if (IS_LETTER(*ptr)) {
        if (parse_symbol(ptr, &name) == PARSE_ERR)
         return PARSE_ERR;
        return PARSE_SYMBOL; 
    }
    else
    return PARSE_ERR;
}


parse_t parse_label(char **ptr, label_t *label)
{
    /* skip the blank and check */
    SKIP_BLANK(*ptr);
    label->y64bin.addr=vmaddr;
    if (*ptr == NULL||IS_BLANK(*ptr) || IS_END(*ptr)) return PARSE_ERR;
    sscanf(*ptr, ".%s", label->name);
    *ptr += 1 + strlen(label->name);
    
    if (!strcmp(label->name, "pos") && !strcmp(label->name, "align")
    && !strcmp(label->name, "byte")&& !strcmp(label->name, "word")
    && !strcmp(label->name, "long") && !strcmp(label->name, "quad"))
        return PARSE_ERR;
    SKIP_BLANK(*ptr);
    if (*ptr == NULL||IS_BLANK(*ptr) || IS_END(*ptr))
    return PARSE_ERR;
    char *symbol_name=malloc(sizeof(char));
    int64_t a;
    
    if (parse_data(ptr, symbol_name, &a) == PARSE_ERR) 
        return PARSE_ERR;
    
    label->data= a;
    label->symbol_name=symbol_name;
        if (!strcmp(label->name, "pos")) {
            vmaddr = label->data;
            label->y64bin.addr=vmaddr;
        }
        if (!strcmp(label->name, "align")) {
            if ((a = vmaddr % label->data) != 0) {
                label->bytes= label->data - a;
            } else label->bytes=0;
            
        }
        if (!strcmp(label->name, "byte")) {
            label->bytes = 1;
            label->y64bin.bytes = 1;
        }
       if (!strcmp(label->name, "word")) {
            label->bytes=2;
            label->y64bin.bytes = 2;
        }
        if (!strcmp(label->name, "long")) {
            label->bytes=4;
            label->y64bin.bytes = 4;
        }
        if (!strcmp(label->name, "quad")) {
            label->bytes= 8;
            label->y64bin.bytes = 8;
        }
        return PARSE_LABEL;
}

/*
 * parse_line: parse a line of y64 code (e.g., 'Loop: mrmovq (%rcx), %rsi')
 * (you could combine above parse_xxx functions to do it)
 * args
 *     line: point to a line_t data with a line of y64 assembly code
 *
 * return
 *     PARSE_XXX: success, fill line_t with assembled y64 code
 *     PARSE_ERR: error, try to print err information (e.g., instr type and line number)
 */

type_t parse_line(line_t *line)
{
/* when finish parse an instruction or lable, we still need to continue check 
* e.g., 
*  Loop: mrmovl (%rbp), %rcx
*           call SUM  #invoke SUM function */
    label_t* label = (label_t *)malloc(sizeof(label_t));
    memset(label, 0, sizeof(label_t));
    label->name=(char *)malloc(sizeof(char));
    label->symbol_name=(char *)malloc(sizeof(char));
    instr_t *instr;
    instr = (instr_t *)malloc(sizeof(instr_t));
    memset(instr, 0, sizeof(instr_t));
    instr->name=(char *)malloc(sizeof(char));
    /* skip blank and check IS_END */
    SKIP_BLANK(line->y64asm);
    if IS_END(line->y64asm) 
        return TYPE_COMM;
    /* is a comment ? */
    if IS_COMMENT(line->y64asm)
        return TYPE_COMM;
            if (IS_LETTER(line->y64asm)) 
        {
            char **name=malloc(sizeof(char*));
            *name=malloc(sizeof(char));
            
            if (strchr(line->y64asm, ':')) {
                if (parse_symbol(&(line->y64asm), name) == PARSE_ERR)
                   {
                       
                        line->type = TYPE_ERR;
                        return line->type;
                   }
                else {
                        if (*(line->y64asm)!=':') 
                        return TYPE_ERR;
                        line->y64asm++;
                        if (add_symbol(*name) == -1)
                        return TYPE_ERR;
                
                    line->type = TYPE_COMM;
 
                }
            }
        }
    else if (*(line->y64asm) != '.'){
         line->type = TYPE_ERR;
         return line->type;
    }
    /* is a label ? */
    SKIP_BLANK(line->y64asm);
    if (*(line->y64asm) == '.')
        {
            if (parse_label(&(line->y64asm), label) == PARSE_ERR)
                {
                    line->type = TYPE_ERR;
                    return line->type;
                }
            else {
                SKIP_BLANK(line->y64asm);
                if (!IS_COMMENT(line->y64asm)&&
                !IS_END(line->y64asm)&&!IS_BLANK(line->y64asm))
                    {                  
                        line->type = TYPE_ERR;
                        return line->type;
                    }
                line->type = TYPE_LABEL;
                line->imm = label->data;
                line->symbol_name = label->symbol_name;
                vmaddr += label->bytes;
                line->y64bin = label->y64bin;
                return line->type;
            }
        }

    /* is a symbol ? */
    SKIP_BLANK(line->y64asm);
    if (IS_COMMENT(line->y64asm)||
                IS_END(line->y64asm)||IS_BLANK(line->y64asm))
                {
                        return line->type;
                }
    int64_t imm;
    char *symbol_name=malloc(sizeof(char));
    bin_t* y64bin = (bin_t *)malloc(sizeof(bin_t));
    memset(y64bin, 0, sizeof(bin_t));
    y64bin->addr=vmaddr;
    if (parse_instr(&(line->y64asm), instr, &imm, symbol_name, y64bin) == PARSE_ERR)
        {  
           line->type = TYPE_ERR;
           return line->type;
        }
    else
    {
        line->symbol_name = symbol_name;
        line->y64bin = *y64bin;
        line->imm = imm;
        line->type = TYPE_INS;
        vmaddr += y64bin->bytes;
    }
    return line->type;
}

/*
 * assemble: assemble an y64 file (e.g., 'asum.ys')
 * args
 *     in: point to input file (an y64 assembly file)
 *
 * return
 *     0: success, assmble the y64 file to a list of line_t
 *     -1: error, try to print err information (e.g., instr type and line number)
 */
int assemble(FILE *in)
{
    static char asm_buf[MAX_INSLEN]; /* the current line of asm code */
    line_t *line;
    int slen;
    char *y64asm;
    /* read y64 code line-by-line, and parse them to generate raw y64 binary code list */
    while (fgets(asm_buf, MAX_INSLEN, in) != NULL) {
        slen  = strlen(asm_buf);
        while ((asm_buf[slen-1] == '\n') || (asm_buf[slen-1] == '\r')) { 
            asm_buf[--slen] = '\0'; /* replace terminator */
        }

        /* store y64 assembly code */
        y64asm = (char *)malloc(sizeof(char) * (slen + 1)); // free in finit
        strcpy(y64asm, asm_buf);

        line = (line_t *)malloc(sizeof(line_t)); // free in finit
        memset(line, '\0', sizeof(line_t));

        line->type = TYPE_COMM;
        line->y64asm = y64asm;
        line->next = NULL;
        lineno ++;
        if (parse_line(line) == TYPE_ERR) {
            return -1;
        }
        line_tail->next = line;
        line_tail = line;
        
    }
    lineno = -1;
    return 0;
}

/*
 * relocate: relocate the raw y64 binary code with symbol address
 *
 * return
 *     0: success
 *     -1: error, try to print err information (e.g., addr and symbol)
 */
bin_t wow[MAX_INSLEN];
int nwow = 0;
int relocate(void)
{
    symbol_t *s = malloc(sizeof(symbol_t));
    memset(s,0,sizeof(symbol_t));
    line_t * line = line_head;
    int ini = 0;
    int64_t val = 0;
    while ((line = line->next)) {
        if (line->type == TYPE_INS) {
            if (line->y64bin.bytes > 2) {
            
                if (line->symbol_name && *(line->symbol_name))
                {
                    s = find_symbol(line->symbol_name);
                    if (s != NULL) line->imm = s->addr;
                    else {
                        err_print("Unknown symbol:'%s'", line->symbol_name);
                        return -1;
                    }
                }
                /*fill in imm to codes*/
                val = line->imm;
                ini = line->y64bin.bytes - 8;
                for(int j=8;j>0;j--) {
                    (line->y64bin.codes)[ini] = (val & 0xff);
                    ini++;
                    val >>= 8;
                }
            }
            wow[nwow] = line->y64bin;
            nwow++;
        } else if (line->type == TYPE_LABEL) {
            if (line->y64bin.bytes == 8) {
                if (line->symbol_name && IS_LETTER(line->symbol_name))
                {
                    s = find_symbol(line->symbol_name);
                    if (s != NULL) line->imm = s->addr;
                    else {
                        err_print("Unknown symbol:'%s'", line->symbol_name);
                        return -1;
                    }
                }
                /*fill in imm to codes*/
            }else  if (line->symbol_name && IS_LETTER(line->symbol_name))
                {
                    s = find_symbol(line->symbol_name);
                    if (s != NULL) line->imm = s->addr;
                    else {
                        err_print("Unknown symbol:'%s'", line->symbol_name);
                        return -1;
                    }
                }

            val = line->imm;
            ini = 0;
            for(int j=8;j>0;j--) {
                line->y64bin.codes[ini] = (val & 0xff);
                ini++;
                val >>= 8;
            }
            wow[nwow] = line->y64bin;
            nwow++;
        }
    }
    return 0;
}
/*
 * binfile: generate the y64 binary file
 * args
 *     out: point to output file (an y64 binary file)
 *
 * return
 *     0: success
 *     -1: error
 */
int binfile(FILE *out)
{
    /* prepare image with y64 binary code */
    int64_t addr = wow[0].addr;
    byte_t zero[2048];
    memset(zero, 0 ,sizeof(byte_t)*2048);
    for (int i=0;i < nwow;i++) {
        if (i==nwow-1&&wow[i].bytes==0) break;
        if (wow[i].addr > addr)
            {
                fwrite(zero, 1, wow[i].addr-addr, out);
                addr = wow[i].addr;
            }
        fwrite(wow[i].codes, 1, wow[i].bytes, out);
        addr += wow[i].bytes;
    }
    /* binary write y64 code to output file (NOTE: see fwrite()) */
    
    return 0;
}


/* whether print the readable output to screen or not ? */
bool_t screen = FALSE; 

static void hexstuff(char *dest, int value, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        char c;
        int h = (value >> 4*i) & 0xF;
        c = h < 10 ? h + '0' : h - 10 + 'a';
        dest[len-i-1] = c;
    }

}

void print_line(line_t *line)
{
    char buf[33];

    /* line format: 0xHHH: cccccccccccc | <line> */
    if (line->type == TYPE_INS||line->type == TYPE_LABEL) {
        bin_t *y64bin = &line->y64bin;
        int i;
        strcpy(buf, "  0x000:                      | ");
        
        hexstuff(buf+4, y64bin->addr, 3);
        if (y64bin->bytes > 0)
            for (i = 0; i < y64bin->bytes; i++)
                hexstuff(buf+9+2*i, y64bin->codes[i]&0xFF, 2);
    } else {
        strcpy(buf, "                              | ");
    }
    printf("%s%s\n", buf, line->y64asm);
}

/* 
 * print_screen: dump readable binary and assembly code to screen
 * (e.g., Figure 4.8 in ICS book)
 */
void print_screen(void)
{
    line_t *tmp = line_head->next;
    while (tmp != NULL) {
        print_line(tmp);
        tmp = tmp->next;
    }
}

/* init and finit */
void init(void)
{
    reltab = (reloc_t *)malloc(sizeof(reloc_t)); // free in finit
    memset(reltab, 0, sizeof(reloc_t));
    symtab = (symbol_t *)malloc(sizeof(symbol_t)); // free in finit
    memset(symtab, 0, sizeof(symbol_t));

    line_head = (line_t *)malloc(sizeof(line_t)); // free in finit
    memset(line_head, 0, sizeof(line_t));
    memset(wow, 0, sizeof(bin_t)*MAX_INSLEN);
    line_tail = line_head;
    lineno = 0;
}

void finit(void)
{
    
    reloc_t *rtmp = NULL;
    do {
        rtmp = reltab->next;
        if (reltab->name) 
            free(reltab->name);
        free(reltab);
        reltab = rtmp;
    } while (reltab);
   
    symbol_t *stmp = NULL;
    do {
        stmp = symtab->next;
        if (symtab->name) 
            free(symtab->name);
        free(symtab);
        symtab = stmp;
    } while (symtab);
   
    line_t *ltmp = NULL;
    do {
        ltmp = line_head->next;
        free(line_head);
        line_head = ltmp;
    } while (line_head);
}

static void usage(char *pname)
{
    printf("Usage: %s [-v] file.ys\n", pname);
    printf("   -v print the readable output to screen\n");
    exit(0);
}

int main(int argc, char *argv[])
{

    int rootlen;
    char infname[512];
    char outfname[512];
    int nextarg = 1;
    FILE *in = NULL, *out = NULL;
    
    if (argc < 2)
        usage(argv[0]);
    
    if (argv[nextarg][0] == '-') {
        char flag = argv[nextarg][1];
        switch (flag) {
          case 'v':
            screen = TRUE;
            nextarg++;
            break;
          default:
            usage(argv[0]);
        }
    }

    /* parse input file name */
    rootlen = strlen(argv[nextarg])-3;
    /* only support the .ys file */
    if (strcmp(argv[nextarg]+rootlen, ".ys"))
        usage(argv[0]);
    
    if (rootlen > 500) {
        err_print("File name too long");
        exit(1);
    }
 

    /* init */
    init();

    
    /* assemble .ys file */
    strncpy(infname, argv[nextarg], rootlen);
    strcpy(infname+rootlen, ".ys");
    in = fopen(infname, "r");
    if (!in) {
        err_print("Can't open input file '%s'", infname);
        exit(1);
    }
    
    if (assemble(in) < 0) {
        err_print("Assemble y64 code error");
        fclose(in);
        exit(1);
    }
    fclose(in);
    /* relocate binary code */
    if (relocate() < 0) {
        err_print("Relocate binary code error");
        exit(1);
    }

    /* generate .bin file */
    strncpy(outfname, argv[nextarg], rootlen);
    strcpy(outfname+rootlen, ".bin");
    out = fopen(outfname, "wb");
    if (!out) {
        err_print("Can't open output file '%s'", outfname);
        exit(1);
    }
    
    if (binfile(out) < 0) {
        err_print("Generate binary file error");
        fclose(out);
        exit(1);
    }
    fclose(out);
    /* print to screen (.yo file) */
    if (screen)
       print_screen(); 
    /* finit */
    finit();
    return 0;
}


