//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------


#ifndef _COMPILER_H_
#define _COMPILER_H_

class Stream;
class DataChunker;

#include "platform/platform.h"
#include "console/ast.h"
#include "console/codeBlock.h"

// Autogenerated, so we should only ever include from once place - here.
// (We can't stick include guards in it without patching bison.)
#ifndef _CMDGRAM_H_
#define _CMDGRAM_H_
#include "console/cmdgram.h"
#endif

namespace Compiler
{
    /// The opcodes for the TorqueScript VM.
    enum CompiledInstructions
    {
        OP_FUNC_DECL,
        OP_CREATE_OBJECT,
        OP_ADD_OBJECT,
        OP_END_OBJECT,
        // Added to fix the stack issue [7/9/2007 Black]
        OP_FINISH_OBJECT,

        OP_JMPIFFNOT,
        OP_JMPIFNOT,
        OP_JMPIFF,
        OP_JMPIF,
        OP_JMPIFNOT_NP,
        OP_JMPIF_NP,
        OP_JMP,
        OP_RETURN,
        OP_CMPEQ,
        OP_CMPGR,
        OP_CMPGE,
        OP_CMPLT,
        OP_CMPLE,
        OP_CMPNE,
        OP_XOR,
        OP_MOD,
        OP_BITAND,
        OP_BITOR,
        OP_NOT,
        OP_NOTF,
        OP_ONESCOMPLEMENT,

        OP_SHR,
        OP_SHL,
        OP_AND,
        OP_OR,

        OP_ADD,
        OP_SUB,
        OP_MUL,
        OP_DIV,
        OP_NEG,

        OP_SETCURVAR,
        OP_SETCURVAR_CREATE,
        OP_SETCURVAR_ARRAY,
        OP_SETCURVAR_ARRAY_CREATE,

        OP_LOADVAR_UINT,
        OP_LOADVAR_FLT,
        OP_LOADVAR_STR,

        OP_SAVEVAR_UINT,
        OP_SAVEVAR_FLT,
        OP_SAVEVAR_STR,

        OP_SETCUROBJECT,
        OP_SETCUROBJECT_NEW,

        OP_SETCURFIELD,
        OP_SETCURFIELD_ARRAY,

        OP_LOADFIELD_UINT,
        OP_LOADFIELD_FLT,
        OP_LOADFIELD_STR,

        OP_SAVEFIELD_UINT,
        OP_SAVEFIELD_FLT,
        OP_SAVEFIELD_STR,

        OP_STR_TO_UINT,
        OP_STR_TO_FLT,
        OP_STR_TO_NONE,
        OP_FLT_TO_UINT,
        OP_FLT_TO_STR,
        OP_FLT_TO_NONE,
        OP_UINT_TO_FLT,
        OP_UINT_TO_STR,
        OP_UINT_TO_NONE,

        OP_LOADIMMED_UINT,
        OP_LOADIMMED_FLT,
        OP_TAG_TO_STR,
        OP_LOADIMMED_STR,
        OP_LOADIMMED_IDENT,

        OP_CALLFUNC_RESOLVE,
        OP_CALLFUNC,

        OP_ADVANCE_STR,
        OP_ADVANCE_STR_APPENDCHAR,
        OP_ADVANCE_STR_COMMA,
        OP_ADVANCE_STR_NUL,
        OP_REWIND_STR,
        OP_TERMINATE_REWIND_STR,
        OP_COMPARE_STR,

        OP_PUSH,
        OP_PUSH_FRAME,

        OP_BREAK,

        OP_INVALID
    };

    //------------------------------------------------------------

    F64 consoleStringToNumber(const char* str, StringTableEntry file = 0, U32 line = 0);
    U32 precompileBlock(StmtNode* block, U32 loopCount);
    U32 compileBlock(StmtNode* block, U32* codeStream, U32 ip, U32 continuePoint, U32 breakPoint);

    //------------------------------------------------------------

    struct CompilerIdentTable
    {
        struct Entry
        {
            U32 offset;
            U32 ip;
            Entry* next;
            Entry* nextIdent;
        };
        Entry* list;
        void add(StringTableEntry ste, U32 ip);
        void reset();
        void write(Stream& st);
    };

    //------------------------------------------------------------

    struct CompilerStringTable
    {
        U32 totalLen;
        struct Entry
        {
            char* string;
            U32 start;
            U32 len;
            bool tag;
            Entry* next;
        };
        Entry* list;

        char buf[256];

        U32 add(const char* str, bool caseSens = true, bool tag = false);
        U32 addIntString(U32 value);
        U32 addFloatString(F64 value);
        void reset();
        char* build();
        void write(Stream& st);
    };

    //------------------------------------------------------------

    struct CompilerFloatTable
    {
        struct Entry
        {
            F64 val;
            Entry* next;
        };
        U32 count;
        Entry* list;

        U32 add(F64 value);
        void reset();
        F64* build();
        void write(Stream& st);
    };

    //------------------------------------------------------------

    inline StringTableEntry U32toSTE(U32 u)
    {
        return *((StringTableEntry*)&u);
    }

    extern U32(*STEtoU32)(StringTableEntry ste, U32 ip);

    U32 evalSTEtoU32(StringTableEntry ste, U32);
    U32 compileSTEtoU32(StringTableEntry ste, U32 ip);

    CompilerStringTable* getCurrentStringTable();
    CompilerStringTable& getGlobalStringTable();
    CompilerStringTable& getFunctionStringTable();

    void setCurrentStringTable(CompilerStringTable* cst);

    CompilerFloatTable* getCurrentFloatTable();
    CompilerFloatTable& getGlobalFloatTable();
    CompilerFloatTable& getFunctionFloatTable();

    void setCurrentFloatTable(CompilerFloatTable* cst);

    CompilerIdentTable& getIdentTable();

    void precompileIdent(StringTableEntry ident);

    CodeBlock* getBreakCodeBlock();
    void setBreakCodeBlock(CodeBlock* cb);

    /// Helper function to reset the float, string, and ident tables to a base
    /// starting state.
    void resetTables();

    void* consoleAlloc(U32 size);
    void consoleAllocReset();

    extern bool gSyntaxError;
};

#endif