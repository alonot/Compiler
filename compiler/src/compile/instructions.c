#include "../../include/includes.h"

void __loadw(Instructions_info* instr_info,Register* rd, Register* rs1) {
    write_instr( instr_info, "%-7s\t%s,\t%d(%s)", "lw", rd->name, rs1-> offset, rs1 -> name);
}

void __storew(Instructions_info* instr_info,Register* rs1, Register* rs2) {
    write_instr( instr_info, "%-7s\t%s,\t%lld(%s)", "sw", rs1->name, rs2-> offset, rs2 -> name);
}



RegPromise* get_immediate(Instructions_info* instr_info, lli immediate) {
    RegPromise* imm_reg = get_free_register_promise(instr_info);
    // printf("Half\n");
    write_instr( instr_info, "%-7s\t%s,\t%d", "lui",   imm_reg->reg->name, (immediate >> 16));
    immediate = immediate & DEF32;
    write_instr( instr_info, "%-7s\t%s,\t%s,\t%d", "addiu", imm_reg->reg->name, imm_reg->reg->name, immediate);
    return imm_reg;
}

void addiu(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, lli immediate) {
    if (immediate > MAX_32_BIT) {
        // printf("kk\n");
        RegPromise* imm_reg = get_immediate(instr_info, immediate);
        addu(instr_info, rd, rs1, imm_reg);
        __free_regpromise(imm_reg);
    } else {
        reload_reg(rd, instr_info); 
        // printf("done\n");
        reload_reg(rs1, instr_info);
        write_instr( instr_info, "%-7s\t%s,\t%s,\t%d", "addiu", rd->reg->name, rs1->reg->name, immediate);
    }
}

void sltiu(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, lli immediate) {
    if (immediate > MAX_32_BIT) {
        RegPromise* imm_reg = get_immediate(instr_info, immediate);
        sltu(instr_info,rd,rs1,imm_reg);
        __free_regpromise(imm_reg);
    } else {
        reload_reg(rd, instr_info); reload_reg(rs1, instr_info);
        write_instr( instr_info, "%-7s\t%s,\t%s,\t%d", "sltiu", rd->reg->name, rs1->reg->name, immediate);
    }
}

void slti(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, lli immediate) {
    if (immediate > MAX_32_BIT) {
        RegPromise* imm_reg = get_immediate(instr_info, immediate);
        slt(instr_info,rd,rs1,imm_reg);
        __free_regpromise(imm_reg);
    } else {
        reload_reg(rd, instr_info); reload_reg(rs1, instr_info);
        write_instr( instr_info, "%-7s\t%s,\t%s,\t%d","slti", rd->reg->name, rs1->reg->name, immediate);
    }
}

void xori(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, lli immediate) {
    if (immediate > MAX_32_BIT) {
        RegPromise* imm_reg = get_immediate(instr_info, immediate);
        xor(instr_info,rd,rs1,imm_reg);
        __free_regpromise(imm_reg);
    } else {
        reload_reg(rd, instr_info); reload_reg(rs1, instr_info);
        write_instr( instr_info, "%-7s\t%s,\t%s,\t%d","xori", rd->reg->name, rs1->reg->name, immediate);
    }
}

void andi(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, lli immediate) {
    if (immediate > MAX_32_BIT) {
        RegPromise* imm_reg = get_immediate(instr_info, immediate);
        and(instr_info,rd,rs1,imm_reg);
        __free_regpromise(imm_reg);
    } else {
        reload_reg(rd, instr_info); reload_reg(rs1, instr_info);
        write_instr( instr_info, "%-7s\t%s,\t%s,\t%d", "andi", rd->reg->name, rs1->reg->name, immediate);
    }
}

void ori(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, lli immediate) {
    if (immediate > MAX_32_BIT) {
        RegPromise* imm_reg = get_immediate(instr_info, immediate);
        or(instr_info,rd,rs1,imm_reg);
        __free_regpromise(imm_reg);
    } else {
        reload_reg(rd, instr_info); reload_reg(rs1, instr_info);
        write_instr( instr_info, "%-7s\t%s,\t%s,\t%d","ori", rd->reg->name, rs1->reg->name, immediate);
    }
}

