/*
 * Copyright (C) 2010 France Telecom
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <assert.h>

# include "Code.h"
# include "Tokenizer.h"
# include "Types.h"
# include "Scripter.h"

bool ByteCode::s_compat = false;

ByteCode::ByteCode (StringTable * stringTable, IntTable * intTable) {
    m_bytecode = m_bytecodeStorage;
    for (int i = 0; i < MAX_REGISTERS; i++) {
        m_registery[i].m_index = i;
        m_registery[i].m_isUsed = false;
        m_registery[i].m_level = 0;
    }
    m_topRegister = -1;
    m_bunchMode = false;
    m_maxRegister = 0;
    m_maxLabels = 0;

    m_stringTable = stringTable;
    m_intTable = intTable;
}

void ByteCode::freeRegister (int i) {
    //printf ("11 freeRegister: %d (%d)\n", i, m_topRegister);
    if (i >= 0 && i < MAX_REGISTERS) {
        m_registery[i].m_isUsed = false;
        m_registery[i].m_level = 0;
        if (i == m_topRegister) {
            while (i >= 0 && m_registery[i].m_isUsed == false) {
                i--;
            }
            m_topRegister = i;
            //printf (" $$ freeRegister: top is %d\n", i);
        }
    } else {
        printf ("ERROR: freeRegister: bad index %d\n", i);
        ((char *)0)[0] = 0;
    }
    //printf ("22 freeRegister: %d (%d)\n", i, m_topRegister);
    //printRegisters ();
}

int ByteCode::getRegister (int level, bool bunchMode) {
    int i = getRegister2 (level, bunchMode);
    if (i > m_maxRegister) {
        m_maxRegister = i;
    }
    return i;
}

int ByteCode::getRegister2 (int level, bool bunchMode) {
    if (bunchMode) {
        int i = ++m_topRegister;
        m_registery[i].m_isUsed = true;
        m_registery[i].m_level = level;
        return i;
    }
    for (int i = 0; i < MAX_REGISTERS; i++) {
        if (m_registery[i].m_isUsed == false) {
            m_registery[i].m_isUsed = true;
            m_registery[i].m_level = level;
            //printf (" $$ getRegister: %d\n", i);
            if (i > m_topRegister) { 
                m_topRegister = i;
                //printf (" $$ getRegister: top is %d\n", i);
            }
            //printRegisters ();
            return i;
        }
    }
    fprintf (stderr, "Error: no more free register\n");
    return (-1);
}

void ByteCode::printRegisters () {
    printf ("[[");
    for (int i = 0; i <= m_topRegister; i ++) {
        printf ("%c", m_registery[i].m_isUsed ? '1' : '0');
    }
    printf ("]]\n");
}

static void exchangeBytes (unsigned char * s) {
    char t = s[0];
    s[0] = s[3];
    s[3] = t;
    t = s[1];
    s[1] = s[2];
    s[2] = t;
}

int ByteCode::getLabel () {
    if (m_maxLabels >= MAX_LABELS) {
        fprintf (stderr, "Max labels (%d) per Script reached !\n",MAX_LABELS);
        exit (-1);
    }
    int index = m_maxLabels++;
    m_labels[index].m_maxRefs = 0;
    m_labels[index].m_offset = -1;
    return index;
}

void ByteCode::setLabel (int labelIndex) {
    if (s_compat) {
        add (ByteCode::ASM_LABEL_COMPAT, labelIndex);
        return;
    }
    int offset = m_bytecode - m_bytecodeStorage;
    Label& label = m_labels[labelIndex];
    label.m_offset = offset;
    // Correct offset of jumps referencing this label
    for (int i = 0; i<label.m_maxRefs; i++) {
        int refOffset = label.m_refs[i];
        m_bytecodeStorage[refOffset] = (unsigned char) ((offset & 0xFF00) >> 8);
        m_bytecodeStorage[refOffset+1] = (unsigned char) (offset & 0xFF);
    }
}

void ByteCode::addByte (int i) {
    *m_bytecode++ = (unsigned char)(i & 0xFF);
}

void ByteCode::addInt (int i) {
    if (s_compat) {
	    unsigned char * s = (unsigned char *)&i;
	    exchangeBytes (s);
	    for (int i = 0; i < 4; i++) {
	        addByte (*s++);
        }
        return;
    }
    addByte (m_intTable->findOrAdd (i));
}

void ByteCode::addFloat (float f) {
    addInt (int(f*65536));
}

void ByteCode::addString (char * s) {
    if (s_compat) {
        while (*s != 0) {
            addByte (*s++);
        }
        addByte (0);
        return;
    }
    addByte (m_stringTable->findOrAdd (s));
}

void ByteCode::add (int opcode) {
    addByte (opcode);
}

void ByteCode::add (int opcode, int reg1) {
    add (opcode);
    addByte (reg1);
}

void ByteCode::add (int opcode, int reg1, int reg2) {
    add (opcode, reg1);
    addByte (reg2);
}

void ByteCode::add (int opcode, int reg1, int reg2, int reg3) {
    add (opcode, reg1, reg2);
    addByte (reg3);
}

void ByteCode::add (int opcode, int reg1, int reg2, int reg3, int reg4) {
    add (opcode, reg1, reg2, reg3);
    addByte (reg4);
}

void ByteCode::add (int opcode, int reg1, char * s) {
    add (opcode, reg1);
    addString (s);
}

void ByteCode::addJump (int labelIndex) {
    if (s_compat) {
        add (ASM_JUMP_COMPAT, labelIndex);
        return;
    }
    add (ASM_JUMP);
    addLabelOffset (m_labels[labelIndex]);
}

void ByteCode::addJumpZero (int reg, int labelIndex, bool negate) {
    if (s_compat) {
        assert (negate == false);
        add (ASM_JUMP_ZERO_COMPAT, reg, labelIndex);
        return;
    }
    add (negate ? ASM_JUMP_NZERO : ASM_JUMP_ZERO, reg);
    addLabelOffset (m_labels[labelIndex]);
}

void ByteCode::addLabelOffset (Label& label) {
    int offset = label.m_offset;
    if (offset >= 0) { // Label offset is defined
        add ((offset & 0xFF00) >> 8, offset & 0xFF);
    } else { // Label offset is still unknown, register this jump
        if (label.m_maxRefs >= MAX_REFS) {
            fprintf (stderr, "Max jumps per label (%d) reached !\n", MAX_REFS);
            exit (-1);
        }
        label.m_refs[label.m_maxRefs++] = m_bytecode - m_bytecodeStorage;
        add (0, 0); // reserve 2 bytes for future offset value
    }
}

unsigned char * ByteCode::getCode (int & len) {
    len = m_bytecode - m_bytecodeStorage;
    if (len <= 0) {
        return NULL;
    }
    char * tmp = (char *)malloc (len);
    memcpy (tmp, m_bytecodeStorage, len);
    return ((unsigned char *)tmp);
}

int ByteCode::setBreakLabel (int label)  {
  int oldLabel = m_breakLabel;
  m_breakLabel = label;
  return oldLabel;
}

int ByteCode::getBreakLabel () {
  return m_breakLabel;
}

int ByteCode::setContinueLabel (int label)  {
  int oldLabel = m_continueLabel;
  m_continueLabel = label;
  return oldLabel;
}

int ByteCode::getContinueLabel () {
  return m_continueLabel;
}

void printInt (unsigned char * data) {
    exchangeBytes (data);
    printf ("%d ", *((int*)data));
    exchangeBytes (data);
}

void printFloat (unsigned char * data) {
    exchangeBytes (data);
    int tmp = * ((int *)data);
    printf ("%g ", (tmp/65536.0f));
    exchangeBytes (data);
}

void printByte (unsigned char * data) {
    printf ("%d ", data[0]);
}

void ByteCode::dump (unsigned char * data, int len) {
    unsigned char * end = data + len;
    while (data < end) {
        switch (*data) {
        case ASM_NOP: printf ("    ASM_NOP\n"); data++; break;
        case ASM_LOAD_REG_INT: 
            printf ("    ASM_LOAD_REG_INT "); data++; 
            printByte (data); data += 1;
            printInt (data); data += 4;
            break;
        case ASM_LOAD_REG_FLT: 
            printf ("    ASM_LOAD_REG_FLT "); data++; 
            printByte (data); data += 1;
            printFloat (data); data += 4;
            break;
        case ASM_LOAD_REG_STR: 
            printf ("    ASM_LOAD_REG_STR "); data++; 
            printByte (data); data += 1;
            printf ("'");
            while (*data) {
                printf ("%c", *data); data += 1;
            }
            printf ("'");
            data ++;
            break;
        case ASM_MOVE_REG_REG: 
            printf ("    ASM_MOV_REG_REG "); data++; 
            printByte (data); data += 1;
            printByte (data); data += 1;
            break;

        case ASM_FIELD_PUSH:
            printf ("    ASM_FIELD_PUSH "); data++; 
            break;
        case ASM_FIELD_POP:
            printf ("    ASM_FIELD_POP "); data++; 
            break;
        case ASM_FIELD_USE_INT:
            printf ("    ASM_FIELD_USE_INT "); data++; 
            printByte (data); data += 1;
            break;
        case ASM_FIELD_IDX_REG:
            printf ("    ASM_FIELD_IDX_REG "); data++; 
            printByte (data); data += 1;
            break;
        case ASM_FIELD_SET_INT_REG:
            printf ("    ASM_FIELD_SET_INT_REG "); data++; 
            printByte (data); data += 1;
            printByte (data); data += 1;
            break;
        case ASM_FIELD_GET_INT_REG:
            printf ("    ASM_FIELD_GET_INT_REG "); data++; 
            printByte (data); data += 1;
            printByte (data); data += 1;
            break;

        case ASM_ADD_REG_REG: 
            printf ("    ASM_ADD_REG_REG "); data++; 
            printByte (data); data += 1;
            printByte (data); data += 1;
            break;
        case ASM_SUB_REG_REG: 
            printf ("    ASM_SUB_REG_REG "); data++; 
            printByte (data); data += 1;
            printByte (data); data += 1;
            break;
        case ASM_MUL_REG_REG: 
            printf ("    ASM_MUL_REG_REG "); data++; 
            printByte (data); data += 1;
            printByte (data); data += 1;
            break;
        case ASM_DIV_REG_REG: 
            printf ("    ASM_DIV_REG_REG "); data++; 
            printByte (data); data += 1;
            printByte (data); data += 1;
            break;
        case ASM_MOD_REG_REG: 
            printf ("    ASM_MOD_REG_REG "); data++; 
            printByte (data); data += 1;
            printByte (data); data += 1;
            break;
        case ASM_TEST_EQU: 
            printf ("    ASM_TEST_EQU "); data++; 
            printByte (data); data += 1;
            printByte (data); data += 1;
            break;
        case ASM_TEST_NEQ:
            printf ("    ASM_TEST_NEQ "); data++; 
            printByte (data); data += 1;
            printByte (data); data += 1;
            break;
        case ASM_TEST_LES:
            printf ("    ASM_TEST_LES "); data++; 
            printByte (data); data += 1;
            printByte (data); data += 1;
            break;
        case ASM_TEST_LEE:
            printf ("    ASM_TEST_LEE "); data++; 
            printByte (data); data += 1;
            printByte (data); data += 1;
            break;
        case ASM_TEST_GRT:
            printf ("    ASM_TEST_GRT "); data++; 
            printByte (data); data += 1;
            printByte (data); data += 1;
            break;
        case ASM_TEST_GRE:
            printf ("    ASM_TEST_GRE "); data++; 
            printByte (data); data += 1;
            printByte (data); data += 1;
            break;
        case ASM_TEST_AND:
            printf ("    ASM_TEST_AND "); data++; 
            printByte (data); data += 1;
            printByte (data); data += 1;
            break;
        case ASM_TEST_OR:
            printf ("    ASM_TEST_OR "); data++; 
            printByte (data); data += 1;
            printByte (data); data += 1;
            break;
        case ASM_BIT_AND:
            printf ("    ASM_BIT_AND "); data++; 
            printByte (data); data += 1;
            printByte (data); data += 1;
            break;
        case ASM_BIT_OR:
            printf ("    ASM_BIT_OR "); data++; 
            printByte (data); data += 1;
            printByte (data); data += 1;
            break;
        case ASM_BIT_XOR:
            printf ("    ASM_BIT_XOR "); data++; 
            printByte (data); data += 1;
            printByte (data); data += 1;
            break;
        case ASM_BIT_INV:
            printf ("    ASM_BIT_INV "); data++; 
            printByte (data); data += 1;
            break;
        case ASM_BIT_LS:
            printf ("    ASM_BIT_LS "); data++; 
            printByte (data); data += 1;
            printByte (data); data += 1;
            break;
        case ASM_BIT_RS:
            printf ("    ASM_BIT_RS "); data++; 
            printByte (data); data += 1;
            printByte (data); data += 1;
            break;
        case ASM_BIT_RRS:
            printf ("    ASM_BIT_RRS "); data++; 
            printByte (data); data += 1;
            printByte (data); data += 1;
            break;
        case ASM_JUMP_COMPAT:
            printf ("    ASM_JMP "); data++;
            printByte (data); data += 1;
            break;
        case ASM_JUMP_ZERO_COMPAT:
            printf ("    ASM_JMP_ZERO "); data++; 
            printByte (data); data += 1;
            printByte (data); data += 1;
            break;
        case ASM_LABEL_COMPAT:
            printf ("    ASM_LABEL "); data++;
            printByte (data); data += 1;
            break;
        case ASM_JUMP:
            printf ("    ASM_JMP "); data++;
            printByte (data); data += 1;
            printByte (data); data += 1;
            break;
        case ASM_JUMP_ZERO:
            printf ("    ASM_JMP_ZERO "); data++; 
            printByte (data); data += 1;
            printByte (data); data += 1;
            printByte (data); data += 1;
            break;
        case ASM_JUMP_NZERO:
            printf ("    ASM_JMP_NZERO "); data++;
            printByte (data); data += 1;
            printByte (data); data += 1;
            printByte (data); data += 1;
            break;
        case ASM_EXT_CALL:
            printf ("    ASM_EXT_CALL "); data++; 
            printByte (data); data += 1;
            printByte (data); data += 1;
            printByte (data); data += 1;
            printByte (data); data += 1;
            break;
        case ASM_INT_CALL:
            printf ("    ASM_INT_CALL "); data++; 
            printByte (data); data += 1;
            printByte (data); data += 1;
            break;
        case ASM_RETURN:
            printf ("    ASM_RETURN "); data++; 
            printByte (data); data += 1;
            break;
        default:
            printf ("Unknwon asm code: %d\n", *data);
            return;
        }
        printf ("\n");
    }
}

void ByteCode::generate (char * n) {
    FILE * fp = fopen (n, "wb");
    if (fp == NULL) {
        fprintf (stderr, "Error: cannot open %s for writing\n", n);
        return;
    }
    fprintf (fp, "// File automatically generated, do not edit\n");
    fprintf (fp, "\npackage memoplayer;\n\n");
    fprintf (fp, "public class ByteCode {\n");
    fprintf (fp, "    final static int ASM_ERROR = %d;\n", ASM_ERROR);
    fprintf (fp, "    final static int ASM_NOP = %d;\n", ASM_NOP);
    //fprintf (fp, "    final static int ASM_ALLOC = %d;\n", ASM_ALLOC);
    //fprintf (fp, "    final static int ASM_FREE = %d;\n", ASM_FREE);
    fprintf (fp, "    final static int ASM_LABEL_COMPAT = %d;\n", ASM_LABEL_COMPAT);
    fprintf (fp, "    final static int ASM_JUMP_COMPAT = %d;\n", ASM_JUMP_COMPAT);
    fprintf (fp, "    final static int ASM_JUMP_ZERO_COMPAT = %d;\n", ASM_JUMP_ZERO_COMPAT);
    fprintf (fp, "    final static int ASM_JUMP = %d;\n", ASM_JUMP);
    fprintf (fp, "    final static int ASM_JUMP_ZERO = %d;\n", ASM_JUMP_ZERO);
    fprintf (fp, "    final static int ASM_JUMP_NZERO = %d;\n", ASM_JUMP_NZERO);
    fprintf (fp, "    final static int ASM_EXT_CALL = %d;\n", ASM_EXT_CALL);
    fprintf (fp, "    final static int ASM_INT_CALL = %d;\n", ASM_INT_CALL);
    fprintf (fp, "    final static int ASM_RETURN = %d;\n", ASM_RETURN);
    fprintf (fp, "    final static int ASM_LOAD_REG_INT = %d;\n", ASM_LOAD_REG_INT);
    fprintf (fp, "    final static int ASM_LOAD_REG_FLT = %d;\n", ASM_LOAD_REG_FLT);
    fprintf (fp, "    final static int ASM_LOAD_REG_STR = %d;\n", ASM_LOAD_REG_STR);
    //fprintf (fp, "    final static int ASM_LOAD_REG_REL = %d;\n", ASM_LOAD_REG_REL);
    fprintf (fp, "    final static int ASM_MOVE_REG_REG = %d;\n", ASM_MOVE_REG_REG);
    //fprintf (fp, "    final static int ASM_SAVE_REG_REL = %d;\n", ASM_SAVE_REG_REL);
    //     fprintf (fp, "    final static int ASM_INC_REG = %d;\n", ASM_INC_REG);
    //     fprintf (fp, "    final static int ASM_DEC_REG = %d;\n", ASM_DEC_REG);
    fprintf (fp, "    final static int ASM_ADD_REG_REG = %d;\n", ASM_ADD_REG_REG);
    fprintf (fp, "    final static int ASM_SUB_REG_REG = %d;\n", ASM_SUB_REG_REG);
    fprintf (fp, "    final static int ASM_MUL_REG_REG = %d;\n", ASM_MUL_REG_REG);
    fprintf (fp, "    final static int ASM_DIV_REG_REG = %d;\n", ASM_DIV_REG_REG);
    fprintf (fp, "    final static int ASM_MOD_REG_REG = %d;\n", ASM_MOD_REG_REG);
    fprintf (fp, "    final static int ASM_TEST_EQU = %d;\n", ASM_TEST_EQU);
    fprintf (fp, "    final static int ASM_TEST_NEQ = %d;\n", ASM_TEST_NEQ);
    fprintf (fp, "    final static int ASM_TEST_GRT = %d;\n", ASM_TEST_GRT);
    fprintf (fp, "    final static int ASM_TEST_GRE = %d;\n", ASM_TEST_GRE);
    fprintf (fp, "    final static int ASM_TEST_LES = %d;\n", ASM_TEST_LES);
    fprintf (fp, "    final static int ASM_TEST_LEE = %d;\n", ASM_TEST_LEE);
    fprintf (fp, "    final static int ASM_TEST_AND = %d;\n", ASM_TEST_AND);
    fprintf (fp, "    final static int ASM_TEST_OR = %d;\n", ASM_TEST_OR);
    fprintf (fp, "    final static int ASM_BIT_AND = %d;\n", ASM_BIT_AND);
    fprintf (fp, "    final static int ASM_BIT_OR = %d;\n", ASM_BIT_OR);
    fprintf (fp, "    final static int ASM_BIT_XOR = %d;\n", ASM_BIT_XOR);
    fprintf (fp, "    final static int ASM_BIT_INV = %d;\n", ASM_BIT_INV);
    fprintf (fp, "    final static int ASM_BIT_LS = %d;\n", ASM_BIT_LS);
    fprintf (fp, "    final static int ASM_BIT_RS = %d;\n", ASM_BIT_RS);
    fprintf (fp, "    final static int ASM_BIT_RRS = %d;\n", ASM_BIT_RRS);
    //fprintf (fp, "    final static int ASM_RET = %d;\n", ASM_RET);
    //fprintf (fp, "    final static int ASM_PUSH_BASE = %d;\n", ASM_PUSH_BASE);
    //fprintf (fp, "    final static int ASM_POP_BASE = %d;\n", ASM_POP_BASE);

    fprintf (fp, "    final static int ASM_FIELD_PUSH = %d;\n", ASM_FIELD_PUSH);
    fprintf (fp, "    final static int ASM_FIELD_POP = %d;\n", ASM_FIELD_POP);
    fprintf (fp, "    final static int ASM_FIELD_USE_INT = %d;\n", ASM_FIELD_USE_INT);
    fprintf (fp, "    final static int ASM_FIELD_IDX_REG = %d;\n", ASM_FIELD_IDX_REG);
    fprintf (fp, "    final static int ASM_FIELD_SET_INT_REG = %d;\n", ASM_FIELD_SET_INT_REG);
    fprintf (fp, "    final static int ASM_FIELD_GET_INT_REG = %d;\n", ASM_FIELD_GET_INT_REG);
    fprintf (fp, "}\n");
}


bool debugOutput = true;

# define PRINTF if (debugOutput) printf

Code::Code (char value) {
    init ();
    m_type = CODE_CHAR;
    m_ival = value;
}

Code::Code (int value) {
    init ();
    m_type = CODE_INT;
    m_ival = value;
}

Code::Code (float value) {
    init ();
    m_type = CODE_FLOAT;
    m_fval = value;
}

Code::Code (char * name, bool isString) {
    init ();
    if (isString) {
        m_type = CODE_STRING;
        m_name = name;
    } else {
        m_type = CODE_NAME;
        m_name = strdup (name);
    }
}

// Code::Code (char * name, char * className, bool isVar) {
//     init ();
//     m_type = isVar ? CODE_STRING : CODE_NAME;
//     m_name = strdup (name);
//     if (className) {
//         m_class = strdup (className);
//     }
// }

Code::Code (int op, Code * left, Code * right, Code * third) {
    init ();
    m_type = op;
    m_first = left;
    m_second = right;
    m_third = third;
}

Code::~Code () {
    if (m_name && m_type != CODE_STRING) {
        free (m_name);
    }
    if (m_class) {
        free (m_class);
    }
}


Code * Code::cloneInvertAccess () {
    int t = m_type;
    if (t == CODE_SET_VAR) {
        t = CODE_GET_VAR;
    } else if (t == CODE_GET_VAR) {
        t = CODE_SET_VAR;
    } else if (t == CODE_SET_FIELD) {
        t = CODE_GET_FIELD;
    } else if (t == CODE_SET_FIELD) {
        t = CODE_GET_FIELD;
    }
    Code * tmp = new Code (t, NULL, NULL, NULL);
    tmp->m_ival = m_ival;
    tmp->m_fval = m_fval;
    tmp->m_name = m_name;

    if (m_first) {
        tmp->m_first = m_first->cloneInvertAccess ();
    }
    if (m_second) {
        tmp->m_second = m_second->cloneInvertAccess ();
    }
    if (m_third) {
        tmp->m_third= m_third->cloneInvertAccess ();
    }
    if (m_next) {
        tmp->m_next = m_next ->cloneInvertAccess (); 
    }
    return (tmp);
}

void Code::destroy () {
    if (m_first) {
        m_first->destroy ();
    }
    if (m_second) {
        m_second->destroy ();
    }
    if (m_third) {
        m_third->destroy ();
    }
    if (m_next) {
        m_next->destroy ();
    }
    delete this;
}

bool Code::isTerm () {
    return (m_type == CODE_CHAR || m_type == CODE_INT || m_type == CODE_NAME);
}

void printSpaces (int n, const char * before = NULL, const char * after = NULL) {
    if (before) {
        printf ("%s", before);
    }
    while (n-->0) {
        printf (" ");
    }
    if (after) {
        printf ("%s", after);
    }
}

void Code::printAux (const char * left, const char * middle, const char * right, int level) {
    if (m_first != NULL) {
        if (level == 0) {
            printf ("%s", left);
        }
        m_first->print ();
    } else { 
        printf ("NULL");
    }
    printf ("%s", middle);
    if (m_second != NULL) {
        m_second->print ();
        if (level == 0) {
            printf ("%s", right);
        }
    } else { 
        printf ("NULL");
    }
}

/*void Code::dump () {
    printf ("// type  : %d\n", m_type);
    printf ("// value : %d\n", m_ival );
    printf ("// name  : %s\n", m_name ? m_name : "NULL");
    printf ("// first : %p\n", m_first ? m_first : 0);
    printf ("// second: %p\n", m_second ? m_second : 0);
    printf ("// third : %p\n", m_third ? m_third : 0);
    printf ("// next  : %p\n", m_next ? m_next : 0);
    }*/


void Code::print (int indentLevel) {
    switch (m_type) {
    case CODE_NOP:
        break;
    case CODE_INT: 
        printf ("%d", m_ival); break;
    case CODE_FLOAT: 
        printf ("%g", m_fval); break;
    case CODE_CHAR:
        if (m_ival == '\n') {
            printf ("'\\n'");
        } else if (m_ival == '\t') {
            printf ("'\\t'");
        } else if (m_ival == '\0') {
            printf ("'\\0'");
        } else {
            printf ("'%c'", m_ival); 
        }
        break;
    case CODE_STRING: 
        printf ("\"%s\"", m_name); break;
    case CODE_NAME: 
        printf ("%s", m_name); break;
    case CODE_PARAM:
        m_first->print ();
        if (m_next) {
            printf (", ");
            m_next->print (0);
        }
        break;
    case CODE_CALL_STATIC:
        printSpaces (indentLevel);
        m_first->print (0);
        printf ("."); 
        m_second->print (0);
        printf (" (");
        if (m_third) { m_third->print (); }
        printf (");\n");
        break;
    case CODE_CALL_FUNCTION:
        printSpaces (indentLevel);
        m_first->print (0);
        printf (" (");
        if (m_second) { m_second->print (); }
        printf (");\n");
        break;
    case CODE_RETURN:
        printSpaces (indentLevel);
        printf (" (");
        if (m_first) { m_first->print (); }
        printf (");\n");
        break; 
    case CODE_NEW_VAR:
        printSpaces (indentLevel);
        printf ("var %s", m_first ? m_first->m_name : "null");
        if (m_second) {
            printf (" = ");
            m_second->print ();
        }
        printf (";\n"); break;
        break;
    case CODE_SET_VAR:
    case CODE_GET_VAR:
        if (m_first) { m_first->print (0); }
        break;
    case CODE_ASSIGN:
    case CODE_SELFPLUS: 
    case CODE_SELFMINUS: 
    case CODE_SELFDIV: 
    case CODE_SELFMULT: 
        printSpaces (indentLevel);
        if (m_first) {
            m_first->print (0);
        } else {
            printf ("ERROR: no lvalue defined!\n");
        }
        if (m_second) {
            switch (m_type) {
            case CODE_ASSIGN:  printf (" = "); break;
            case CODE_SELFPLUS:  printf (" += "); break;
            case CODE_SELFMINUS:  printf (" -= "); break;
            case CODE_SELFMULT:  printf (" *= "); break;
            case CODE_SELFDIV:  printf (" /= "); break;
            default: printf (" ??? "); break;
            }
            m_second->print (0);
        }
        printf (";\n"); break;
    case CODE_SET_FIELD: 
        printf (".%d<-", m_first->m_ival); break;
    case CODE_GET_FIELD: 
        printf (".%d->", m_first->m_ival); break;
    case CODE_USE_FIELD: 
        printf ("*(%d)", m_first->m_ival); 
        if (m_next) {
            m_next->print (0);
        } else {
            printf ("ERROR");
        }
        break;
    case CODE_USE_IDX_FIELD: 
        printf ("[");
        m_first->print (0); 
        printf ("]");
        break;
    case CODE_PLUS: 
        printAux ("(", " + ", ")", indentLevel); break;
    case CODE_MINUS:
        printAux ("(", " - ", ")", indentLevel); break;
    case CODE_MULT: 
        printAux ("(", " * ", ")", indentLevel); break;
    case CODE_DIV: 
        printAux ("(", " / ", ")", indentLevel); break;
    case CODE_MODULO: 
        printAux ("(", " %% ", ")", indentLevel); break;
    case CODE_EQUAL: 
        printAux ("(", " == ", ")", indentLevel); break;
    case CODE_NOTEQUAL: 
        printAux ("(", " != ", ")", indentLevel); break;
    case CODE_GREATER: 
        printAux ("(", " > ", ")", indentLevel); break;
    case CODE_GREATEQ: 
        printAux ("(", " >= ", ")", indentLevel); break;
    case CODE_LESSER: 
        printAux ("(", " < ", ")", indentLevel); break;
    case CODE_LESSEQ: 
        printAux ("(", " <= ", ")", indentLevel); break;
    case CODE_LOG_AND: 
        printAux ("(", " && ", ")", indentLevel); break;
    case CODE_LOG_OR: 
        printAux ("(", " || ", ")", indentLevel); break;
    case CODE_BIT_AND:
        printAux ("(", " & ", ")", indentLevel); break;
    case CODE_BIT_OR:
        printAux ("(", " | ", ")", indentLevel); break;
    case CODE_BIT_XOR:
        printAux ("(", " ^ ", ")", indentLevel); break;
    case CODE_BIT_INV:
        printSpaces (indentLevel);
        printf ("~ (");
        if (m_first) { m_first->print (); }
        printf (")\n");
        break;
    case CODE_BIT_LSHIFT:
        printAux ("(", " << ", ")", indentLevel); break;
    case CODE_BIT_RSHIFT:
        printAux ("(", " >> ", ")", indentLevel); break;
    case CODE_BIT_RRSHIFT:
        printAux ("(", " >>> ", ")", indentLevel); break;
    case CODE_BLOCK: 
        printf (" {\n" );
        if (m_first) {
            m_first->printAll (indentLevel+4);
        }
        printSpaces (indentLevel, NULL, "}");
        break;
    case CODE_IF: 
        printSpaces (indentLevel, NULL, "if ");
        if (m_first != NULL) {
            m_first->print ();
        }
        if (m_second) {
            if (m_second->m_type == CODE_BLOCK) {
                m_second->printAll (indentLevel);
            } else {
                printSpaces (indentLevel+4, " {\n");
                m_second->print ();
                printSpaces (indentLevel, NULL, "}");
            }
        }
        if (m_third) {
            printf (" else" );
            if (m_third->m_type == CODE_BLOCK) {
                m_third->printAll (indentLevel);
            } else {
                printSpaces (indentLevel+4, " {\n");
                m_third->print ();
                printSpaces (indentLevel, NULL, "}");
            }
        }
        printf ("\n");
        break;
    case CODE_WHILE: 
        printSpaces (indentLevel, NULL, "while ");
        if (m_first != NULL) {
            m_first->print ();
        }
        if (m_second) {
            if (m_second->m_type == CODE_BLOCK) {
                m_second->printAll (indentLevel);
            } else {
                printSpaces (indentLevel+4, " {\n");
                m_second->print ();
                printSpaces (indentLevel, NULL, "}");
            }
        }
        printf ("\n");
        break;
    case CODE_FOR: 
        printSpaces (indentLevel, NULL, "for (; ");
        if (m_first != NULL) {
            m_first->print ();
        }
        printf ("; ");
        if (m_third != NULL) {
          m_third->print ();
        } 
        printf (")");
        if (m_second) {
            if (m_second->m_type == CODE_BLOCK) {
                m_second->printAll (indentLevel);
            } else {
                printSpaces (indentLevel+4, " {\n");
                m_second->print ();
                printSpaces (indentLevel, NULL, "}");
            }
        }
        printf ("\n");
        break;
    case CODE_CONTINUE:
        printSpaces (indentLevel, NULL, "continue;\n");
        break;
    case CODE_BREAK:
        printSpaces (indentLevel, NULL, "break;\n");
        break;
    case CODE_TERNARY_COMP:
        printf ("( ");
        if (m_first) { m_first->print (); }
        printf (" ? ");
        if (m_second) { m_second->print (); }
        printf (" : ");
        if (m_third) { m_third->print (); }
        printf (")");
        break;

    case CODE_ERROR:
    default:  
        printf (" ERROR %d\n", m_type);
    }  
}

void Code::printAll (int indentLevel) {
    print (indentLevel);
    if (m_next) {
        m_next->printAll (indentLevel);
    }   
}

void Code::init () {
    m_type = CODE_NOP;
    m_ival = 0;
    m_name = NULL;
    m_class = NULL;
    m_next = m_first = m_second = m_third = NULL;
}

void Code::setSecondToLast (Code * code) {
    if (m_next) {
        m_next->setSecondToLast (code);
    } else {
        m_second = code;
    }
}
 
void Code::append (Code * code) {
    if (m_next) {
        m_next->append (code);
    } else {
        m_next = code;
    }
}

Code * Code::getNext () { 
    return (m_next); 
}

int Code::getLength () {
    if (m_next) {
        return (1+m_next->getLength ());
    } else {
        return (1);
    }
}

bool Code::generateAll (ByteCode * bc, Function * f) {
    generate (bc, f, -1);
    if (m_next) {
        m_next->generateAll (bc, f);
    } 
    return (true);
}

void Code::generateBinary (int opcode, ByteCode * bc, Function * f, int reg) {
    //fprintf (stderr, "genBin: %d: %d\n", opcode, reg);
    assert (reg>=0);
    m_first->generate (bc, f, reg);
    int reg2 = bc->getRegister(f->m_blockLevel);
    m_second->generate (bc, f, reg2);
    bc->add (opcode, reg, reg2);
    bc->freeRegister (reg2);
}

void Code::generate (ByteCode * bc, Function * f, int reg) {
    int reg1, reg2, reg3, nbParams;
    int breakLabel, continueLabel;
    int cnt1, cnt2;
    Var * var;
    Code * tmp;
    Code * defaultLabel = NULL;
    switch (m_type) {
    case CODE_THEEND:
        bc->add (ByteCode::ASM_ENDOFCODE);
        break;
    case CODE_NOP:
        bc->add (ByteCode::ASM_NOP);
        break;
    case CODE_INT: 
        assert (reg>=0);
        bc->add (ByteCode::ASM_LOAD_REG_INT, reg);
        bc->addInt (m_ival);
        break;
    case CODE_FLOAT:
        assert (reg>=0);
        bc->add (ByteCode::ASM_LOAD_REG_FLT, reg);
        bc->addFloat (m_fval);
        break;
    case CODE_STRING: 
        assert (reg>=0);
        bc->add (ByteCode::ASM_LOAD_REG_STR, reg, m_name);
        break;
    case CODE_NAME:
        printf ("generate CODE_NAME (%s) not implemented", m_name);
        break;
    case CODE_IF:
        reg1 = bc->getRegister (f->m_blockLevel);
        m_first-> generate (bc, f, reg1);
        reg2 = bc->getLabel ();
        bc->addJumpZero (reg1, reg2);
        bc->freeRegister (reg1);
        m_second->generate (bc, f, -1);
        if (m_third) {
            reg3 = bc->getLabel ();
            bc->addJump (reg3); // jump to end of else
            bc->setLabel (reg2);
            m_third->generate (bc, f, -1);
            bc->setLabel (reg3);
        } else {
            bc->setLabel (reg2);
        }
        break;
    case CODE_WHILE: 
        reg1 = bc->getLabel ();
        bc->setLabel (reg1);
        reg2 = bc->getRegister (f->m_blockLevel);
        m_first-> generate (bc, f, reg2);
        reg3 = bc->getLabel ();
        bc->addJumpZero (reg2, reg3);
        bc->freeRegister (reg2);
        continueLabel = bc->setContinueLabel (reg1);
        breakLabel = bc->setBreakLabel (reg3);
        m_second->generate (bc, f, -1);
        bc->addJump (reg1); // jump to end of else
        bc->setLabel (reg3);
        bc->setBreakLabel (breakLabel);
        bc->setContinueLabel (continueLabel);
        break;
    case CODE_FOR:
        reg1 = bc->getLabel ();
        bc->setLabel (reg1);                           // mark condition
        reg2 = bc->getRegister (f->m_blockLevel);
        m_first-> generate (bc, f, reg2);              // condition instruction
        reg3 = bc->getLabel ();
        bc->addJumpZero (reg2, reg3);                  // test loop condition
        bc->freeRegister (reg2);
        cnt1 = bc->getLabel ();
        continueLabel = bc->setContinueLabel (cnt1);   // set new continue / break labels
        breakLabel = bc->setBreakLabel (reg3);         //  before start of loop (and keep the old ones)
        m_second->generate (bc, f, -1);                // loop instructions
        bc->setLabel (cnt1);                           // mark post instruction (for continue jumps)
        m_third->generate (bc, f, -1);                 // post instruction
        bc->addJump (reg1);                            // jump back to loop condition
        bc->setLabel (reg3);                           // mark loop exit
        bc->setBreakLabel (breakLabel);                // restore previous break label
        bc->setContinueLabel (continueLabel);          // restore previous continue label
        break;
    case CODE_CONTINUE:
        bc->addJump (bc->getContinueLabel ());
        break;
    case CODE_BREAK:
        bc->addJump (bc->getBreakLabel ());
        break;
    case CODE_DEFAULT:
    case CODE_CASE:
        bc->setLabel (m_ival); // use label index stored in m_ival
        break;
    case CODE_SWITCH:
        reg1 = bc->getRegister (f->m_blockLevel);
        m_first->generate (bc, f, reg1);
        reg2 = bc->getLabel ();
        breakLabel = bc->setBreakLabel (reg2);
        // Parse all BLOCK's CASE & DEFAULT to build test & jump code
        tmp = m_second->m_first;
        reg3 = bc->getRegister (f->m_blockLevel);
        while (tmp) {
            if (tmp->m_type == CODE_CASE) {
                tmp->m_ival = bc->getLabel (); // reuse m_ival to store label index
                tmp->m_first->generate (bc, f, reg3);
                bc->add (ByteCode::ASM_TEST_NEQ, reg3, reg1);
                bc->addJumpZero (reg3, tmp->m_ival);
            } else if (tmp->m_type == CODE_DEFAULT) {
                tmp->m_ival = bc->getLabel (); // reuse m_ival to store label
                defaultLabel = tmp; // keep default until the end
            }
            tmp = tmp->m_next;
        }
        bc->freeRegister (reg3);
        bc->addJump (defaultLabel ? defaultLabel->m_ival : reg2);
        bc->freeRegister (reg1);
        m_second->generate (bc, f, -1);
        bc->setLabel (reg2);
        bc->setBreakLabel (breakLabel);
        break;
    case CODE_BLOCK:
        f->m_blockLevel++;
        if (m_first) {
            m_first->generateAll (bc, f);
        }
        f->removeVars (f->m_blockLevel, bc);
        f->m_blockLevel--;
        break;
    case CODE_PARAM:
        assert (reg>=0);
        m_first->generate (bc, f, reg);
        break;
    case CODE_CALL_STATIC:
        reg1 = m_first->m_ival;
        reg2 = m_second->m_ival;
        if (m_third) {
            nbParams = m_third->getLength();
            reg3 = bc->getRegister (f->m_blockLevel, true);
            for (int i=1; i<nbParams; i++) {
                bc->getRegister (f->m_blockLevel, true);
            }
            int preg = reg3;
            tmp = m_third;
            while (tmp) {
                tmp->generate (bc, f, preg++);
                tmp = tmp->m_next;
            }
        } else {
            nbParams = 0;
            // at least one register to store the return value
            reg3 = bc->getRegister (f->m_blockLevel, true);
        }
        bc->add (ByteCode::ASM_EXT_CALL, reg1, reg2, reg3, nbParams);
        if (reg>=0) {
            bc->add (ByteCode::ASM_MOVE_REG_REG, reg, reg3);
        }
        if (m_third) {
            for (int i=0; i<nbParams; i++) {
                bc->freeRegister (reg3++);
            }
        } else {
            bc->freeRegister (reg3);
        }
        break;
    case CODE_CALL_FUNCTION:
        reg1 = m_first->m_ival;
        if (m_second) {
            nbParams = m_second->getLength();
            reg3 = bc->getRegister (f->m_blockLevel, true);
            for (int i=1; i<nbParams; i++) {
                bc->getRegister (f->m_blockLevel, true);
            }
            int preg = reg3;
            tmp = m_second;
            while (tmp) {
                tmp->generate (bc, f, preg++);
                tmp = tmp->m_next;
            }
        } else {
            nbParams = 0;
            // at least one register to store the return value
            reg3 = bc->getRegister (f->m_blockLevel, true);
        }
        bc->add (ByteCode::ASM_INT_CALL, reg1, reg3);
        if (reg>=0) {
            bc->add (ByteCode::ASM_MOVE_REG_REG, reg, reg3);
        }
        if (m_second) {
            for (int i=0; i<nbParams; i++) {
                bc->freeRegister (reg3++);
            }
        } else {
            bc->freeRegister (reg3);
        }
        break;
    case CODE_RETURN:
        reg1 = bc->getRegister (f->m_blockLevel);
        if (m_first) {
            m_first->generate (bc, f, reg1);
        } else {
            bc->add (ByteCode::ASM_LOAD_REG_INT, reg1);
            bc->addInt (0);
        }
        bc->add (ByteCode::ASM_RETURN, reg1);
        bc->freeRegister (reg1);
        break;
    case CODE_NEW_VAR:
        reg1 = bc->getRegister (f->m_blockLevel);
        var = f->addVar (m_first->m_name, f->m_blockLevel, reg1);
        if (var) {
            var->setIndex (reg1);
            if (m_second) {
                m_second->generate (bc, f, reg1);
            } else {
                bc->add (ByteCode::ASM_LOAD_REG_INT, reg1);
                bc->addInt (0);
            }
        } else {
            fprintf (stderr, "error: no var found for %s\n", m_first->m_name);
        }
        break;
    case CODE_ASSIGN:
        reg1 = reg < 0 ? bc->getRegister (f->m_blockLevel) : reg;
        if (m_second) {
            m_second->generate (bc, f, reg1);
            m_first->generate (bc, f, reg1);
        }
        if (reg < 0) bc->freeRegister (reg1);
        break;
    case CODE_GET_VAR:
        var = f->findVar (m_first->m_name);
        if (var) {
            assert (reg>=0);
            reg2 = var->getIndex ();
            bc->add (ByteCode::ASM_MOVE_REG_REG, reg, reg2);
        } else {
            fprintf (stderr, "error: no var found for %s\n", m_first->m_name);
            exit (1);
        }
        break;
    case CODE_SET_VAR:
        var = f->findVar (m_first->m_name);
        if (var) {
            assert (reg>=0);
            reg1 = var->getIndex ();
            bc->add (ByteCode::ASM_MOVE_REG_REG, reg1, reg);
        } else {
            fprintf (stderr, "error: no var found for %s\n", m_first->m_name);
            exit (1);
        }
        break;
    case CODE_SET_FIELD:
        assert (reg>=0);
        bc->add (ByteCode::ASM_FIELD_SET_INT_REG, m_first->m_ival, reg);
        break;
    case CODE_GET_FIELD: 
        assert (reg>=0);
        bc->add (ByteCode::ASM_FIELD_GET_INT_REG, m_first->m_ival, reg);
        break;
    case CODE_USE_FIELD:
        assert (reg>=0);
        bc->add (ByteCode::ASM_FIELD_USE_INT, m_first->m_ival);
        if (m_next) {
            m_next->generate (bc, f, reg);
        }
        break;
    case CODE_USE_IDX_FIELD:
        assert (reg>=0);
        bc->add (ByteCode::ASM_FIELD_PUSH);
        reg1 = bc->getRegister (f->m_blockLevel);
        m_first->generate (bc, f, reg1);
        bc->add (ByteCode::ASM_FIELD_POP);
        bc->add (ByteCode::ASM_FIELD_IDX_REG, reg1);
        bc->freeRegister (reg1);
        m_next->generate (bc, f, reg);
        break;
    case CODE_INC:
    case CODE_PLUS: 
        generateBinary (ByteCode::ASM_ADD_REG_REG, bc, f, reg); break;
    case CODE_DEC:
    case CODE_MINUS:
        generateBinary (ByteCode::ASM_SUB_REG_REG, bc, f, reg); break;
    case CODE_MULT:
        generateBinary (ByteCode::ASM_MUL_REG_REG, bc, f, reg); break;
    case CODE_DIV:
        generateBinary (ByteCode::ASM_DIV_REG_REG, bc, f, reg); break;
    case CODE_MODULO:
        generateBinary (ByteCode::ASM_MOD_REG_REG, bc, f, reg); break;
    case CODE_EQUAL:
        generateBinary (ByteCode::ASM_TEST_EQU, bc, f, reg); break;
    case CODE_NOTEQUAL:
        generateBinary (ByteCode::ASM_TEST_NEQ, bc, f, reg); break;
    case CODE_GREATER:
        generateBinary (ByteCode::ASM_TEST_GRT, bc, f, reg); break;
    case CODE_GREATEQ:
        generateBinary (ByteCode::ASM_TEST_GRE, bc, f, reg); break;
    case CODE_LESSER:
        generateBinary (ByteCode::ASM_TEST_LES, bc, f, reg); break;
    case CODE_LESSEQ:
        generateBinary (ByteCode::ASM_TEST_LEE, bc, f, reg); break;
    case CODE_LOG_AND:
        //generateBinary (ByteCode::ASM_TEST_AND, bc, f, reg); break;
        assert (reg>=0);
        cnt1 = bc->getLabel ();
        m_first->generate (bc, f, reg);
        bc->addJumpZero (reg, cnt1);
        m_second->generate (bc, f, reg);
        bc->setLabel (cnt1);
        break;
    case CODE_LOG_OR:
        //generateBinary (ByteCode::ASM_TEST_OR, bc, f, reg); break;
        assert (reg>=0);
        m_first->generate (bc, f, reg);
        cnt1 = bc->getLabel ();
        if (ByteCode::s_compat) {
            // Old byte only supports the JUMP_ZERO and no negation...
            cnt2 = bc->getLabel ();
            bc->addJumpZero (reg, cnt2);
            bc->addJump (cnt1);
            bc->setLabel (cnt2);
        } else {
            bc->addJumpZero (reg, cnt1, true);
        }
        m_second->generate (bc, f, reg);
        bc->setLabel (cnt1);
        break;
    case CODE_BIT_AND:
        generateBinary (ByteCode::ASM_BIT_AND, bc, f, reg); break;
    case CODE_BIT_OR:
        generateBinary (ByteCode::ASM_BIT_OR, bc, f, reg); break;
    case CODE_BIT_XOR:
        generateBinary (ByteCode::ASM_BIT_XOR, bc, f, reg); break;
    case CODE_BIT_INV:
        assert (reg>=0);
        m_first->generate (bc, f, reg);
        bc->add (ByteCode::ASM_BIT_INV, reg);
        break;
    case CODE_BIT_LSHIFT:
        generateBinary (ByteCode::ASM_BIT_LS, bc, f, reg); break;
    case CODE_BIT_RSHIFT:
        generateBinary (ByteCode::ASM_BIT_RS, bc, f, reg); break;
    case CODE_BIT_RRSHIFT:
        generateBinary (ByteCode::ASM_BIT_RRS, bc, f, reg); break;
    case CODE_TERNARY_COMP: // reg1 ? reg2 : reg3
        assert (reg>=0);
        m_first-> generate (bc, f, reg);
        cnt1 = bc->getLabel ();
        bc->addJumpZero (reg, cnt1);
        m_second->generate (bc, f, reg);
        cnt2 = bc->getLabel ();
        bc->addJump (cnt2); // jump to end
        bc->setLabel (cnt1);
        m_third->generate (bc, f, reg);
        bc->setLabel (cnt2);
        break;
    default:
        fprintf (stderr, "Code::generate: unknown code %d\n", m_type);
    }
};

