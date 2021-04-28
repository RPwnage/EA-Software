#include <setjmp.h>

struct SEhIndexEntry
{
	u32 Addr() const {return FuncAddr + (u32)&FuncAddr + 0x80000000;} // not sure why 0x80000000 is necessary
	u32* pData() const {return (u32*)(Data + (u32)&Data);}
	u32 FuncAddr;
	u32 Data;
};

bool operator < (const SEhIndexEntry& lhs, u32 rhs) {return lhs.Addr() < rhs;}
bool operator < (u32 lhs, const SEhIndexEntry& rhs) {return lhs < rhs.Addr();}

extern "C" const SEhIndexEntry __exidx_start[];
extern "C" const SEhIndexEntry __exidx_end[];

u32 StackTrace_FillStackTrace( u32 * trace, u32 max_entries )
{
	u32 returnAddress = __builtin_current_pc();
	u32* vsp = (u32*)__builtin_current_sp();
	u32 entry = 0;
	jmp_buf buf;
	setjmp(buf); // capture cpu registers so we can unwind past alloca
	u32* cpuregs = (u32*)&buf;
	while (entry < max_entries && returnAddress)
	{
		const SEhIndexEntry* pEntry = std::upper_bound(__exidx_start, __exidx_end, returnAddress);
		if (pEntry == __exidx_start)
			break;
		--pEntry;
		if (returnAddress < pEntry->Addr())
			break;
		if (pEntry->Data == 1)
			break;

		const u32* pData;
		if (pEntry->Data>>31)
			pData = &pEntry->Data;
		else
			pData = pEntry->pData();
		int numWords = 1;
		int type = (*pData>>24) & 15;
		ASSERT(type == 0 || type == 1, "Unrecognised unwind table type");
		if (type == 1)
			numWords += (*pData>>16)&255; // number of additional words

		// endian swap the words so we have an easier time decoding them
		ASSERT(numWords < 16, "Too many words in stack unwind instruction stream");
		u32 instrucBuf[16];
		for(int i=0; i<numWords; ++i)
			instrucBuf[i] = __builtin_rev(*pData++);
		const u8* pInstruc = (u8*)instrucBuf + 1 + (type == 1);
		const u8* pEnd = (u8*)&instrucBuf[numWords];

		returnAddress = 0;
		while (pInstruc < pEnd && *pInstruc != 0xB0)
		{
			int instruc = *pInstruc++;
			
			// 10100nnn  Pop r4-r[4+nnn]
			// 10101nnn  Pop r4-r[4+nnn], r14 
			if ((instruc & 0xF0) == 0xA0)
			{
				for(int i=0; i<=(instruc&7); ++i)
					cpuregs[i] = *vsp++;
				if (instruc & 8)
				{
					returnAddress = *vsp++;		
					ASSERT(returnAddress > 0x81000000, "Bad return address during stack backtrace");
				}
			}
			// 1000iiii iiiiiiii  Pop up to 12 integer registers under masks {r15-r12}, {r11-r4} (see remark b)
			else if ((instruc & 0xF0) == 0x80)
			{
				int regs = *pInstruc++;
				// 10000000 00000000  Refuse to unwind (for example, out of a cleanup)
				if (!regs && instruc == 0x80)
					break;
				for(int i=0; regs; ++i, regs >>= 1)
					if (regs & 1)
						cpuregs[i] = *vsp++;
				if (instruc & 1) 
					cpuregs[8] = *vsp++;
				if (instruc & 2) 
					cpuregs[9] = *vsp++;
				if (instruc & 4) 
				{
					returnAddress = *vsp++;
					ASSERT(returnAddress > 0x81000000, "Bad return address during stack backtrace");
				}
				if (instruc & 8) 
					++vsp;
			}
			// 00xxxxxx  vsp = vsp + (xxxxxx << 2) + 4. Covers range 0x04-0x100 inclusive
			else if ((instruc & 0xC0) == 0)
			{
				vsp += (instruc & 0x3F) + 1;
			}
			// 10110010 uleb128  vsp = vsp + 0x204+ (uleb128 << 2)  
			// (for vsp increments of 0x104-0x200, use 00xxxxxx twice)
			else if (instruc == 0xB2)
			{		
				vsp += 0x81;
				for(int shift=0; ; shift +=7)
				{
					vsp += (*pInstruc & 0x7F) << shift;
					if (!(*pInstruc++ & 0x80))
						break;
				}
			}
			// 10110001 0000iiii (i not all 0) Pop integer registers under mask {r3, r2, r1, r0}
			else if (instruc == 0xB1)
			{
				int regs = *pInstruc++;
				for(; regs; regs >>= 1)
					++vsp;
			}
			// 11001001 sssscccc  Pop VFP double precision registers D[ssss]-D[ssss+cccc] saved (as if) by FSTMFDD
			else if (instruc == 0xC9)
			{
				vsp += ((*pInstruc++ & 15) + 1) * 2;
			}
			// 11010nnn  Pop VFP double-precision registers D[8]-D[8+nnn] saved (as if) by FSTMFDD 
			else if ((instruc & 0xF8) == 0xD0)
			{
				vsp += ((instruc & 7) + 1) * 2;
			}
			// 1001nnnn  Set vsp = r[nnnn]
			else if ((instruc & 0xF0) == 0x90)
			{
				// in all likelihood this function uses alloca and the frame pointer is 
				// therefore saved in a different register. this is the only reason for tracking
				// the cpuregs.
				ASSERT((u32*)cpuregs[(instruc&15)-4] - vsp < 0x10000, "suspiciously large stack frame during stack backtrace");
				vsp = (u32*)cpuregs[(instruc&15)-4];
			}
			else
			{
				ASSERTV(0, "Unrecognised stack unwind instruction: 0x%x\n", instruc);
			}
		}
		ASSERT(pInstruc == pEnd || *pInstruc == 0xB0, "gone past end of stack unwind instruction stream");			
		if (returnAddress)
			trace[entry++] = returnAddress;
	}
	if (entry < max_entries)
		trace[entry] = 0; // to make it clear where the end is
	return entry;
}
