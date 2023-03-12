.data
extern address : qword

.code
	AsmJmp PROC
		jmp qword ptr [address]
	AsmJmp ENDP
end
