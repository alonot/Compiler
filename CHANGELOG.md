#### Week 1
Made `Simple Calculator`
1. It can handle any number of variable assignments and calculations using semicolons in same line. 
2. Prints the variables store inside the symbol table on pressing ENTER
3. Can also handle Brackets i.e. `(10 + 20) * 30` will give `900` and `10 + 20 * 30`  will give `610`.
4. Also Supports string variable names following `[a-zA-Z]+` rule.

#### Week 2
Initial compiler
1. Created a String type, which was used to store levels while printing the parse tree.
2. Added functionality to store multiple children for each node. using .next pointer
3. Stack to create the Syntax tree.
4. Printing the output of each write statement

#### Week 3
Part 1
* double type implemented
* any depth array implemented
* leak fixes

Part 2
* removed stack 
* removed evaluation of expr while creating syntax tree
* print fixed with new approach

Part 3
* Conditional statements are evaluated as follows ... 
    1. Conditional statements are marked with t_COND
    2. in each node of type t_COND, 1st child is the `expression`, 2nd child is `if block`, and 3rd child is`else block` (in case `else` exists)
* Similarly for loops there is a `loop_block`.
* Added a new way to visualize trees. This new method uses indentations to display syntax trees and is more clean to visualize. Current program uses this new print function: `printTreeIdent`.
* changed write grammer to include string and variables together. 
* Implemented `read`.
* added `break` and `continue` to the grammer.

Part 4
* printed symbol table
* change in array address calculation


Part5 
* Came up with a modular way for register allocation and handle spill/fills automatically.
* Every function work with RegPromise. Each of these RegPromise may/may not hold an actual register. In case its reg field is empty and given some other checks, we can be sure that the register this promise pointed to was spilled, so we allocate a new register and fill it with the info in this promise.
* malloc() tcache error came many time, mainly due to multiple free
* Covered all the statements : for, while, do-while, conditionals, break, continue