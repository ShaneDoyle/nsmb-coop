asm(R"(
#define BASE_IOREG			0x04000000
#define BASE_SHARED			0x027FF000
#define SRAMCHECKER7		BASE_SHARED + 0xA7E
#define IO_OFS_SNDEXCNT		0x4700

detectEnvironment:

	@ Detect DS mode on DSi
	mov		r0, #BASE_IOREG
	add		r0, #IO_OFS_SNDEXCNT
	ldr		r0, [r0]					@ sndexcnt = regSNDEXCNT

	and		r0, #0xA000					@ sndexcnt &= 0xA000
	cmp		r0, #0x8000					@ if (sndexcnt == 0x8000) {
	ldreq	r1, =SRAMCHECKER7			@	SRAMCHECKER7 = 0x8000
	strheq	r0, [r1]					@ }

	bx		lr
)");

extern "C"
{
	void CTRDG_Init(void);
	void detectEnvironment(void);
}

// Get console type at the end of OS_Init

ncp_call(0x037FCEEC)
void call_037FCEEC()
{
	CTRDG_Init(); // Keep replaced instruction
	detectEnvironment();
}
