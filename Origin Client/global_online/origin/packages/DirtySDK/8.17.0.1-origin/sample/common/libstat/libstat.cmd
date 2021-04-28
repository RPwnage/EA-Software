/*_stack = -1;*/
_stack_size = 0x6000;
_heap_size = 0x400000;

GROUP(
-ldev
-ldma
-lgraph
-lipu
-lpc
-lpkt
-lvu0
-lc
-lgcc
-lkernl
-lm
-lpad
-lmrpc
-lcdvd
-lmc
)

ENTRY(ENTRYPOINT)

SECTIONS
{
	.text	0x00100000:
	{
		crt0.o(.text)
		*(.text)
        	QUAD(0)
	}
	.reginfo		  : { KEEP(*(.reginfo)) }
	.data		ALIGN(128): { *(.data) }
	.rodata		ALIGN(128): { *(.rodata) }
	.rdata		ALIGN(128): { *(.rdata) }
	.gcc_except_table ALIGN(128): { *(.gcc_except_table) }
	_gp = ALIGN(128) + 0x7ff0;
	.lit8       	ALIGN(128): { *(.lit8) }
	.lit4       	ALIGN(128): { *(.lit4) }
	.sdata		ALIGN(128): { *(.sdata) }
	.sbss		ALIGN(128): { _fbss = .; *(.sbss) *(.scommon) }
	.bss		ALIGN(128): { *(.bss) }
	.stack		ALIGN(128): { _stack = .; . += _stack_size; }
	end = .;
	_end = .;
	.spad	0x70000000:
	{
		 crt0.o(.spad)
		 *(.spad)
	}
}
	