void sll(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, lli immediate) {
    if (immediate > 32) {
        addiu(instr_info,rd,zero_reg_promise,0);
    } else {
        reload_reg(rd, instr_info); reload_reg(rs1, instr_info);
        write_instr( instr_info, "%-7s\t%s,\t%s,\t%d","sll", rd->reg->name, rs1->reg->name, immediate);
    }
}

void subiu(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, lli immediate) {
    if (immediate > MAX_32_BIT) {
        RegPromise* imm_reg = get_immediate(instr_info, immediate);
        subu(instr_info,rd,rs1,imm_reg);
        __free_regpromise(imm_reg);
    } else {
        reload_reg(rd, instr_info); reload_reg(rs1, instr_info);
        write_instr( instr_info, "%-7s\t%s,\t%s,\t%d","addiu", rd->reg->name, rs1->reg->name, -immediate);
    }
}

void addu(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, RegPromise* rs2) {
    if (rs1->immediate != NULL && rs2->immediate != NULL) {
        double* first_imm = (double*)(rs1->immediate);
        *first_imm += *(double*)(rs2->immediate);
        return;
    } else if (rs1->immediate != NULL) {
        lli immediate = (lli)*(double*)(rs1->immediate);
        if (rd == rs1) {
            RegPromise* new_promise = get_free_register_promise(instr_info);
            assign_reg_promise(rd, new_promise);
            free(new_promise);
        }
        return addiu(instr_info, rd,rs2, immediate);
    } else if (rs2->immediate != NULL) {
        lli immediate = (lli)*(double*)(rs2->immediate);
        if (rd == rs2) {
            RegPromise* new_promise = get_free_register_promise(instr_info);
            assign_reg_promise(rd, new_promise);
            free(new_promise);
        }
        return addiu(instr_info, rd,rs1, immediate);
    }
    reload_reg(rd, instr_info); reload_reg(rs1, instr_info); reload_reg(rs2, instr_info);
    write_instr( instr_info, "%-7s\t%s,\t%s,\t%s","addu", rd->reg->name, rs1->reg -> name, rs2->reg -> name);
}

void and(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, RegPromise* rs2) {
    if (rs1->immediate != NULL && rs2->immediate != NULL) {
        double* first_imm = (double*)(rs1->immediate);
        *first_imm = (lli)(*first_imm) & (lli)*(double*)(rs2->immediate);
        return;
    } else if (rs1->immediate != NULL) {
        lli immediate = (lli)*(double*)(rs1->immediate);
        if (rd == rs1) {
            RegPromise* new_promise = get_free_register_promise(instr_info);
            assign_reg_promise(rd, new_promise);
            free(new_promise);
        }
        return andi(instr_info, rd,rs2, immediate);
    } else if (rs2->immediate != NULL) {
        lli immediate = (lli)*(double*)(rs2->immediate);
        if (rd == rs2) {
            RegPromise* new_promise = get_free_register_promise(instr_info);
            assign_reg_promise(rd, new_promise);
            free(new_promise);
        }
        return andi(instr_info, rd,rs1, immediate);
    }
    reload_reg(rd, instr_info); reload_reg(rs1, instr_info); reload_reg(rs2, instr_info);
    write_instr( instr_info, "%-7s\t%s,\t%s,\t%s", "and", rd->reg->name, rs1->reg -> name, rs2->reg -> name);
}

void or(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, RegPromise* rs2) {
    if (rs1->immediate != NULL && rs2->immediate != NULL) {
        double* first_imm = (double*)(rs1->immediate);
        *first_imm = (lli)(*first_imm) | (lli)*(double*)(rs2->immediate);
        return;
    } else if (rs1->immediate != NULL) {
        lli immediate = (lli)*(double*)(rs1->immediate);
        if (rd == rs1) {
            RegPromise* new_promise = get_free_register_promise(instr_info);
            assign_reg_promise(rd, new_promise);
            free(new_promise);
        }
        return ori(instr_info, rd,rs2, immediate);
    } else if (rs2->immediate != NULL) {
        lli immediate = (lli)*(double*)(rs2->immediate);
        if (rd == rs2) {
            RegPromise* new_promise = get_free_register_promise(instr_info);
            assign_reg_promise(rd, new_promise);
            free(new_promise);
        }
        return ori(instr_info, rd,rs1, immediate);
    }
    reload_reg(rd, instr_info); reload_reg(rs1, instr_info); reload_reg(rs2, instr_info);
    write_instr( instr_info, "%-7s\t%s,\t%s,\t%s", "or", rd->reg->name, rs1->reg -> name, rs2->reg -> name);
}

