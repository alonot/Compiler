NAME: Priyanshu Pandey
ROLL NO: 112201020

Tools used:
```CONFIG
bison (GNU Bison) 3.8.2
flex 2.6.4
gcc (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0
qemu-mips version 8.2.2 (Debian 1:8.2.2+ds-0ubuntu1.6)
mips-linux-gnu-gcc-10 (Ubuntu 10.5.0-4ubuntu2) 10.5.0
valgrind-3.22.0
qemu-user
qemu-user-static
```

NOTE: For previous versions of bison, compiler.y may throw error due to 
`%define parse.error verbose` in line 41. 
Please comment it if such error occurs

---

### Running the compiler:

* Generates the assembly code in the current working directory : `pwd`

The current compiler covers:
* integer
* Expressions, increament, decreament
* if, while, dowhile, for
* continue, break, return
* multi-dimension arrays
* Function calls
* string labels

#### To run the compiler:
```
{path_to_compiler(bin)}/compiler -o <output file location> -d <debug file location> -v <input file>
```
Arguments:
1. **-o <output_file>** : to specify where to output the assembly instructions.
    *   If not specified, set to `<input_filename>.s`

2. **-v** : prevent printing of the debug information. Default : Debug information is **printed**.

3. **-d <debug_file>** : to specify where to output the compiler debugging information, syntax tree and symbol table, **`if -v option is not set`**

    * If not specified, set to `stdout`
4. **<input_file>** : to specify where to output the assembly instructions.
    * If not specified, set to `stdin`

### Example Run:
Assumption: Test is saved in ./test/a.sil
1. Compile the file to generate assembly in `pwd` as a.s:

    ```
    ./bin/compiler ./test/a.sil
    ```

    This print the ast and symbol table in stdout.

2. Compile the assembly file and run on qemu:

    ```
    mips-linux-gnu-gcc-10 ./a.s 
	qemu-mips -L /usr/mips-linux-gnu/ ./a.out
    ```

Example Program:
```

decl
  integer a, b;
enddecl

begin
  read(a, b);
  write(a, b);

  read(a);
  write(a, b); // Make sure b is not changed

  read(b);
  write(a, b); // Make sure a is not changed
end

// 1
// 2
// 1 2
// 4
// 4 2
// 3
// 4 3
```
AST and Symbol table:
```
PROG
    |-FUNC_DEF(main)
            |-FUNC_RET
            |-ARG_LIST
            |-DECL
                    |-KEYWORD(INTEGER)
                            |-(int)(a)
                            |-(int)(b)
            |-FUNC_BODY
                    |-CALL(read)
                            |-PARAM_LIST
                                    |-EXPR(1)
                                            |-(int)(a)
                                    |-EXPR(1)
                                            |-(int)(b)
                    |-CALL(write)
                            |-PARAM_LIST
                                    |-EXPR(1)
                                            |-(int)(a)
                                    |-EXPR(1)
                                            |-(int)(b)
                    |-CALL(read)
                            |-PARAM_LIST
                                    |-EXPR(1)
                                            |-(int)(a)
                    |-CALL(write)
                            |-PARAM_LIST
                                    |-EXPR(1)
                                            |-(int)(a)
                                    |-EXPR(1)
                                            |-(int)(b)
                    |-CALL(read)
                            |-PARAM_LIST
                                    |-EXPR(1)
                                            |-(int)(b)
                    |-CALL(write)
                            |-PARAM_LIST
                                    |-EXPR(1)
                                            |-(int)(a)
                                    |-EXPR(1)
                                            |-(int)(b)
Symbol Table
|         a |                  INT       | 0
|         b |                  INT       | 0
```

---

### ``` Rules```
* The code written directly without any function(like in python) will be treated as to be running inside a function named `main`.
* Starting point of program is `main`.
* `TODO`: When compiling multiple files, then all the global sections will be stacked upon one - another and then will be packed up in the `main` function. The compiler does not garauntee any order in the stacking up of code. As long as individual code compiles, compiler will not generate any error. `No user function can be named as main.`. 
* calling function will save the arguments directly to the stack of the callee. 
* Stack of any function will be as follows:

```
+---------------+
|     argn      | --|
|     ....     |   |
|     ....      |   | --- filled by caller
|     argn1     |   |
|     argn0     | --|
+---------------+  < ------ sp
|     sp        |  --|
|     fp        |  --| -- filled by callee
|    local0     | --|
|    local1     |   | --- filled by callee
|    ......     |   |
|    ......     |   .
|    ......     |   .
|    ......     |   .
|callee saved r0|   .
|callee saved r1|   .
|    ......     |   .
+---------------+
```

```
Reg Name | Number | Alias | Usage / Purpose | Saved By
$zero | $0 | constant | Always zero | —
$at | $1 | assembler | Reserved for assembler |
$v0 | $2 | value | Return value 1 | Caller
$v1 | $3 | value | Return value 2 | Caller
$a0 | $4 | arg0 | Temporary | Caller
$a1 | $5 | arg1 | Temporary | Caller
$a2 | $6 | arg2 | Temporary | Caller
$a3 | $7 | arg3 | Temporary | Caller
$t0 | $8 | temp0 | Temporary | Caller
$t1 | $9 | temp1 | Temporary | Caller
$t2 | $10 | temp2 | Temporary | Caller
$t3 | $11 | temp3 | Temporary | Caller
$t4 | $12 | temp4 | Temporary | Caller
$t5 | $13 | temp5 | Temporary | Caller
$t6 | $14 | temp6 | Temporary | Caller
$t7 | $15 | temp7 | Temporary | Caller
$s0 | $16 | saved0 | Callee-saved variable | Callee
$s1 | $17 | saved1 | Callee-saved variable | Callee
$s2 | $18 | saved2 | Callee-saved variable | Callee
$s3 | $19 | saved3 | Callee-saved variable | Callee
$s4 | $20 | saved4 | Callee-saved variable | Callee
$s5 | $21 | saved5 | Callee-saved variable | Callee
$s6 | $22 | saved6 | Callee-saved variable | Callee
$s7 | $23 | saved7 | Callee-saved variable | Callee
$t8 | $24 | temp8 | Temporary | Caller
$t9 | $25 | temp9 | Temporary | Caller
----- 
***Not used by this compiler**
$k0 | $26 | kernel0 | OS kernel use only | —
$k1 | $27 | kernel1 | OS kernel use only | —
-----
$gp | $28 | global | Global pointer | —
$sp | $29 | stack | Stack pointer | —
$fp | $30 | frame | Frame pointer / saved register ($s8) | Callee
$ra | $31 | return | Return address | Caller

```

* The best way to evaluate the expression tree is.. to evaluate the child with greater depth. if both have same depth then first evaluate left.
Problem is that by convention, expressions must be evaluated left to right. And expressions may contain increament operator which can impact right part of the expression.

* One solution is to evaluate all the increment operator before hand and then evaluate using above method, using corrent version of that variable where required. But here, we go with convectional way i.e. evaluate left child first, then the right child.

* In both the given ways if required then store the register value in the stack, in case of shortage of registers. The first way may use less register in most cases, if increament is handled correctly.


* in expressions like (a + (a ++ - 10)). if a = 10 then c outputs  10. 

* When doing function calls through register it is necessary in mips to keep the function address in $25. This is because in shared libraries or PIC code, the function prologue uses $25 to determine which module's GOT to load.

* the callee (your function) must always reserve space for 4 arguments on the stack 