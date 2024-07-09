#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"
#include "vm.h"

typedef enum {
    BIN_OP_ADDITION,
    BIN_OP_SUBTRACTION,
    BIN_OP_MULTIPLICATION,
    BIN_OP_DIVISION,
    BIN_OP_GREATER,
    BIN_OP_LESS
} BinaryOp;

VM vm;

static Value printNative(int arg_count, Value *args) {
    for (int i = 0; i < arg_count - 1; i++) {
        printValue(args[i]);
        printf(" ");
    }
    if (arg_count > 0) {
        printValue(args[arg_count - 1]);
    }
    printf("\n");
    return NULL_VAL;
}

static Value clockNative(int arg_count, Value *args) {
    return FLOAT_VAL((float)clock() / CLOCKS_PER_SEC);
}

static void resetStack() {
    vm.sp = vm.stack;
    vm.frame_count = 0;
    vm.open_upvalues = NULL;
}

static void runtimeError(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm.frame_count - 1; i >= 0; i--) {
        CallFrame *frame = &vm.frames[i];
        ObjFunction *function = frame->closure->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "\x1b[1;90m[%s:%d] in %s\x1b[0m\n", function->chunk.filename, function->chunk.lines[instruction], function->name == NULL ? "<script>" : function->name->chars);
    }

    resetStack();
}

static void defineNative(const char *name, NativeFn function) {
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

void initVM() {
    resetStack();
    vm.objects = NULL;

    initTable(&vm.globals);
    initTable(&vm.strings);

    defineNative("print", printNative);
    defineNative("clock", clockNative);
}

void freeVM() {
    freeTable(&vm.globals);
    freeTable(&vm.strings);
    freeObjects();
}

void push(Value value) {
    *vm.sp = value;
    vm.sp++;
}

Value pop() {
    vm.sp--;
    return *vm.sp;
}

static Value peek(int distance) {
    return vm.sp[-1 - distance];
}

static bool call(ObjClosure *closure, int arg_count) {
    if (arg_count != closure->function->arity) {
        runtimeError("Expected %d arguments, got %d", closure->function->arity, arg_count);
        return false;
    }

    if (vm.frame_count == FRAMES_MAX) {
        runtimeError("Stack overflow");
        return false;
    }
    
    CallFrame *frame = &vm.frames[vm.frame_count++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    frame->slots = vm.sp - arg_count - 1;
    return true;
}

static bool callValue(Value callee, int arg_count) {
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_CLOSURE: return call(AS_CLOSURE(callee), arg_count);
            case OBJ_NATIVE: {
                NativeFn native = AS_NATIVE(callee);
                Value result = native(arg_count, vm.sp - arg_count);
                vm.sp -= arg_count + 1;
                push(result);
                return true;
            }
            default: break;
        }
    }
    runtimeError("Can't call non-function values");
    return false;
}

static ObjUpvalue *captureUpvalue(Value *local) {
    ObjUpvalue *prev_upvalue = NULL;
    ObjUpvalue *upvalue = vm.open_upvalues;
    while (upvalue != NULL && upvalue->location > local) {
        prev_upvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != NULL && upvalue->location == local) {
        return upvalue;
    }
    
    ObjUpvalue *created_upvalue = newUpvalue(local);
    created_upvalue->next = upvalue;

    if (prev_upvalue == NULL) {
        vm.open_upvalues = created_upvalue;
    } else {
        prev_upvalue->next = created_upvalue;
    }
    
    return created_upvalue;
}

static void closeUpvalues(Value *last) {
    while (vm.open_upvalues != NULL && vm.open_upvalues->location >= last) {
        ObjUpvalue *upvalue = vm.open_upvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm.open_upvalues = upvalue->next;
    }
}

static bool isFalsey(Value value) {
    return IS_NULL(value)
        || (IS_BOOL(value) && !AS_BOOL(value))
        || (IS_INT(value) && AS_INT(value) == 0)
        || (IS_FLOAT(value) && AS_FLOAT(value) == 0.0);
}

