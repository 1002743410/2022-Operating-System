SECTION .data
message db "Invalid", 0h

SECTION .bss
input_string: resb 255 ;输入的字符串

first_string: resb 255 ;第一个操作数
second_string: resb 255 ;第二个操作数
operate: resb 255 ;操作符
judge:resb 255 ;无效操作符
add_result: resb 255 ;加法结果
mul_result: resb 255 ;乘法结果
add_ptr: resb 1

SECTION .text
global _start
; 输入字符串
gets:
    push edx
    push ecx
    push ebx
    push eax

    mov edx, ebx
    mov ecx, eax
    mov ebx, 1
    mov eax, 3
    int 80h ;系统调用

    pop eax
    pop ebx
    pop ecx
    pop edx
    ret

; 计算字符串长度
strlen:
    push ebx
    mov ebx, eax
.next:
    cmp BYTE[ebx], 0
    jz .finish ;若字符串为空，ze跳转到.finish
    inc ebx
    jmp .next
.finish:
    sub ebx, eax
    mov eax, ebx
    pop ebx
    ret

;分割字符串，并提取操作符operate
parse_input:
    mov ebx,operate
    mov esi,judge
.loop:
    cmp BYTE[ecx], 43 ;判断字符是否为“+”
    jz .rett_add
    cmp BYTE[ecx], 42 ;判断字符是否为“*”
    jz .rett_mul
    cmp BYTE[ecx], 32 ;判断字符是否为space
    jz .rett
    cmp BYTE[ecx], 10 ;判断换行
    jz .rett
    cmp BYTE[ecx],48 ;判断是否是无效字符
    jl .rett_error

    mov dl, BYTE[ecx]
    mov BYTE[eax], dl
    inc eax
    inc ecx
    jmp .loop

; 若为“+”，则将其保存到操作符中
.rett_add:
    inc ecx
    mov BYTE[ebx],43
    ret
; 若为“*”，则将其保存到操作符中
.rett_mul:
    inc ecx
    mov BYTE[ebx],42
    ret
; 无效输入
.rett_error:
    inc ecx
    mov BYTE[esi],1
    ret
.rett:
    inc ecx
    ret

; 输出字符串
puts:
    push edx
    push ecx
    push ebx
    push eax

    mov ecx, eax
    call strlen
    mov edx, eax
    mov ebx, 1
    mov eax, 4
    int 80h

    pop eax
    pop ebx
    pop ecx
    pop edx
    ret

; 输出单个字符
putchar:
    push edx
    push ecx
    push ebx
    push eax

    mov eax, 4
    mov ebx, 1
    mov ecx, esp
    mov edx, 1
    int 80h

    pop eax
    pop ebx
    pop ecx
    pop edx
    ret

; 换行输出
endl:
    push eax
    mov eax, 0Ah
    call putchar
    pop eax
    ret

format:
    push edx
    push eax
.loop:
    mov eax, mul_result
    add eax, 255
    cmp edx, eax                
    jge .finish
    add BYTE[edx], 48
    inc edx
    jmp .loop
.finish:
    pop eax
    pop edx 
    ret

; 位运算
sub_digit:
    mov ecx, 1
    sub al, 10
    ret

; 位乘
inner_loopMul:
    push esi
    push ebx
    push edx

.loop:
    cmp esi, first_string      ; mul by digits
    jl .finish
    xor eax, eax
    xor ebx, ebx
    add al, BYTE[esi]
    sub al, 48
    add bl, BYTE[edi]          ; add by digits
    sub bl, 48
    mul bl
    add BYTE[edx], al
    mov al, BYTE[edx]
    mov ah, 0
    mov bl, 10
    div bl
    mov BYTE[edx], ah
    dec esi                   ; move ptr1
    dec edx                     ; move result ptr
    add BYTE[edx], al
    jmp .loop

.finish:
    mov eax, edx
    pop edx
    pop ebx
    pop esi
    ret

; 清空字符串
cleanstring:
    push eax
    push ebx
.loop:
    cmp ebx,0
    jz .rett
    mov BYTE[eax],0
    sub ebx,1
    inc eax
    jmp .loop
.rett:
    pop ebx
    pop eax

    ret

