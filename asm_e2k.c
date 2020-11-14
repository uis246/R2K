/*
	E2K disassembler by uis

	Distributed under MIT
*/

#include <stdio.h>
#include <string.h>
#include <r_types.h>
#include <r_lib.h>
#include <r_asm.h>

//Syllables order:
//HS
//SS (12)
//ALS{0-5} (26..31)
//CS0 (14)
//ALES{2,5} (22,25)
//CS1 (15)
//ALES{0,1,3,4} (20,21,23,24)
//AAS{0..5} (SS)
//LTS{3..0} {readed}[len-readed-cdscnt-plscnt]
//PLS{2..0} {end-cdscnt-plscnt}[plscnt]
//CDS{2..0} {end-cdscnt}[cdscnt]

//HS format
//{31..26}	|{25..20}	|{19:18}	|{17:16}
//ALS{5..0}	|ALES{5..0}	|PLS count	|CDS count
//
//|{15..14}	|13			|12	|11		|10			|{9..7}	|{6:4}			|{3:0}
//|CS{1..0}	|set_mark?	|SS	|rsrvd	|loop_mode?	|nop?	|payload length	|middle ptr?

struct hs_bits {
	const char *name;
	unsigned int size;
	int bit;
};
static struct hs_bits hs_bits[] = {
	{"SS", 4, 12},
	{"ALS0", 4, 26},
	{"ALS1", 4, 27},
	{"ALS2", 4, 28},
	{"ALS3", 4, 29},
	{"ALS4", 4, 30},
	{"ALS5", 4, 31},
	{"CS0", 4, 14},
	{"ALES2", 2, 22},
	{"ALES5", 2, 25},
	{"CS1", 4, 15},
	{"ALES0", 2, 20},
	{"ALES1", 2, 21},
	{"ALES3", 2, 23},
	{"ALES4", 2, 24},
};

static int inslen(const ut8 *buf) {
	ut32 hs = r_read_le32(buf);
	int len = (hs & 0x70) >> 4;
	return (len+1)*8;
}

static bool addSyll(const ut32 hs, const int bit, const char *name, RStrBuf *sb) {
	if(hs & (1<<bit)) {
		r_strbuf_appendf(sb, " %s", name);
		return true;
	}
	return false;
}

static void parseHS(const ut8 *buf, int oplen, RStrBuf *sb) {
	int pos = 4;
	ut32 hs = r_read_le32(buf)/*, opcode = r_read_le32(buf+4)*/;
	for(size_t i = 0; i < (sizeof(hs_bits)/sizeof(hs_bits[0])); i++) {
		if(pos == oplen)
			break;
		if(addSyll(hs, hs_bits[i].bit, hs_bits[i].name, sb))
			pos += hs_bits[i].size;
	}
	//FIXME
	if(pos%4)
		pos+=2;
	int plscnt, cdscnt;
	cdscnt = (hs>>16)&3;
	plscnt = (hs>>18)&3;
	for(int i = (oplen-(pos+cdscnt+plscnt))/4; i > 0; i--) {
		r_strbuf_appendf(sb, " LTS%u", i);
		pos += 4;
	}
	for(int i = plscnt; i > 0; i--) {
		r_strbuf_appendf(sb, " PLS%u", i);
		pos += 4;
	}
	for(int i = cdscnt; i > 0; i--) {
		r_strbuf_appendf(sb, " CDS%u", i);
		pos += 4;
	}
}

static int disassemble(RAsm *as, RAsmOp *op, const ut8 *buf, int len) {
	if(len < 4)
		return -1;
	int ilen = op->size = inslen(buf);
	if(ilen > len)
		return -1;
//	sprintf(op->buf_asm.buf, "Len: %d:", ilen);
	r_strbuf_setf(&op->buf_asm, ";Len:%d: {", ilen);
	parseHS(buf, ilen, &op->buf_asm);
	r_strbuf_append(&op->buf_asm, " }");
	return ilen;
}

RAsmPlugin r_asm_plugin_e2k = {
	.name = "elbrus",
	.license = "MIT",
	.author = "uis",
	.desc = "ELBRUS disassembly plugin",
	.arch = "elbrus",
	.bits = 32|64,
	.endian = R_SYS_ENDIAN_LITTLE,
	.disassemble = &disassemble
};

#ifndef R2_PLUGIN_INCORE
R_API RLibStruct radare_plugin = {
    .type = R_LIB_TYPE_ASM,
    .data = &r_asm_plugin_e2k,
	.version = R2_VERSION,
	.pkgname = "RE2K decompiler"
};
#endif