int Code::getOpArity (int op) {
    switch (op) {
    case CODE_NOT:
    case CODE_PRE_INC:
    case CODE_PRE_DEC:
    case CODE_BIT_INV:
    case CODE_INC:
    case CODE_DEC:
        return 1;
    case CODE_PLUS:
    case CODE_MINUS:
    case CODE_MULT:
    case CODE_DIV:
    case CODE_MODULO:
    case CODE_GREATER:
    case CODE_GREATEQ:
    case CODE_LESSER:
    case CODE_LESSEQ:
    case CODE_EQUAL:
    case CODE_NOTEQUAL:
    case CODE_LOG_AND:
    case CODE_LOG_OR:
    case CODE_BIT_AND:
    case CODE_BIT_OR:
    case CODE_BIT_XOR:
    case CODE_BIT_LSHIFT:
    case CODE_BIT_RSHIFT:
    case CODE_BIT_RRSHIFT:
        return 2;
    case CODE_TERNARY_COMP:
        return 3;
    default:
        fprintf (stderr, "Code::getOpArity: unknown code %d\n", op);
    case CODE_ERROR:
        return 0;
    }
}

int Code::getOpPrecedence (int op) {
    switch (op) {
    case CODE_INC:
    case CODE_DEC:
        return 12;
    case CODE_PRE_INC:
    case CODE_PRE_DEC:
    case CODE_BIT_INV:
    case CODE_NOT:
        return 11;
    case CODE_MULT:
    case CODE_DIV:
    case CODE_MODULO:
        return 10;
    case CODE_PLUS:
    case CODE_MINUS:
        return 9;
    case CODE_BIT_LSHIFT:
    case CODE_BIT_RSHIFT:
    case CODE_BIT_RRSHIFT:
        return 8;
    case CODE_GREATER:
    case CODE_GREATEQ:
    case CODE_LESSER:
    case CODE_LESSEQ:
        return 7;
    case CODE_EQUAL:
    case CODE_NOTEQUAL:
        return 6;
    case CODE_BIT_AND:
        return 5;
    case CODE_BIT_XOR:
        return 4;
    case CODE_BIT_OR:
        return 3;
    case CODE_LOG_AND:
        return 2;
    case CODE_LOG_OR:
        return 1;
    case CODE_TERNARY_COMP:
        return 0;
    default:
        fprintf (stderr, "Code::getOpPrecedence: unknown code %d\n", op);
    case CODE_ERROR:
        return 0;
    }
}

bool Code::isOpRightAssociative (int op) {
    switch (op) {
    case CODE_PRE_INC:
    case CODE_PRE_DEC:
    case CODE_BIT_INV:
    case CODE_NOT:
    case CODE_TERNARY_COMP:
        return true;
    }
    return false;
}