_start:
.run:
    ; 每次循环可能会留下上一次循环的数据，因此需要清空
    mov eax,input_string
    mov ebx,255
    call cleanstring

    mov eax,first_string
    call cleanstring

    mov eax,second_string
    call cleanstring

    mov eax,operate
    call cleanstring

    mov eax,judge
    call cleanstring

    mov eax,add_result
    call cleanstring

    mov eax,mul_result
    call cleanstring

    ; 输入字符串
    mov eax, input_string
    mov ebx, 255
    call gets

    mov ecx, input_string
    cmp BYTE[ecx], 71H ;判断是否输入q，若为q，则终止程序
    jz .exit

    mov eax, first_string ;得到第一个操作数
    call parse_input

    mov eax, second_string ;得到第二个操作数
    call parse_input

    mov ebx,judge
    cmp BYTE[ebx],1 ;无效输入
    jz .rerun

    mov ebx,operate ;操作符

    cmp BYTE[ebx],43 ;进行加法运算
    jz .choiceadd

    cmp BYTE[ebx],42 ;进行乘法运算
    jz .choicemul

; 无效输入
.rerun:
    mov eax,message
    call puts
    call endl
    jmp .run

; 选择加法
.choiceadd:
    jmp .start_add
; 选择乘法
.choicemul:
    jmp .start_mul
; 结束程序
.exit:
    mov eax, 1
    mov ebx, 0
    int 80h

; 进行加法运算
.start_add:
    mov ecx, 0
    mov edx, add_result
    add edx, 255
    xor eax, eax
    mov al, 10
    mov BYTE[edx], al

    mov eax, first_string      ; get len of the first number
    call strlen                 ; esi: ptr of num1
    mov esi, eax 
    add esi, first_string
    sub esi, 1

    mov eax, second_string      ; get len of the second number
    call strlen              ; edi: ptr of num2
    mov edi, eax
    add edi, second_string
    sub edi, 1

; 循环相加
.loopAdd:
    cmp esi, first_string
    jl .rest_second_digits
    cmp edi, second_string
    jl .rest_first_digits
    xor eax, eax ;清空
    add al, BYTE[esi]
    sub al, 48;因为是字符
    add al, BYTE[edi]          ; add by digits
    add al, cl                 ; add carry
    mov ecx, 0                 ; reset carry
    dec esi                     ; move ptr1
    dec edi                      ; move ptr2
    dec edx                      ; move result ptr
    cmp al, 57                 ; check if overflow occurs
    mov BYTE[edx], al
    jle .loopAdd             ; if not continue the loop
    call sub_digit             ; if so, call sub digit
    mov BYTE[edx], al
    jmp .loopAdd

.rest_first_digits:
    cmp esi, first_string
    jl .after_add
    xor eax, eax
    add al, BYTE[esi]           ; add by digits
    add al, cl                  ; add carry
    mov ecx, 0
    dec esi                    ; move ptr2
    dec edx                ; move result ptr
    mov BYTE[edx], al
    cmp al, 57
    jle .rest_first_digits
    call sub_digit
    mov BYTE[edx], al
    jmp .rest_first_digits

.rest_second_digits:
    cmp edi, second_string
    jl .after_add
    xor eax, eax
    add al, BYTE[edi]          ; add by digits
    add al, cl                 ; add carry
    mov ecx, 0
    dec edi                   ; move ptr2
    dec edx                     ; move result ptr
    mov BYTE[edx], al
    cmp al, 57
    jle .rest_second_digits
    call sub_digit
    mov BYTE[edx], al
    jmp .rest_second_digits

.add_carry:
    mov al, 49
    dec edx
    mov BYTE[edx], al
    jmp .output_add

.after_add:
    cmp ecx, 1
    jz .add_carry
    jmp .output_add

.output_add:
    mov eax, edx
    call puts
    jmp .run

; 进行乘法运算
.start_mul:
    mov edx, mul_result
    mov ecx, 0
    add edx, 255          ; edx: ptr of result      
    xor eax, eax
    mov al, 10
    mov BYTE[edx], al
    dec edx

    mov eax, first_string       ; get len of the first number
    call strlen                ; esi: ptr of num1
    mov esi, eax
    add esi, first_string
    sub esi, 1

    mov eax, second_string     ; get len of the second number
    call strlen                 ; edi: ptr of num2
    mov edi, eax
    add edi, second_string
    sub edi, 1                ; from tail

.outter_loopMul:
.loop:
    cmp edi, second_string      ; mul by digits
    jl .mul_output
    call inner_loopMul
    dec edi
    dec edx
    jmp .loop

.mul_output:
    mov edx, eax
    call format

.output_loop:
    cmp BYTE[edx], 48
    jnz .print_mul
    xor ecx, ecx
    add ecx, mul_result
    add ecx, 254
    cmp edx, ecx
    je .print_mul
    add edx, 1
    jmp .output_loop

.print_mul:
    mov eax, edx
    call puts
    jmp .run