void sllv(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, RegPromise* rs2) {
    if (rs1->immediate != NULL && rs2->immediate != NULL) {
        double* first_imm = (double*)(rs1->immediate);
        *first_imm += *(double*)(rs2->immediate);
        return;
    } else if (rs1->immediate != NULL) {
        lli immediate = (lli)*(double*)(rs1->immediate);
        RegPromise* new_reg = get_free_register_promise(instr_info);
        addiu(instr_info, new_reg, zero_reg_promise,  immediate);
        assign_reg_promise(rs1,new_reg);
        sllv(instr_info, rd,new_reg, rs2);
        __free_regpromise(new_reg);
    } else if (rs2->immediate != NULL) {
        lli immediate = (lli)*(double*)(rs2->immediate);
        if (rd == rs2) {
            RegPromise* new_promise = get_free_register_promise(instr_info);
            assign_reg_promise(rd, new_promise);
            free(new_promise);
        }
        return sll(instr_info, rd,rs1, immediate);
    }
    reload_reg(rd, instr_info); reload_reg(rs1, instr_info); reload_reg(rs2, instr_info);
    write_instr( instr_info, "%-7s\t%s,\t%s,\t%s", "sllv", rd->reg->name, rs1->reg -> name, rs2->reg -> name);
}

void xor(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, RegPromise* rs2) {
    if (rs1->immediate != NULL && rs2->immediate != NULL) {
        double* first_imm = (double*)(rs1->immediate);
        *first_imm = (lli)(*first_imm) ^ (lli)*(double*)(rs2->immediate);
        return;
    } else if (rs1->immediate != NULL) {
        lli immediate = (lli)*(double*)(rs1->immediate);
        if (rd == rs1) {
            RegPromise* new_promise = get_free_register_promise(instr_info);
            assign_reg_promise(rd, new_promise);
            free(new_promise);
        }
        return xori(instr_info, rd,rs2, immediate);
    } else if (rs2->immediate != NULL) {
        
        lli immediate = (lli)*(double*)(rs2->immediate);
        if (rd == rs2) {
            RegPromise* new_promise = get_free_register_promise(instr_info);
            assign_reg_promise(rd, new_promise);
            free(new_promise);
        }
        return xori(instr_info, rd,rs1, immediate);
    }
    reload_reg(rd, instr_info); reload_reg(rs1, instr_info); reload_reg(rs2, instr_info);
    write_instr( instr_info, "%-7s\t%s,\t%s,\t%s", "xor", rd->reg->name, rs1->reg -> name, rs2->reg -> name);
}

void subu(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, RegPromise* rs2) {
    if (rs1->immediate != NULL && rs2->immediate != NULL) {
        double* first_imm = (double*)(rs1->immediate);
        *first_imm -= *(double*)(rs2->immediate);
    } else if (rs1->immediate != NULL) {
        lli immediate = (lli)*(double*)(rs1->immediate);
        RegPromise* new_reg = get_free_register_promise(instr_info);
        subiu(instr_info, new_reg, zero_reg_promise,  immediate);
        assign_reg_promise(rs1,new_reg);
        subu(instr_info, rd,new_reg, rs2);
        free(new_reg);
    } else if (rs2->immediate != NULL) {
        lli immediate = (lli)*(double*)(rs2->immediate);
        if (rd == rs2) {
            RegPromise* new_promise = get_free_register_promise(instr_info);
            assign_reg_promise(rd, new_promise);
            free(new_promise);
        }
        subiu(instr_info, rd,rs1, immediate);
    } else {
        reload_reg(rd, instr_info); reload_reg(rs1, instr_info); reload_reg(rs2, instr_info);
        write_instr( instr_info, "%-7s\t%s,\t%s,\t%s", "subu", rd->reg->name, rs1->reg -> name, rs2->reg -> name);
    }
}

