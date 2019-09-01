includelib legacy_stdio_definitions.lib

.data
extern presentAddr : qword
extern jmpBackAddr : qword

; This jmps to our hook, then performs instructions originally performed
; by dxgi.dll in the memory that we've replaced, and then jmps back.

; These assembly files aren't really necessary, as you could easily
; allocate space and directly write the needed instructions into that space,
; I just didn't want to put in the effort to replace these

.code
	JmpToHookAndJmpBack64 PROC 
		jmp qword ptr [presentAddr]

		; Previously overwritten instructions will be put here
		; The original instructions will be copied and then pasted into this function's code,
		; after the initial jump to the present function. The original instructions are commented below.
		; Copying and pasting should increase compatability with other software or hooks,
		; rather than hardcoding the instructions. Nop's are used for creating space.
		;mov [rsp+10h],rbx
		;mov [rsp+20h],rsi
		;push rbp
		;push rdi
		;push r14
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop

		jmp qword ptr [jmpBackAddr]
	JmpToHookAndJmpBack64 ENDP
end

