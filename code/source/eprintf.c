#include <stddef.h>
#include <stdarg.h>

asm(
".global eprintf_flush\n"
".global eprintf_buf\n"
"\n"
"eprintf_flush:\n"
"	MOV   R12, R12\n"
"	B     return\n"
"	.word 0x6464\n"
"eprintf_buf:\n"
"	.fill 120\n"
"	.word 0\n"
"return:\n"
"	BX    LR\n"
);

int OS_VSNPrintf(char* dst, size_t len, const char* fmt, va_list vlist);
void eprintf_flush();
extern char eprintf_buf[120];

int eprintf(const char* fmt, ...)
{
	int result;
	va_list argList;
	va_start(argList, fmt);
	result = OS_VSNPrintf(eprintf_buf, 120, fmt, argList);
	eprintf_flush();
	va_end(argList);
	return result;
}
