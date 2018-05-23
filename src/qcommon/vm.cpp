/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2013 Darklegion Development

This file is part of Tremulous.

Tremulous is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Tremulous is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Tremulous; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// vm.c -- virtual machine

//
// intermix code and data symbol table
//
// a dll has one imported function: VM_SystemCall
// and one exported function: Perform
//

#include "vm.h"

#include "sys/sys_shared.h"

#include "cmd.h"
#include "cvar.h"
#include "files.h"

int vm_debugLevel;
void VM_Debug(int level) { vm_debugLevel = level; }
vm_t *currentVM = nullptr;
vm_t *lastVM = nullptr;

void VM_Init(void)
{
    Cvar_Get("vm_cgame", "0", CVAR_ARCHIVE|CVAR_ROM);
    Cvar_Get("vm_game", "0", CVAR_ARCHIVE|CVAR_ROM);
    Cvar_Get("vm_ui", "0", CVAR_ARCHIVE|CVAR_ROM);
}

/*
============
VM_DllSyscall

Dlls will call this directly

 rcg010206 The horror; the horror.

  The syscall mechanism relies on stack manipulation to get its args.
   This is likely due to C's inability to pass "..." parameters to
   a function in one clean chunk. On PowerPC Linux, these parameters
   are not necessarily passed on the stack, so while (&arg[0] == arg)
   is true, (&arg[1] == 2nd function parameter) is not necessarily
   accurate, as arg's value might have been stored to the stack or
   other piece of scratch memory to give it a valid address, but the
   next parameter might still be sitting in a register.

  Quake's syscall system also assumes that the stack grows downward,
   and that any needed types can be squeezed, safely, into a signed int.

  This hack below copies all needed values for an argument to a
   array in memory, so that Quake can get the correct values. This can
   also be used on systems where the stack grows upwards, as the
   presumably standard and safe stdargs.h macros are used.

  As for having enough space in a signed int for your datatypes, well,
   it might be better to wait for DOOM 3 before you start porting.  :)

  The original code, while probably still inherently dangerous, seems
   to work well enough for the platforms it already works on. Rather
   than add the performance hit for those platforms, the original code
   is still in use there.

  For speed, we just grab 15 arguments, and don't worry about exactly
   how many the syscall actually needs; the extra is thrown away.

============
*/
__attribute__((no_sanitize_address))
static intptr_t QDECL VM_DllSyscall(intptr_t arg, ...)
{
#if !id386 || defined __clang__
    // rcg010206 - see commentary above
    intptr_t args[MAX_VMSYSCALL_ARGS];
    int i;
    va_list ap;

    args[0] = arg;

    va_start(ap, arg);
    for (i = 1; i < ARRAY_LEN(args); i++) args[i] = va_arg(ap, intptr_t);
    va_end(ap);

    return currentVM->systemCall(args);
#else  // original id code
    return currentVM->systemCall(&arg);
#endif
}

