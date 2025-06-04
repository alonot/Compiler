# Register Promise-Based Compiler Architecture

This document describes a compiler implementation that uses a promise-based register allocation system to efficiently manage register usage and automatic spilling during code generation.

## Overview

The core concept revolves around **Register Promises** - a deferred register allocation mechanism that handles the complexity of register management, spilling, and reloading transparently. Each expression in the compilation process is associated with a register promise that tracks where its value is stored and manages the logistics of making that value available when needed.

## Core Data Structure

### RegPromise

The `RegPromise` structure serves as the central abstraction for value storage and retrieval:

```c
typedef struct __reg_promise {
    Register* reg;        // Currently assigned register (NULL if spilled)
    lli* immediate;       // Immediate value (for compile-time constants)
    int offset;           // Stack offset for spilled values
    int loc;              // Memory location from stack pointer (NO_ENTRY if unused)
    STEntry* ste;         // Symbol table entry (NULL for expression results)
    String* label;        // Label reference for jumps/branches
} RegPromise;
```

This structure handles multiple scenarios:
- **Active registers**: When `reg` points to an allocated register
- **Spilled values**: When `reg` is NULL and `offset` contains stack location
- **Immediate values**: When `immediate` contains compile-time constants
- **Memory references**: When `loc` specifies stack-relative addressing
- **Symbols**: When `ste` references symbol table entries

## Register Management System

### Allocation Strategy

The register allocator follows a hierarchical approach through `get_free_register()`:

1. **Caller-saved registers first**: Prioritizes volatile registers to minimize save/restore overhead
2. **Callee-saved registers**: Uses non-volatile registers when caller-saved are unavailable  
3. **Spill oldest allocation**: When all registers are occupied, evicts the least recently assigned register using a queue-based LRU policy

### Targeted Allocation

The `get_specific_register()` function allows requesting specific registers (e.g., `$s0`) by:
- Checking if the target register is available
- Freeing the register if currently occupied (triggering spill if necessary)
- Returning the requested register for immediate use

## Automatic Spill and Reload

### Spill Mechanism

When a register must be freed:
1. The value is written to a stack location
2. The associated `RegPromise` updates its `offset` field with the stack position
3. The `reg` field is set to NULL to indicate the value is no longer in a register

### Reload Operations

Two reload functions handle different scenarios:

#### `reload_reg()`
- Used for basic expression evaluation
- Checks if the promise's value is currently in a register
- If spilled (`reg == NULL`), allocates a new register and loads the value from stack
- Returns the register containing the live value

#### `reload_withoffset()`
- Specialized for memory operations (arrays, pointer dereferences)
- Handles cases where the value itself represents a memory address
- Distinguishes between spilled registers and legitimate memory references

## Code Generation Integration

The instruction generation functions in `instructions.c` seamlessly integrate with the register promise system:

### Transparent Loading
Each instruction function automatically calls the appropriate reload function before operating on values, ensuring that:
- Spilled values are restored to registers when needed
- Memory references are properly resolved
- The programmer doesn't need to manually manage register state

### Compile-Time Optimization
When both operands of an operation contain immediate values:
- The operation is performed at compile time
- The result is stored as an immediate in the resulting register promise
- No runtime instruction is generated for the operation

## Benefits of This Approach

### Simplified Code Generation
- Expression evaluation code doesn't need to track register allocation state
- Spill/reload logic is centralized and automatic
- Complex addressing modes are handled transparently

### Efficient Register Usage
- LRU-based spilling minimizes unnecessary memory traffic
- Caller/callee-saved prioritization reduces function call overhead
- Immediate value optimization eliminates redundant load operations

### Maintainable Architecture
- Clear separation between register allocation and instruction generation
- Consistent interface for value access regardless of storage location
- Easy to extend with additional optimization passes


This architecture provides a robust foundation for register allocation while keeping the complexity hidden from the instruction generation layer, resulting in cleaner and more maintainable compiler code.

<p style="font-size:11px;">
Credits: Claude.ai
</p>
