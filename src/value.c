#include <stdio.h>
#include <string.h>

#include "object.h"
#include "memory.h"
#include "value.h"

void initValueArray(ValueArray *array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void writeValueArray(ValueArray *array, Value value) {
    if (array->capacity < array->count + 1) {
        int old_capacity = array->capacity;
        array->capacity = GROW_CAPACITY(old_capacity);
        array->values = GROW_ARRAY(Value, array->values, old_capacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void freeValueArray(ValueArray *array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}

void printValue(Value value) {
    switch (value.type) {
        case VAL_BOOL:
            printf(AS_BOOL(value) ? "true" : "false");
            break;
        case VAL_NULL: printf("null"); break;
        case VAL_INT: printf("%d", AS_INT(value)); break;
        case VAL_FLOAT: printf("%g", AS_FLOAT(value)); break;
        case VAL_OBJ: printObject(value); break;
    }
}

bool valuesEqual(Value a, Value b) {
    if (a.type != b.type && !(IS_INT(a) || IS_FLOAT(a)) && !(IS_INT(b) || IS_FLOAT(b))) return false;
    switch (a.type) {
        case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NULL: return true;
        case VAL_INT:
            if (IS_INT(b)) {
                return AS_INT(a) == AS_INT(b);
            } else {
                return AS_INT(a) == AS_FLOAT(b);
            }
        case VAL_FLOAT:
            if (IS_INT(b)) {
                return AS_FLOAT(a) == AS_INT(b);
            } else {
                return AS_FLOAT(a) == AS_FLOAT(b);
            }
        case VAL_OBJ: return AS_OBJ(a) == AS_OBJ(b);
        default: return false;
    }
}