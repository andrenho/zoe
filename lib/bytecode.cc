#include "bytecode.h"

#include <inttypes.h>

#include "opcode.h"

extern int parse(Zoe::Bytecode& bc, string const& code);  // defined in lib/parser.y


namespace Zoe {


Bytecode Bytecode::FromCode(string const& code)
{
    Bytecode bc;
    parse(bc, code);  // magic happens
    return bc;
}


void Bytecode::Disassemble(FILE* f, vector<uint8_t> const& data)
{
    uint64_t p = 4;

    auto next = [&](uint64_t sz) {  // {{{
        for(uint8_t i=0; i<sz; ++i) {
            fprintf(f, "%02X ", data[p+i]);
        }
        fprintf(f, "\n");
        p += sz;
    }; /// }}}

    int ns;
    while(p < data.size()) {
        fprintf(f, "%08" PRIx64 ":\t", p);
        switch(data[p]) {
            case PUSH_I:
                fprintf(f, "push_i\t");
                ns = fprintf(f, "%" PRId64, ZInteger(data, p+1).Value());
                fprintf(f, "%*s", 23-ns, " ");
                next(9);
                break;
            default:
                fprintf(stderr, "Invalid opcode %02X\n", data[p]);
                return;
        }
    }
}


}

// vim: ts=4:sw=4:sts=4:expandtab:foldmethod=marker