/*
 * Emerald NX game linker script.
 *
 * This is libnx's switch.ld with the port's game-data layout added, following
 * the same scheme as 3ds_port/emerald3ds.ld.in and android_port/emerald_android.ld:
 *
 *   .ctr_game_data  the game's own variables, kept together so the soft reset
 *                   can restore them as the GBA clears its RAM;
 *   .gamedata       the game translation units' read-only data and pointer
 *                   tables, taken out of the executable at build time and
 *                   loaded from romfs at start-up (gen_gamedata_bundle.py).
 *
 * The shipped link reserves .gamedata as NOLOAD; the image link keeps its
 * contents so the bundle can be extracted. Both links must place every symbol
 * at the same address.
 */
OUTPUT_ARCH(aarch64)
ENTRY(_start)

PHDRS
{
	code   PT_LOAD FLAGS(5) /* Read | Execute */;
	rodata PT_LOAD FLAGS(4) /* Read */;
	data   PT_LOAD FLAGS(6) /* Read | Write */;
	dyn    PT_DYNAMIC;
}

SECTIONS
{
	PROVIDE_HIDDEN( __start__ = 0x0 );
	/* lld provides __ehdr_start for the Android shared library; GNU ld does
	 * not.  3ds_assets.c and 3ds_script_loader.c use it as the load bias of
	 * the PIE image, which is __start__ itself. */
	PROVIDE_HIDDEN( __ehdr_start = __start__ );

	/* =========== CODE section =========== */
	. = __start__;
	PROVIDE_HIDDEN( __code_start = . );

	.text :
	{
		KEEP (*(.crt0))
		*(.text.unlikely .text.*_unlikely .text.unlikely.*)
		*(.text.exit .text.exit.*)
		*(.text.startup .text.startup.*)
		*(.text.hot .text.hot.*)
		*(.text .stub .text.* .gnu.linkonce.t.*)
		. = ALIGN(8);
	} :code

	.init :
	{
		KEEP( *(.init) )
		. = ALIGN(8);
	} :code

	.plt :
	{
		*(.plt)
		*(.iplt)
		. = ALIGN(8);
	} :code

	.fini :
	{
		KEEP( *(.fini) )
		. = ALIGN(8);
	} :code

	/* =========== RODATA section =========== */
	. = ALIGN(0x1000);
	PROVIDE_HIDDEN( __rodata_start = . );

	.nx-module-name : { KEEP (*(.nx-module-name)) } :rodata

	/* Only the dynamic relocations belong to the loadable image.  The image
	 * link's --emit-relocs sections (.rela.gamedata and friends) stay
	 * non-allocated, so they do not shift the release link's layout. */
	.rela.dyn : { *(.rela.dyn) *(.rela.plt) } :rodata
	.relr.dyn : { *(.relr.dyn) } :rodata

	.hash     : { *(.hash) } :rodata
	.gnu.hash : { *(.gnu.hash) } :rodata

	.dynsym   : { *(.dynsym) } :rodata
	.dynstr   : { *(.dynstr) } :rodata

	.rodata :
	{
		/* The game's translation units' rodata is bundled in .gamedata. */
		EXCLUDE_FILE(*build/root/*) *(.rodata .rodata.* .gnu.linkonce.r.*)
		/* The port's platform layer stays in the executable. */
		*build/root/src/platform/*(.rodata .rodata.* .gnu.linkonce.r.*)
		. = ALIGN(8);
	} :rodata

	.tls.align :
	{
		QUAD( MAX( ALIGNOF(.tdata), ALIGNOF(.tbss) ) )
	} :rodata

	PROVIDE_HIDDEN( __tls_align = ADDR(.tls.align) );

	.gcc_except_table  : { *(.gcc_except_table .gcc_except_table.*) } :rodata
	.eh_frame_hdr      : { *(.eh_frame_hdr) *(.eh_frame_entry .eh_frame_entry.*) } :rodata
	.eh_frame          : { KEEP (*(.eh_frame)) *(.eh_frame.*) } :rodata
	.gnu_extab         : { *(.gnu_extab*) } : rodata
	.exception_ranges  : { *(.exception_ranges .exception_ranges*) } :rodata

	PROVIDE_HIDDEN( __eh_frame_hdr_start = ADDR(.eh_frame_hdr)                         );
	PROVIDE_HIDDEN( __eh_frame_hdr_end   = ADDR(.eh_frame_hdr) + SIZEOF(.eh_frame_hdr) );

	.gnu.version       : { *(.gnu.version) } :rodata
	.gnu.version_d     : { *(.gnu.version_d) } :rodata
	.gnu.version_r     : { *(.gnu.version_r) } :rodata

	.note.gnu.build-id : { *(.note.gnu.build-id) } :rodata

	/* =========== RELRO section =========== */
	. = ALIGN(0x1000);
	PROVIDE_HIDDEN( __relro_start = . );

	.preinit_array :
	{
		PROVIDE_HIDDEN( __preinit_array_start = . );
		KEEP (*(.preinit_array))
		PROVIDE_HIDDEN( __preinit_array_end = . );
	} :data

	.init_array :
	{
		PROVIDE_HIDDEN( __init_array_start = . );
		KEEP( *(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)) )
		KEEP( *(.init_array .ctors) )
		PROVIDE_HIDDEN( __init_array_end = . );
	} :data

	.fini_array :
	{
		PROVIDE_HIDDEN( __fini_array_start = . );
		KEEP( *(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)) )
		KEEP( *(.fini_array .dtors) )
		PROVIDE_HIDDEN( __fini_array_end = . );
	} :data

	.data.rel.ro :
	{
		/* The game's pointer tables are bundled in .gamedata. */
		EXCLUDE_FILE(*build/root/*) *(.data.rel.ro.local* .gnu.linkonce.d.rel.ro.local.*)
		EXCLUDE_FILE(*build/root/*) *(.data.rel.ro .data.rel.ro.* .gnu.linkonce.d.rel.ro.*)
		. = ALIGN(8);
	} :data

	.dynamic : { *(.dynamic) } :data :dyn

	.got     : { *(.got) *(.igot) } :data
	.got.plt : { *(.got.plt)  *(.igot.plt) } :data

	PROVIDE_HIDDEN( __got_start__ = ADDR(.got)                        );
	PROVIDE_HIDDEN( __got_end__   = ADDR(.got.plt) + SIZEOF(.got.plt) );

	/* =========== DATA section =========== */
	. = ALIGN(0x1000);
	PROVIDE_HIDDEN( __data_start = . );

	.data :
	{
		/* The game's own variables (not its platform/ layer, which holds the
		 * emulated hardware and the flash with the save) come first and
		 * together, for the soft reset. */
		__ctr_game_data = .;
		EXCLUDE_FILE(*build/root/src/platform/*) *build/root/src/*(CTR_WRITABLE_DATA)
		*build/root/data/*(CTR_WRITABLE_DATA)
		. = ALIGN(8);
		__ctr_game_data_end = .;

		/* The port's platform layer and everything outside the game tree. */
		*build/root/src/platform/*(.data .data.*)
		EXCLUDE_FILE(*build/root/*) *(.data .data.* .gnu.linkonce.d.*)
		SORT(CONSTRUCTORS)
		. = ALIGN(8);
	} :data

	/* The only script_data linked is the Mystery Event command table's, which
	 * .gamedata takes below. Another one would be an orphan after .gamedata,
	 * following a NOBITS section with contents: the NOLOAD link would gain a
	 * segment the image does not have, and gen_gamedata_bundle.py would refuse
	 * the two links. */
	.gamedata ALIGN(32) @GAMEDATA_TYPE@ :
	{
		__ctr_gamedata = .;
		/* Every game object, including the rw_data copies whose paths keep
		 * "build/root" after "build/rw/". */
		*build/root/*(.rodata .rodata.* .data.rel.ro .data.rel.ro.*)
		/* The Mystery Event command table: assembled 32-bit addresses, which
		 * only the bundle loader fills in on arm64. */
		*build/root/data/*(script_data)
		. = ALIGN(8);
		__ctr_gamedata_end = .;
	} :data

	.tdata :
	{
		*(.tdata .tdata.* .gnu.linkonce.td.*)
		. = ALIGN(8);
	} :data

	PROVIDE_HIDDEN( __tdata_lma     = ADDR(.tdata)                  );
	PROVIDE_HIDDEN( __tdata_lma_end = ADDR(.tdata) + SIZEOF(.tdata) );

	.tbss :
	{
		*(.tbss .tbss.* .gnu.linkonce.tb.*)
		*(.tcommon)
		. = ALIGN(8);
	} :data

	.bss :
	{
		__ctr_game_bss = .;
		/* Juan's defeat text runs past gDisplayedStringBattle[300]: room behind it. */
		*build/root/src/battle_main.o(.bss.gDisplayedStringBattle)
		. += 1024;
		EXCLUDE_FILE(*build/root/src/platform/*) *build/root/src/*(.bss .bss.* COMMON)
		*build/root/data/*(.bss .bss.* COMMON)
		. = ALIGN(8);
		__ctr_game_bss_end = .;

		*build/root/src/platform/*(.bss .bss.* COMMON)
		EXCLUDE_FILE(*build/root/*) *(.dynbss)
		EXCLUDE_FILE(*build/root/*) *(.bss .bss.* .gnu.linkonce.b.*)
		EXCLUDE_FILE(*build/root/*) *(COMMON)
		. = ALIGN(8);
	} :data

	/* Reserve space for the TLS segment of the main thread */
	.main.tls ALIGN(MAX(ALIGNOF(.tdata),ALIGNOF(.tbss))) :
	{
		. += SIZEOF(.tdata);
		. = ALIGN(ALIGNOF(.tbss));
		. += SIZEOF(.tbss);
	} :data

	PROVIDE_HIDDEN( __tls_start   = ADDR(.main.tls)                     );
	PROVIDE_HIDDEN( __tls_end     = ADDR(.main.tls) + SIZEOF(.main.tls) );

	PROVIDE_HIDDEN( __bss_start__ = ADDR(.bss) );
	PROVIDE_HIDDEN( __bss_end__   = __tls_end  );

	PROVIDE_HIDDEN( __end__ = . );

	/* =========== Argument buffer =========== */
	. = ALIGN(0x1000);
	PROVIDE_HIDDEN( __argdata__ = . );

	/* ==================
	   ==== Metadata ====
	   ================== */

	/* Discard sections that difficult post-processing */
	/DISCARD/ : { *(.group .comment .note) }

	/* Stabs debugging sections. */
	.stab          0 : { *(.stab) }
	.stabstr       0 : { *(.stabstr) }
	.stab.excl     0 : { *(.stab.excl) }
	.stab.exclstr  0 : { *(.stab.exclstr) }
	.stab.index    0 : { *(.stab.index) }
	.stab.indexstr 0 : { *(.stab.indexstr) }

	/* DWARF debug sections.
	   Symbols in the DWARF debugging sections are relative to the beginning
	   of the section so we begin them at 0. */

	/* DWARF 1 */
	.debug          0 : { *(.debug) }
	.line           0 : { *(.line) }

	/* GNU DWARF 1 extensions */
	.debug_srcinfo  0 : { *(.debug_srcinfo) }
	.debug_sfnames  0 : { *(.debug_sfnames) }

	/* DWARF 1.1 and DWARF 2 */
	.debug_aranges  0 : { *(.debug_aranges) }
	.debug_pubnames 0 : { *(.debug_pubnames) }

	/* DWARF 2 */
	.debug_info     0 : { *(.debug_info) }
	.debug_abbrev   0 : { *(.debug_abbrev) }
	.debug_line     0 : { *(.debug_line) }
	.debug_frame    0 : { *(.debug_frame) }
	.debug_str      0 : { *(.debug_str) }
	.debug_loc      0 : { *(.debug_loc) }
	.debug_macinfo  0 : { *(.debug_macinfo) }
}
