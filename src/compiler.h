#ifndef quasar_compiler_h
#define quasar_compiler_h

#include "object.h"
#include "chunk.h"
#include "vm.h"

ObjFunction *compile(const char *filename, const char *source);

#endif