static vmHeader_t *VM_LoadQVM(vm_t *vm, bool alloc, bool unpure)
{
    int dataLength;
    int i;
    char filename[MAX_QPATH];
    union {
        vmHeader_t *h;
        void *v;
    } header;

    // load the image
    Com_sprintf(filename, sizeof(filename), "vm/%s.qvm", vm->name);
    Com_Printf("Loading vm file %s...\n", filename);

    FS_ReadFileDir(filename, vm->searchPath, unpure, &header.v);

    if (!header.h)
    {
        Com_Printf("Failed.\n");
        Com_Printf(S_COLOR_YELLOW "Warning: Couldn't open VM file %s\n", filename);
        return nullptr;
    }

    // show where the qvm was loaded from
    FS_Which(filename, vm->searchPath);

    if (LittleLong(header.h->vmMagic) == VM_MAGIC_VER2)
    {
        Com_Printf("...which has vmMagic VM_MAGIC_VER2\n");

        // byte swap the header
        for (i = 0; i < sizeof(vmHeader_t) / 4; i++)
        {
            ((int *)header.h)[i] = LittleLong(((int *)header.h)[i]);
        }

        // validate
        if (header.h->jtrgLength < 0 || header.h->bssLength < 0 || header.h->dataLength < 0 ||
            header.h->litLength < 0 || header.h->codeLength <= 0)
        {
            FS_FreeFile(header.v);
            Com_Printf(S_COLOR_YELLOW "Warning: %s has bad header\n", filename);
            return nullptr;
        }
    }
    else if (LittleLong(header.h->vmMagic) == VM_MAGIC)
    {
        // byte swap the header
        // sizeof( vmHeader_t ) - sizeof( int ) is the 1.32b vm header size
        for (i = 0; i < (sizeof(vmHeader_t) - sizeof(int)) / 4; i++)
        {
            ((int *)header.h)[i] = LittleLong(((int *)header.h)[i]);
        }

        // validate
        if (header.h->bssLength < 0 || header.h->dataLength < 0 || header.h->litLength < 0 || header.h->codeLength <= 0)
        {
            FS_FreeFile(header.v);
            Com_Printf(S_COLOR_YELLOW "Warning: %s has bad header\n", filename);
            return nullptr;
        }
    }
    else
    {
        FS_FreeFile(header.v);
        Com_Printf(S_COLOR_YELLOW "Warning: %s does not have a recognisable magic number in its header\n", filename);
        return nullptr;
    }

    // round up to next power of 2 so all data operations can be mask protected
    dataLength = header.h->dataLength + header.h->litLength + header.h->bssLength;
    for (i = 0; dataLength > (1 << i); i++)
        ;
    dataLength = 1 << i;

    if (alloc)
    {
        // allocate zero filled space for initialized and uninitialized data
        vm->dataBase = new unsigned char[dataLength]();
        vm->dataMask = dataLength - 1;
    }
    else
    {
        // clear the data, but make sure we're not clearing more than allocated
        if (vm->dataMask + 1 != dataLength)
        {
            FS_FreeFile(header.v);
            Com_Printf(S_COLOR_YELLOW "Warning: Data region size of %s not matching after VM_Restart()\n", filename);
            return nullptr;
        }

        ::memset(vm->dataBase, 0, dataLength);
    }

    // copy the intialized data
    ::memcpy(vm->dataBase, (byte *)header.h + header.h->dataOffset, header.h->dataLength + header.h->litLength);

    // byte swap the longs
    for (i = 0; i < header.h->dataLength; i += 4)
    {
        *(int *)(vm->dataBase + i) = LittleLong(*(int *)(vm->dataBase + i));
    }

    if (header.h->vmMagic == VM_MAGIC_VER2)
    {
        int previousNumJumpTableTargets = vm->numJumpTableTargets;

        header.h->jtrgLength &= ~0x03;

        vm->numJumpTableTargets = header.h->jtrgLength >> 2;
        Com_Printf("Loading %d jump table targets\n", vm->numJumpTableTargets);

        if (alloc)
        {
            vm->jumpTableTargets = new unsigned char[header.h->jtrgLength]();
        }
        else
        {
            if (vm->numJumpTableTargets != previousNumJumpTableTargets)
            {
                FS_FreeFile(header.v);

                Com_Printf(S_COLOR_YELLOW "Warning: Jump table size of %s not matching after VM_Restart()\n", filename);
                return nullptr;
            }

            ::memset(vm->jumpTableTargets, 0, header.h->jtrgLength);
        }

        ::memcpy(vm->jumpTableTargets,
            (byte *)header.h + header.h->dataOffset + header.h->dataLength + header.h->litLength, header.h->jtrgLength);

        // byte swap the longs
        for (i = 0; i < header.h->jtrgLength; i += 4)
        {
            *(int *)(vm->jumpTableTargets + i) = LittleLong(*(int *)(vm->jumpTableTargets + i));
        }
    }

    return header.h;
}

void vm_t::free()
{
    if (destroy) destroy(this);  // This will call VM_Destroy_Compiled()
    if (dllHandle) Sys_UnloadDll(dllHandle);
    if (dataBase) delete dataBase;
    if (instructionPointers) delete instructionPointers;
    this->clear();
}

void VM_BlockCopy(unsigned int dest, unsigned int src, size_t n)
{
    if ((dest & currentVM->dataMask) != dest || (src & currentVM->dataMask) != src ||
        ((dest + n) & currentVM->dataMask) != dest + n || ((src + n) & currentVM->dataMask) != src + n)
    {
        Com_Error(ERR_DROP, "OP_BLOCK_COPY out of range!");
    }

    ::memcpy(currentVM->dataBase + dest, currentVM->dataBase + src, n);
}

//=================================================================
bool NativeVM::init(const char *module, SystemCall systemCalls)
{
    void *startSearch = nullptr;
    int retval;

    Q_strncpyz(vm.name, module, sizeof(vm.name));

    do {
        char filename[MAX_OSPATH];
        retval = FS_FindVM(&startSearch, filename, sizeof(filename), module, true);

        if (retval != VMI_NATIVE) continue;

        vm.dllHandle = Sys_LoadGameDll(filename, &vm.entryPoint, VM_DllSyscall);
    } while (retval >= 0);

    if (!vm.dllHandle) return false;

    vm.systemCall = systemCalls;

    return true;
}

__attribute__((no_sanitize_address))
intptr_t NativeVM::Call(int callnum, ...)
{
    vm_t *oldVM = currentVM;
    currentVM = &vm;
    lastVM = &vm;

    vm.callLevel++;

    // if we have a dll loaded, call it directly
    assert(vm.entryPoint);

    int args[MAX_VMMAIN_ARGS - 1];
    va_list ap;

    va_start(ap, callnum);
    for (unsigned i = 0; i < ARRAY_LEN(args); i++) args[i] = va_arg(ap, int);
    va_end(ap);

    intptr_t r = vm.entryPoint(callnum, args[0], args[1], args[2]);
    vm.callLevel--;

    if (oldVM) currentVM = oldVM;

    return r;
}