void slt(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, RegPromise* rs2) {
    if (rs1->immediate != NULL && rs2->immediate != NULL) {
        double* first_imm = (double*)(rs1->immediate);
        *first_imm = (*first_imm < *(double*)(rs2->immediate));
    } else if (rs1->immediate != NULL) {
        lli immediate = (lli)*(double*)(rs1->immediate);
        RegPromise* new_reg = get_free_register_promise(instr_info);
        addiu(instr_info, new_reg, zero_reg_promise,  immediate);
        assign_reg_promise(rs1,new_reg);
        slt(instr_info, rd,new_reg, rs2);
        free(new_reg);
    } else if (rs2->immediate != NULL) {
        lli immediate = (lli)*(double*)(rs2->immediate);
        if (rd == rs2) {
            RegPromise* new_promise = get_free_register_promise(instr_info);
            assign_reg_promise(rd, new_promise);
            free(new_promise);
        }
        slti(instr_info, rd,rs1, immediate);
    } else {
        reload_reg(rd, instr_info); reload_reg(rs1, instr_info); reload_reg(rs2, instr_info);
        write_instr( instr_info, "%-7s\t%s,\t%s,\t%s", "slt", rd->reg->name, rs1->reg -> name, rs2->reg -> name);
    }
}

void sltu(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, RegPromise* rs2) {
    if (rs1->immediate != NULL && rs2->immediate != NULL) {
        double* first_imm = (double*)(rs1->immediate);
        *first_imm = (*first_imm < *(double*)(rs2->immediate));
    } else if (rs1->immediate != NULL) {
        lli immediate = (lli)*(double*)(rs1->immediate);
        RegPromise* new_reg = get_free_register_promise(instr_info);
        addiu(instr_info, new_reg, zero_reg_promise,  immediate);
        assign_reg_promise(rs1,new_reg);
        sltu(instr_info, rd,new_reg, rs2);
        free(new_reg);
    } else if (rs2->immediate != NULL) {
        lli immediate = (lli)*(double*)(rs2->immediate);
        if (rd == rs2) {
            RegPromise* new_promise = get_free_register_promise(instr_info);
            assign_reg_promise(rd, new_promise);
            free(new_promise);
        }
        sltiu(instr_info, rd,rs1, immediate);
    } else {
        reload_reg(rd, instr_info); reload_reg(rs1, instr_info); reload_reg(rs2, instr_info);
        write_instr( instr_info, "%-7s\t%s,\t%s,\t%s", "sltu", rd->reg->name, rs1->reg -> name, rs2->reg -> name);
    }
}

void nor(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1, RegPromise* rs2) {
    reload_reg(rd, instr_info); reload_reg(rs1, instr_info); reload_reg(rs2, instr_info);
    write_instr( instr_info, "%-7s\t%s,\t%s,\t%s", "nor", rd->reg->name, rs1->reg -> name, rs2->reg -> name);
}

void mult(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1) {
    if (rs1->immediate != NULL && rd->immediate != NULL) {
        double* first_imm = (double*)(rd->immediate);
        *first_imm *= *(double*)(rs1->immediate);
    } else if (rs1->immediate != NULL) {
        lli immediate = (lli)*(double*)(rs1->immediate);
        RegPromise* new_reg = get_free_register_promise(instr_info);
        addiu(instr_info, new_reg, zero_reg_promise,  immediate);
        mult(instr_info, rd, new_reg);
        assign_reg_promise(rs1,new_reg);
        free(new_reg);
    } else if (rd->immediate != NULL) {
        lli immediate = (lli)*(double*)(rd->immediate);
        RegPromise* new_reg = get_free_register_promise(instr_info);
        addiu(instr_info, new_reg, zero_reg_promise,  immediate);
        mult(instr_info, new_reg,rs1);
        assign_reg_promise(rd,new_reg);
        free(new_reg); // freeing withour function call, so that the register does not gets freed
    } else {
        reload_reg(rd, instr_info); reload_reg(rs1, instr_info);
        write_instr( instr_info, "%-7s\t%s,\t%s", "mult",rd->reg->name, rs1->reg -> name);
    }
}

