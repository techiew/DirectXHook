includelib legacy_stdio_definitions.lib

.model flat, c

.data
extern procAddr : dword

; This code simply jumps to an address in memory,
; used for forward exporting functions to the original DLL

.code
	JmpToAddr PROC
		jmp [procAddr]
	JmpToAddr ENDP
end