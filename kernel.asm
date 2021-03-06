;;kernel.asm
;;made by Matt Perry
bits 32 ;;setting mode to 32 bit
section .text ;;start of the command list
    align 4
    dd 0x1BADB002 ;; magic number for allowing boot
    dd 0x00  ;; flags field set to zero
    dd - (0x1BADB002 + 0x00) ;; checksum that should set total between flag and magic nubmer back to zero.

global start
global keyboard_handler
global read_port
global write_port
global load_idt

extern kernelMain  ;; kernelMain is our main function defined in the c file
extern keyboard_handler_main

read_port:
	mov edx, [esp + 4] ;al is the lower 8 bits of eax
	in al, dx	         ;dx is the lower 16 bits of edx
	ret

write_port:
	mov   edx, [esp + 4]
	mov   al, [esp + 4 + 4]
	out   dx, al
	ret

load_idt:
	mov edx, [esp + 4]
	lidt [edx] ;;load global interrupts
	sti 				;turn on interrupts flags
	ret

keyboard_handler:
	call    keyboard_handler_main
	iretd ;;allows returns from our c program.

start:
    cli ;; clear all interrupts
    mov esp, stack_space ;; setting the stack pointer address
    call kernelMain ;;calling the kernelMain function
    hlt ;; halt the CPU only awakes on interrupts and interrupts are blocked.

    section .bss
    resb 8192 ;; creating 8kb space for the stack
    stack_space:
