#ifndef quasar_vm_h
#define quasar_vm_h

#include "object.h"
#include "table.h"
#include "value.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
    ObjClosure *closure;
    uint8_t *ip;
    Value *slots;
} CallFrame;

typedef struct {
    CallFrame frames[FRAMES_MAX];
    int frame_count;
    
    Value stack[STACK_MAX];
    Value *sp;
    Table globals;
    Table strings;
    Obj *objects;
} VM;

typedef enum {
    INTEPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

void initVM();
void freeVM();
InterpretResult interpret(const char *filename, const char *source);
void push(Value value);
Value pop();

#endif