static void concatenate() {
    ObjString *b = AS_STRING(pop());
    ObjString *a = AS_STRING(pop());

    int length = a->length + b->length;
    char *chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString *result = takeString(chars, length);
    push(OBJ_VAL(result));
}

static bool binaryOperation(BinaryOp operation) {
    if (IS_INT(peek(1))) {
        if (IS_INT(peek(0))) {
            int b = AS_INT(pop());
            int a = AS_INT(pop());
            
            switch (operation) {
                case BIN_OP_ADDITION:       push(INT_VAL(a + b)); break;
                case BIN_OP_SUBTRACTION:    push(INT_VAL(a - b)); break;
                case BIN_OP_MULTIPLICATION: push(INT_VAL(a * b)); break;
                case BIN_OP_DIVISION:
                    float a_float = a;
                    float b_float = b;
                    push(FLOAT_VAL(a_float / b_float));
                    break;
                case BIN_OP_GREATER:        push(BOOL_VAL(a > b)); break;
                case BIN_OP_LESS:           push(BOOL_VAL(a < b)); break;
            }

            return true;
        } else if (IS_FLOAT(peek(0))) {
            float b = AS_FLOAT(pop());
            int a = AS_INT(pop());

            switch (operation) {
                case BIN_OP_ADDITION:       push(FLOAT_VAL(a + b)); break;
                case BIN_OP_SUBTRACTION:    push(FLOAT_VAL(a - b)); break;
                case BIN_OP_MULTIPLICATION: push(FLOAT_VAL(a * b)); break;
                case BIN_OP_DIVISION:
                    float a_float = a;
                    float b_float = b;
                    push(FLOAT_VAL(a_float / b_float));
                    break;
                case BIN_OP_GREATER:        push(BOOL_VAL(a > b)); break;
                case BIN_OP_LESS:           push(BOOL_VAL(a < b)); break;
            }

            return true;
        } else {
            runtimeError("Both operands must be numbers");
            return false;
        }
    } else if (IS_FLOAT(peek(1))) {
        if (IS_INT(peek(0))) {
            int b = AS_INT(pop());
            float a = AS_FLOAT(pop());

            switch (operation) {
                case BIN_OP_ADDITION:       push(FLOAT_VAL(a + b)); break;
                case BIN_OP_SUBTRACTION:    push(FLOAT_VAL(a - b)); break;
                case BIN_OP_MULTIPLICATION: push(FLOAT_VAL(a * b)); break;
                case BIN_OP_DIVISION:
                    float a_float = a;
                    float b_float = b;
                    push(FLOAT_VAL(a_float / b_float));
                    break;
                case BIN_OP_GREATER:        push(BOOL_VAL(a > b)); break;
                case BIN_OP_LESS:           push(BOOL_VAL(a < b)); break;
            }

            return true;
        } else if (IS_FLOAT(peek(0))) {
            float b = AS_FLOAT(pop());
            float a = AS_FLOAT(pop());

            switch (operation) {
                case BIN_OP_ADDITION:       push(FLOAT_VAL(a + b)); break;
                case BIN_OP_SUBTRACTION:    push(FLOAT_VAL(a - b)); break;
                case BIN_OP_MULTIPLICATION: push(FLOAT_VAL(a * b)); break;
                case BIN_OP_DIVISION:
                    float a_float = a;
                    float b_float = b;
                    push(FLOAT_VAL(a_float / b_float));
                    break;
                case BIN_OP_GREATER:        push(BOOL_VAL(a > b)); break;
                case BIN_OP_LESS:           push(BOOL_VAL(a < b)); break;
            }

            return true;
        } else {
            runtimeError("Both operands must be numbers");
            return false;
        }
    } else {
        runtimeError("Both operands must be numbers");
        return false;
    }

    return true;
}

