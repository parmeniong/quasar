#include <stdio.h>

#include "debug.h"
#include "object.h"
#include "value.h"

void disassembleChunk(Chunk *chunk, const char *name) {
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(chunk, offset);
    }
}

static int constantInstruction(const char *name, Chunk *chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 2;
}

static int simpleInstruction(const char *name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

static int byteInstruction(const char *name, Chunk *chunk, int offset) {
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

static int jumpInstruction(const char *name, int sign, Chunk *chunk, int offset) {
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];
    printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

int disassembleInstruction(Chunk *chunk, int offset) {
    printf("%04d ", offset);
    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
        printf("   | ");
    } else {
        printf("%4d ", chunk->lines[offset]);
    }

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_JUMP:
            return jumpInstruction("JUMP", 1, chunk, offset);
        case OP_JUMP_IF_FALSE:
            return jumpInstruction("JUMP_IF_FALSE", 1, chunk, offset);
        case OP_LOOP:
            return jumpInstruction("LOOP", -1, chunk, offset);
        case OP_CALL:
            return byteInstruction("CALL", chunk, offset);
        case OP_CLOSURE: {
            offset++;
            uint8_t constant = chunk->code[offset++];
            printf("%-16s %4d ", "CLOSURE", constant);
            printValue(chunk->constants.values[constant]);
            printf("\n");

            ObjFunction *function = AS_FUNCTION(chunk->constants.values[constant]);
            for (int j = 0; j < function->upvalue_count; j++) {
                int is_local = chunk->code[offset++];
                int index = chunk->code[offset++];
                printf("%04d      |                     %s %d\n", offset - 2, is_local ? "local" : "upvalue", index);
            }
            
            return offset;
        }
        case OP_RETURN:
            return simpleInstruction("RETURN", offset);
        case OP_CONSTANT:
            return constantInstruction("CONSTANT", chunk, offset);
        case OP_NULL:
            return simpleInstruction("NULL", offset);
        case OP_TRUE:
            return simpleInstruction("TRUE", offset);
        case OP_FALSE:
            return simpleInstruction("FALSE", offset);
        case OP_POP:
            return simpleInstruction("POP", offset);
        case OP_GET_LOCAL:
            return byteInstruction("GET_LOCAL", chunk, offset);
        case OP_SET_LOCAL:
            return byteInstruction("SET_LOCAL", chunk, offset);
        case OP_GET_GLOBAL:
            return constantInstruction("GET_GLOBAL", chunk, offset);
        case OP_DEFINE_GLOBAL:
            return constantInstruction("DEFINE_GLOBAL", chunk, offset);
        case OP_SET_GLOBAL:
            return constantInstruction("SET_GLOBAL", chunk, offset);
        case OP_GET_UPVALUE:
            return byteInstruction("GET_UPVALUE", chunk, offset);
        case OP_SET_UPVALUE:
            return byteInstruction("SET_UPVALUE", chunk, offset);
        case OP_EQUAL:
            return simpleInstruction("EQUAL", offset);
        case OP_GREATER:
            return simpleInstruction("GREATER", offset);
        case OP_LESS:
            return simpleInstruction("LESS", offset);
        case OP_ADD:
            return simpleInstruction("ADD", offset);
        case OP_SUBTRACT:
            return simpleInstruction("SUBTRACT", offset);
        case OP_MULTIPLY:
            return simpleInstruction("MULTIPLY", offset);
        case OP_DIVIDE:
            return simpleInstruction("DIVIDE", offset);
        case OP_NOT:
            return simpleInstruction("NOT", offset);
        case OP_NEGATE:
            return simpleInstruction("NEGATE", offset);
        case OP_CONCATENATE:
            return simpleInstruction("CONCATENATE", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}