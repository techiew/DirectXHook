includelib legacy_stdio_definitions.lib

.data
extern procAddr : qword

; This code simply jumps to an address in memory,
; used for forward exporting functions to the original DLL

.code
	JmpToAddr PROC
		jmp qword ptr [procAddr]
	JmpToAddr ENDP
end
