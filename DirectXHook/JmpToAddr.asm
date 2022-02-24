includelib legacy_stdio_definitions.lib

.data
extern functionAddress : qword

.code
	JmpToAddr PROC
		jmp qword ptr [functionAddress]
	JmpToAddr ENDP
end