void *NativeVM::ArgPtr(intptr_t intvalue)
{
    if (!intvalue) return nullptr;
    return (void *)(vm.dataBase + intvalue);
}

//=================================================================
bool BytecodeVM::init(const char *module, SystemCall systemCalls)
{
    vmHeader_t *header;
    void *startSearch = nullptr;
    int retval;

    Q_strncpyz(vm.name, module, sizeof(vm.name));

    do {
        char filename[MAX_OSPATH];

        retval = FS_FindVM(&startSearch, filename, sizeof(filename), module, false);
        if (retval != VMI_COMPILED) continue;

        vm.searchPath = startSearch;
        header = VM_LoadQVM(&vm, true, false);
        if (header) break;

        Q_strncpyz(vm.name, module, sizeof(vm.name));
    } while (retval >= 0);

    if (retval < 0)
        return false;

    vm.systemCall = systemCalls;
    vm.instructionCount = header->instructionCount;
    vm.instructionPointers = new intptr_t[vm.instructionCount]();
    vm.codeLength = header->codeLength;
    vm.compiled = false;
    VM_PrepareInterpreter(&vm, header);
    FS_FreeFile(header);
    vm.programStack = vm.dataMask + 1;
    vm.stackBottom = vm.programStack - PROGRAM_STACK_SIZE;

    return true;
}

__attribute__((no_sanitize_address))
intptr_t BytecodeVM::Call(int callnum, ...)
{
    vm_t *oldVM = currentVM;
    currentVM = &vm;
    lastVM = &vm;

    vm.callLevel++;

    struct {
        int callnum;
        int args[MAX_VMMAIN_ARGS - 1];
    } a;

    a.callnum = callnum;

    va_list ap;
    va_start(ap, callnum);
    for (unsigned i = 0; i < ARRAY_LEN(a.args); i++) a.args[i] = va_arg(ap, int);
    va_end(ap);

    intptr_t r = VM_CallInterpreted(&vm, &a.callnum);
    vm.callLevel--;

    if (oldVM) currentVM = oldVM;

    return r;
}

void *BytecodeVM::ArgPtr(intptr_t intvalue)
{
    if (!intvalue) return nullptr;
    return (void *)(vm.dataBase + (intvalue & vm.dataMask));
}

//=================================================================
#ifndef NO_VM_COMPILED
bool CompiledVM::init(const char *module, SystemCall systemCalls)
{
    vmHeader_t *header;
    void *startSearch = nullptr;
    int retval;

    Q_strncpyz(vm.name, module, sizeof(vm.name));

    do
    {
        char filename[MAX_OSPATH];

        retval = FS_FindVM(&startSearch, filename, sizeof(filename), module, false);
        if (retval != VMI_COMPILED) continue;

        vm.searchPath = startSearch;
        header = VM_LoadQVM(&vm, true, false);
        if (header) break;

        Q_strncpyz(vm.name, module, sizeof(vm.name));
    } while (retval >= 0);

    if (retval < 0)
        return false;

    vm.systemCall = systemCalls;
    vm.instructionCount = header->instructionCount;
    vm.instructionPointers = new intptr_t[vm.instructionCount]();
    vm.codeLength = header->codeLength;
    vm.compiled = true;
    VM_Compile(&vm, header);
    FS_FreeFile(header);
    vm.programStack = vm.dataMask + 1;
    vm.stackBottom = vm.programStack - PROGRAM_STACK_SIZE;

    return true;
}

__attribute__((no_sanitize_address))
intptr_t CompiledVM::Call(int callnum, ...)
{
    vm_t *oldVM = currentVM;
    currentVM = &vm;
    lastVM = &vm;

#if (id386 || idsparc) && !defined __clang__
    vm.callLevel++;
    intptr_t r = VM_CallCompiled(&vm, (int *)&callnum);
    vm.callLevel--;

    if (oldVM) currentVM = oldVM;

    return r;
#else
    vm.callLevel++;

    struct {
        int callnum;
        int args[MAX_VMMAIN_ARGS - 1];
    } a;

    a.callnum = callnum;

    va_list ap;
    va_start(ap, callnum);
    for (unsigned i = 0; i < ARRAY_LEN(a.args); i++) a.args[i] = va_arg(ap, int);
    va_end(ap);

    intptr_t r = VM_CallCompiled(&vm, &a.callnum);
    vm.callLevel--;

    if (oldVM) currentVM = oldVM;

    return r;
#endif
}

void *CompiledVM::ArgPtr(intptr_t intvalue)
{
    if (!intvalue) return nullptr;
    return (void *)(vm.dataBase + (intvalue & vm.dataMask));
}
#endif