static InterpretResult run() {
    CallFrame *frame = &vm.frames[vm.frame_count - 1];
    
#define READ_BYTE() (*frame->ip++)

#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])

#define READ_STRING() AS_STRING(READ_CONSTANT())

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value *slot = vm.stack; slot < vm.sp; slot++) {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
        disassembleInstruction(&frame->closure->function->chunk, (int)(frame->ip - frame->closure->function->chunk.code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT:
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            case OP_NULL: push(NULL_VAL); break;
            case OP_TRUE: push(BOOL_VAL(true)); break;
            case OP_FALSE: push(BOOL_VAL(false)); break;
            case OP_POP: pop(); break;
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(frame->slots[slot]);
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(0);
                break;
            }
            case OP_GET_GLOBAL: {
                ObjString *name = READ_STRING();
                Value value;
                if (!tableGet(&vm.globals, name, &value)) {
                    runtimeError("Undefined variable '%s'", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            case OP_DEFINE_GLOBAL: {
                ObjString *name = READ_STRING();
                tableSet(&vm.globals, name, peek(0));
                pop();
                break;
            }
            case OP_SET_GLOBAL: {
                ObjString *name = READ_STRING();
                if (tableSet(&vm.globals, name, peek(0))) {
                    tableDelete(&vm.globals, name);
                    runtimeError("Undefined variable '%s'", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_GET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                push(*frame->closure->upvalues[slot]->location);
                break;
            }
            case OP_SET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                *frame->closure->upvalues[slot]->location = peek(0);
                break;
            }
            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OP_GREATER:
                if (!binaryOperation(BIN_OP_GREATER)) return INTERPRET_RUNTIME_ERROR;
                break;
            case OP_LESS:
                if (!binaryOperation(BIN_OP_LESS)) return INTERPRET_RUNTIME_ERROR;
                break;
            case OP_ADD:
                if (!binaryOperation(BIN_OP_ADDITION)) return INTERPRET_RUNTIME_ERROR;
                break;
            case OP_SUBTRACT:
                if (!binaryOperation(BIN_OP_SUBTRACTION)) return INTERPRET_RUNTIME_ERROR;
                break;
            case OP_MULTIPLY:
                if (!binaryOperation(BIN_OP_MULTIPLICATION)) return INTERPRET_RUNTIME_ERROR;
                break;
            case OP_DIVIDE:
                if (!binaryOperation(BIN_OP_DIVISION)) return INTERPRET_RUNTIME_ERROR;
                break;
            case OP_NOT:
                vm.sp[-1] = BOOL_VAL(isFalsey(vm.sp[-1]));
                break;
            case OP_NEGATE:
                if (IS_INT(peek(0))) {
                    vm.sp[-1] = INT_VAL(-AS_INT(vm.sp[-1]));
                } else if (IS_FLOAT(peek(0))) {
                    vm.sp[-1] = FLOAT_VAL(-AS_FLOAT(vm.sp[-1]));
                } else {
                    runtimeError("Operand must be a number");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            case OP_CONCATENATE:
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concatenate();
                } else {
                    runtimeError("Both operands must be strings");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (isFalsey(peek(0))) frame->ip += offset;
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }
            case OP_CALL: {
                int arg_count = READ_BYTE();
                if (!callValue(peek(arg_count), arg_count)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frame_count - 1];
                break;
            }
            case OP_CLOSURE: {
                ObjFunction *function = AS_FUNCTION(READ_CONSTANT());
                ObjClosure *closure = newClosure(function);
                push(OBJ_VAL(closure));
                for (int i = 0; i < closure->upvalue_count; i++) {
                    uint8_t is_local = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if (is_local) {
                        closure->upvalues[i] = captureUpvalue(frame->slots + index);
                    } else {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }
                break;
            }
            case OP_CLOSE_UPVALUE:
                closeUpvalues(vm.sp - 1);
                pop();
                break;
            case OP_RETURN:
                Value result = pop();
                closeUpvalues(frame->slots);
                vm.frame_count--;
                if (vm.frame_count == 0) {
                    pop();
                    return INTEPRET_OK;
                }

                vm.sp = frame->slots;
                push(result);
                frame = &vm.frames[vm.frame_count - 1];
                break;
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
}

InterpretResult interpret(const char *filename, const char *source) {
    ObjFunction *function = compile(filename, source);
    if (function == NULL) return INTERPRET_COMPILE_ERROR;

    push(OBJ_VAL(function));
    ObjClosure *closure = newClosure(function);
    pop();
    push(OBJ_VAL(closure));
    call(closure, 0);

    return run();
}