void mflo(Instructions_info* instr_info,RegPromise* rd) {
    reload_reg(rd, instr_info);
    write_instr( instr_info, "%-7s\t%s", "mflo", rd->reg->name);
}

void mfhi(Instructions_info* instr_info,RegPromise* rd) {
    reload_reg(rd, instr_info);
    write_instr( instr_info, "%-7s\t%s", "mfhi", rd->reg->name);
}

void mips_div(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1) {
    if (rs1->immediate != NULL && rd->immediate != NULL) {
        double* first_imm = (double*)(rd->immediate);
        *first_imm /= *(double*)(rs1->immediate);
    } else if (rs1->immediate != NULL) {
        lli immediate = (lli)*(double*)(rs1->immediate);
        RegPromise* new_reg = get_free_register_promise(instr_info);
        addiu(instr_info, new_reg, zero_reg_promise,  immediate);
        mips_div(instr_info, rd, new_reg);
        assign_reg_promise(rs1,new_reg);
        free(new_reg);
    } else if (rd->immediate != NULL) {
        lli immediate = (lli)*(double*)(rd->immediate);
        RegPromise* new_reg = get_free_register_promise(instr_info);
        addiu(instr_info, new_reg, zero_reg_promise,  immediate);
        mips_div(instr_info, new_reg,rs1);
        assign_reg_promise(rd,new_reg);
        free(new_reg); // freeing withour function call, so that the register does not gets freed
    } else {
        reload_reg(rd, instr_info); reload_reg(rs1, instr_info);
        write_instr( instr_info, "%-7s\t%s,\t%s", "div", rd->reg->name, rs1->reg -> name);
    }
}

void loadw(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1) {
    reload_reg(rd, instr_info); reload_reg(rs1, instr_info);
    if (rs1->reg->offset != NO_ENTRY && (rs1 != fp_reg_promise || rs1 != sp_reg_promise || rs1 != gp_reg_promise )) {
        // then load the value through s0
        RegPromise* s0_reg_promise = get_specific_register_promise(instr_info,S0_loc);
        move(instr_info, s0_reg_promise, rs1); // just move the register value to s0
        s0_reg_promise->reg->offset = rs1->reg->offset; // load the value from the offset
        __loadw(instr_info, rd->reg, s0_reg_promise->reg);
        __free_regpromise(s0_reg_promise);
    } else {
        __loadw(instr_info, rd->reg, rs1->reg);
    }
}

void move(Instructions_info* instr_info,RegPromise* rd, RegPromise* rs1) {
    reload_reg(rd, instr_info); reload_reg(rs1, instr_info);
    write_instr( instr_info, "%-7s\t%s,\t%s", "move", rd->reg->name, rs1->reg -> name);
}

void storew(Instructions_info* instr_info,RegPromise* rs1, RegPromise* rs2) {
    reload_reg(rs1, instr_info); reload_reg(rs2, instr_info);
    if (rs2->reg->offset != NO_ENTRY && (rs2 != fp_reg_promise || rs2 != sp_reg_promise || rs2 != gp_reg_promise )) {
        // then store the value through s0
        RegPromise* s0_reg_promise = get_specific_register_promise(instr_info,S0_loc);
        move(instr_info, s0_reg_promise, rs2); // just move the register value to s0
        s0_reg_promise->reg->offset = rs2->reg->offset; // load the value from the offset
        __storew(instr_info, rs1->reg, s0_reg_promise->reg);
        __free_regpromise(s0_reg_promise);
    } else {
        __storew(instr_info, rs1->reg, rs2->reg);
    }
}

void beq(Instructions_info* instr_info,RegPromise* rs1, RegPromise* rs2, char* label) {
    reload_reg(rs1, instr_info); reload_reg(rs2, instr_info);
    write_instr( instr_info, "%-7s\t%s,\t%s,%s\n\tnop", "beq", rs1->reg->name, rs2->reg->name, label);
}

void bne(Instructions_info* instr_info,RegPromise* rs1, RegPromise* rs2, char* label) {
    reload_reg(rs1, instr_info); reload_reg(rs2, instr_info);
    write_instr( instr_info, "%-7s\t%s,\t%s,%s\n\tnop", "bne", rs1->reg->name, rs2->reg->name, label);
}
