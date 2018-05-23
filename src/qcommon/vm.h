#ifndef QCOMMON_VM_H
#define QCOMMON_VM_H 1

#include "sys/sys_shared.h"

#include "q_shared.h"
#include "qcommon.h"

/*
==============================================================

VIRTUAL MACHINE

==============================================================
*/

typedef enum {
    TRAP_MEMSET = 100,
    TRAP_MEMCPY,
    TRAP_STRNCPY,
    TRAP_SIN,
    TRAP_COS,
    TRAP_ATAN2,
    TRAP_SQRT,
    TRAP_MATRIXMULTIPLY,
    TRAP_ANGLEVECTORS,
    TRAP_PERPENDICULARVECTOR,
    TRAP_FLOOR,
    TRAP_CEIL,

    TRAP_TESTPRINTINT,
    TRAP_TESTPRINTFLOAT
} sharedTraps_t;

// Max number of arguments to pass from engine to vm's vmMain function.
// command number + 3 arguments
#define MAX_VMMAIN_ARGS 4

// Max number of arguments to pass from a vm to engine's syscall handler function for the vm.
// syscall number + 9 arguments
#define MAX_VMSYSCALL_ARGS 20

// don't change, this is hardcoded into x86 VMs, opStack protection relies
// on this
#define OPSTACK_SIZE 1024
#define OPSTACK_MASK (OPSTACK_SIZE - 1)

// don't change
// Hardcoded in q3asm a reserved at end of bss
#define PROGRAM_STACK_SIZE 0x10000
#define PROGRAM_STACK_MASK (PROGRAM_STACK_SIZE - 1)

enum opcode_t {
    OP_UNDEF,

    OP_IGNORE,

    OP_BREAK,

    OP_ENTER,
    OP_LEAVE,
    OP_CALL,
    OP_PUSH,
    OP_POP,

    OP_CONST,
    OP_LOCAL,

    OP_JUMP,

    //-------------------

    OP_EQ,
    OP_NE,

    OP_LTI,
    OP_LEI,
    OP_GTI,
    OP_GEI,

    OP_LTU,
    OP_LEU,
    OP_GTU,
    OP_GEU,

    OP_EQF,
    OP_NEF,

    OP_LTF,
    OP_LEF,
    OP_GTF,
    OP_GEF,

    //-------------------

    OP_LOAD1,
    OP_LOAD2,
    OP_LOAD4,
    OP_STORE1,
    OP_STORE2,
    OP_STORE4,  // *(stack[top-1]) = stack[top]
    OP_ARG,

    OP_BLOCK_COPY,

    //-------------------

    OP_SEX8,
    OP_SEX16,

    OP_NEGI,
    OP_ADD,
    OP_SUB,
    OP_DIVI,
    OP_DIVU,
    OP_MODI,
    OP_MODU,
    OP_MULI,
    OP_MULU,

    OP_BAND,
    OP_BOR,
    OP_BXOR,
    OP_BCOM,

    OP_LSH,
    OP_RSHI,
    OP_RSHU,

    OP_NEGF,
    OP_ADDF,
    OP_SUBF,
    OP_DIVF,
    OP_MULF,

    OP_CVIF,
    OP_CVFI
};

#define VM_OFFSET_PROGRAM_STACK 0
#define VM_OFFSET_SYSTEM_CALL 4

enum VMType { VMI_NATIVE, VMI_BYTECODE, VMI_COMPILED };

using SystemCalls = intptr_t (*)(intptr_t, ...);
using SystemCall = intptr_t (*)(intptr_t *);

struct vm_t {
    void clear() { ::memset(this, 0, sizeof(*this)); }
    void free();

    // DO NOT MOVE OR CHANGE THESE WITHOUT CHANGING THE VM_OFFSET_* DEFINES
    // USED BY THE ASM CODE
    int programStack;  // the vm may be recursively entered
    SystemCall systemCall;

    //------------------------------------

    char name[MAX_QPATH];
    void *searchPath;  // hint for FS_ReadFileDir()

    // for dynamic linked modules
    void *dllHandle;
    intptr_t(QDECL *entryPoint)(int callNum, ...);
    void (*destroy)(vm_t *self);

    // for interpreted modules
    bool currentlyInterpreting;

    bool compiled;
    byte *codeBase;
    int entryOfs;
    int codeLength;

    intptr_t *instructionPointers;
    int instructionCount;

    byte *dataBase;
    int dataMask;

    int stackBottom;  // if programStack < stackBottom, error

    int numSymbols;
    struct vmSymbol_s *symbols;

    int callLevel;  // counts recursive VM_Call
    int breakFunction;  // increment breakCount on function entry to this
    int breakCount;

    byte *jumpTableTargets;
    int numJumpTableTargets;
};

extern vm_t *currentVM;
extern vm_t *lastVM;

class VM {

public:
    VM() { vm.clear(); }
    virtual ~VM() { vm.free(); }
    void ClearCallLevel() { vm.callLevel = 0; }
    virtual bool init(const char*, SystemCall) = 0;
    virtual intptr_t Call(int callnum, ...) = 0;
    virtual void *ArgPtr(intptr_t intValue) = 0;

    vm_t vm;
};

class NativeVM : public VM {
public:
    bool init(const char *module, SystemCall systemCalls) override;
    intptr_t Call(int callnum, ...) override;
    void *ArgPtr(intptr_t intvalue) override;
};

class BytecodeVM : public VM {
public:
    bool init(const char *module, SystemCall systemCalls) override;
    intptr_t Call(int callnum, ...) override;
    void *ArgPtr(intptr_t intvalue) override;
};

#ifndef NO_VM_COMPILED
class CompiledVM : public VM {
public:
    bool init(const char *module, SystemCall systemCalls) override;
    intptr_t Call(int callnum, ...) override;
    void *ArgPtr(intptr_t intvalue) override;
};
#endif

class VMFactory {
public:
    // static uniq_ptr<vm_t> createVM(VMType type, const char* module, SystemCalls syscalls) {
    static VM *createVM(VMType type, const char *module, SystemCall syscalls)
    {
        VM *vm;

        switch (type)
        {
            case VMI_NATIVE:
                vm = new NativeVM;
                break;
            case VMI_COMPILED:
                vm = new CompiledVM;
                break;
            case VMI_BYTECODE:
                vm = new BytecodeVM;
                break;
        }

        if ( !vm->init(module, syscalls) )
        {
            delete vm;
            return nullptr;
        }

        return vm;
    }
};

extern int vm_debugLevel;

void VM_Compile(vm_t *vm, vmHeader_t *header);
int VM_CallCompiled(vm_t *vm, int *args);

void VM_PrepareInterpreter(vm_t *vm, vmHeader_t *header);
int VM_CallInterpreted(vm_t *vm, int *args);

void VM_LogSyscalls(int *args);

void VM_BlockCopy(unsigned int dest, unsigned int src, size_t n);

void VM_Init(void);

// module should be bare: "cgame", not "cgame.dll" or "vm/cgame.qvm"
void VM_Debug(int level);

static ID_INLINE float _vmf(intptr_t x)
{
    floatint_t fi;
    fi.i = (int)x;
    return fi.f;
}
#define VMF(x) _vmf(args[x])

#endif
