includelib legacy_stdio_definitions.lib

.data
extern procAddr : qword

; This code simply jumps to an address in memory,
; used for forward exporting functions to the original DLL

.code
	JmpToAddr64 PROC
		jmp qword ptr [procAddr]
	JmpToAddr64 ENDP
end
