OUTPUT_FORMAT("elf64-littleaarch64", "elf64-littleaarch64", "elf64-littleaarch64")
OUTPUT_ARCH("aarch64")
ENTRY(_start)

STACK_EL3_SIZE = 0x010;
STACK_EL2_SIZE = 0x010;
STACK_EL1_SIZE = 0x010;
STACK_EL0_SIZE = 0x010;

MEMORY
{
	ram : org = 0x00000000, len = 0x00004000	/* 16KB */
}

SECTIONS
{
	.text :
	{
		PROVIDE(__image_start = .);
		PROVIDE(__text_start = .);
		*libgcc.a:*.o (.text*)
		*(.text*)
		*(.iplt)
		*(.note.gnu.build-id)
		PROVIDE(__text_end = .);
	} > ram

	.initcall ALIGN(8) :
	{
		PROVIDE(__initcall_start = .);
		KEEP(*(.initcall_0.text))
		KEEP(*(.initcall_1.text))
		KEEP(*(.initcall_2.text))
		KEEP(*(.initcall_3.text))
		KEEP(*(.initcall_4.text))
		KEEP(*(.initcall_5.text))
		KEEP(*(.initcall_6.text))
		KEEP(*(.initcall_7.text))
		KEEP(*(.initcall_8.text))
		KEEP(*(.initcall_9.text))
		PROVIDE(__initcall_end = .);
	} > ram

	.exitcall ALIGN(8) :
	{
		PROVIDE(__exitcall_start = .);
		KEEP(*(.exitcall_9.text))
		KEEP(*(.exitcall_8.text))
		KEEP(*(.exitcall_7.text))
		KEEP(*(.exitcall_6.text))
		KEEP(*(.exitcall_5.text))
		KEEP(*(.exitcall_4.text))
		KEEP(*(.exitcall_3.text))
		KEEP(*(.exitcall_2.text))
		KEEP(*(.exitcall_1.text))
		KEEP(*(.exitcall_0.text))
		PROVIDE(__exitcall_end = .);
	} > ram

	.romdisk ALIGN(8) :
	{
		PROVIDE(__romdisk_start = .);
		KEEP(*(.romdisk))
		PROVIDE(__romdisk_end = .);
	} > ram

	.rodata ALIGN(8) :
	{
		PROVIDE(__rodata_start = .);
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.rodata*)))
		PROVIDE(__rodata_end = .);
	} > ram

	.data ALIGN(8) :
	{
		PROVIDE(__data_start = .);
		*(.data*)
		. = ALIGN(8);
  		PROVIDE(__data_end = .);
		PROVIDE(__image_end = .);
	} > ram

	.bss ALIGN(8) (NOLOAD) :
	{
		PROVIDE(__bss_start = .);
		*(.bss*)
		*(.sbss*)
		*(COMMON)
		. = ALIGN(8);
		PROVIDE(__bss_end = .);
	} > ram

	.stack ALIGN(16) (NOLOAD) :
	{
		PROVIDE(__stack_start = .);
		PROVIDE(__stack_el3_start = .);
		. += STACK_EL3_SIZE;
		PROVIDE(__stack_el3_end = .);
		. = ALIGN(16);
		PROVIDE(__stack_el2_start = .);
		. += STACK_EL2_SIZE;
		PROVIDE(__stack_el2_end = .);
		. = ALIGN(16);
		PROVIDE(__stack_el1_start = .);
		. += STACK_EL1_SIZE;
		PROVIDE(__stack_el1_end = .);
		. = ALIGN(16);		
		PROVIDE(__stack_el0_start = .);
		. += STACK_EL0_SIZE;
		PROVIDE(__stack_el0_end = .);
		. = ALIGN(16);
		PROVIDE(__stack_end = .);
	} > ram

	/DISCARD/ : { *(.dynsym) }
	/DISCARD/ : { *(.dynstr*) }
	/DISCARD/ : { *(.dynamic*) }
	/DISCARD/ : { *(.plt*) }
	/DISCARD/ : { *(.interp*) }
	/DISCARD/ : { *(.gnu*) }
}
