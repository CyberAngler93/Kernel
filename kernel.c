// Created by Matthew Perry, Anthony Kolendo and Chris Seamount
// Basic Kernal that makes Kernal name on page
#include "keyboard_map.h"
#define LINES 25
#define COLUMNS_LINE 80
#define BYTES_ELEMENT 2
#define SCREENSIZE BYTES_ELEMENT * COLUMNS_LINE * LINES

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define IDT_SIZE 256
#define INTERRUPT_GATE 0x8e
#define KERNEL_CODE_SEGMENT_OFFSET 0x08

#define ENTER_KEY_CODE 0x1C
#define BACKSPACE_KEY_CODE 0X0E

extern unsigned char keyboard_map[128];
extern void keyboard_handler(void);
extern char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);
extern void load_idt(unsigned long *idt_ptr);

char *videoPtr = (char *) 0xb8000; //setting up video memory beginnning at 0xb8000
unsigned int dWindow = 0; // loop count for drawing video on screen.
unsigned int stringLocation = 0;
unsigned int cWindow = 0; // loop counter for clearing window/

struct IDT_entry {
    unsigned short int offset_lowerbits;
    unsigned short int selector;
    unsigned char zero;
    unsigned char type_attr;
    unsigned short int offset_higherbits;
};
struct IDT_entry IDT[IDT_SIZE];

void idt_init(void)
{
    unsigned long keyboard_address;
    unsigned long idt_address;
    unsigned long idt_ptr[2];

    /* populate IDT entry of keyboard's interrupt */
    keyboard_address = (unsigned long)keyboard_handler;
    IDT[0x21].offset_lowerbits = keyboard_address & 0xffff;
    IDT[0x21].selector = KERNEL_CODE_SEGMENT_OFFSET;
    IDT[0x21].zero = 0;
    IDT[0x21].type_attr = INTERRUPT_GATE;
    IDT[0x21].offset_higherbits = (keyboard_address & 0xffff0000) >> 16;

    /*     Ports
    *    PIC1   PIC2
    *Command 0x20   0xA0
    *Data    0x21   0xA1
    */

    /* ICW1 - begin initialization */
    write_port(0x20 , 0x11);
    write_port(0xA0 , 0x11);

    /* ICW2 - remap offset address of IDT */
    /*
    * In x86 protected mode, we have to remap the PICs beyond 0x20 because
    * Intel have designated the first 32 interrupts as "reserved" for cpu exceptions
    */
    write_port(0x21 , 0x20);
    write_port(0xA1 , 0x28);

    /* ICW3 - setup cascading */
    write_port(0x21 , 0x00);
    write_port(0xA1 , 0x00);

    /* ICW4 - environment info */
    write_port(0x21 , 0x01);
    write_port(0xA1 , 0x01);
    /* Initialization finished */

    /* mask interrupts */
    write_port(0x21 , 0xff);
    write_port(0xA1 , 0xff);

    /* fill the IDT descriptor */
    idt_address = (unsigned long)IDT ;
    idt_ptr[0] = (sizeof (struct IDT_entry) * IDT_SIZE) + ((idt_address & 0xffff) << 16);
    idt_ptr[1] = idt_address >> 16 ;

    load_idt(idt_ptr);
}
void newLine(){
    unsigned int lineSize = BYTES_ELEMENT*COLUMNS_LINE;
    dWindow = dWindow + (lineSize - dWindow % (lineSize));
}

void kb_init(void)
{
    /* 0xFD is 11111101 - enables only IRQ1 (keyboard)*/
    write_port(0x21 , 0xFD);
}

void keyboard_handler_main(void)
{
    unsigned char status;
    char keycode;

    /* write EOI */
    write_port(0x20, 0x20);

    status = read_port(KEYBOARD_STATUS_PORT);
    /* Lowest bit of status will be set if buffer is not empty */
    if (status & 0x01) {
        keycode = read_port(KEYBOARD_DATA_PORT);
        if(keycode < 0)
            return;

        if(keycode == ENTER_KEY_CODE) {
            newLine();
            dWindow+=2;
            return;
        }
        if((dWindow+2) % 160 == 0){
            newLine();
            dWindow+=2;
        }
        if(keycode == BACKSPACE_KEY_CODE){
            if(162%dWindow==0){
              return;
            }
            dWindow-=2;
            videoPtr[dWindow]= ' ';
            return;
        }
        videoPtr[dWindow++] = keyboard_map[(unsigned char) keycode];
        videoPtr[dWindow++] = 0x30;
    }
}

int stringLen(const char *str){
        int i = 0;
    while(str[i] != '\0'){
        i++;
    }
    if(i%2 == 1){
       i++;
    }
return i;
}

int centerLine(const char *str){
    int x = stringLen(str);
    return (80-x);
}

void sleep(){
    for(int i = 0; i < 2000000; i ++){
    }
    return;
}

void clear(){
    cWindow = 0;
    while (cWindow < 80*25*2) {
        //printing blank character
        videoPtr[cWindow] = ' ';
        //setting the attribute-byte - green on black screen
        videoPtr[cWindow+1] = 0x30;
        cWindow += 2;
    }
    cWindow = 0;
    dWindow = 0;
    stringLocation = 0;
}
void draw(const char *str){
  // writing string to video memory
    while(str[stringLocation] != '\0') {
        //setting char in str to char at dWindow in video memory
        videoPtr[dWindow] = str[stringLocation];
        //setting attiribute-byte -green on black screen
        videoPtr[dWindow+1] = 0x30;
        ++stringLocation;
        dWindow += 2;
    }
    stringLocation = 0;
}
void drawSlow(const char *str){
    while(str[stringLocation] != '\0') {
        sleep();
        //setting char in str to char at dWindow in video memory
        videoPtr[dWindow] = str[stringLocation];
        //setting attiribute-byte -green on black screen
        videoPtr[dWindow+1] = 0x30;
        ++stringLocation;
        dWindow += 2;
    }
    stringLocation = 0;
}



void centerScreen(){
    dWindow= 0;
    for(int i = 0; i < 12; i++){
        newLine();
    }

}
void blackLineTop(){
    cWindow = 158;
    while(cWindow > 0){
        sleep();
        videoPtr[cWindow] = ' ';
        videoPtr[cWindow+1] = 0x00;
        cWindow -=2;
    }
    videoPtr[cWindow] = ' ';
    videoPtr[cWindow+1] = 0x00;

}

void blackLineBottom(){
    cWindow = (2*80*24);
     while(cWindow < (80*2*25)){
        sleep();
        videoPtr[cWindow] = ' ';
        videoPtr[cWindow+1] = 0x00;
        cWindow+=2;
    }
}

void borderLeft(){
    cWindow = 160;
    for(int i = 0; i < 23; i++){
        sleep();
        videoPtr[cWindow] = ' ';
        videoPtr[cWindow+1] = 0x00;
        cWindow+=160;
    }
}

void borderRight(){
    for(int i = 0; i < 23; i++){
        sleep();
        cWindow -=162;
        videoPtr[cWindow] = ' ';
        videoPtr[cWindow+1] = 0x00;
        cWindow +=2;
    }
}


void drawBox(){
    blackLineTop();
    borderLeft();
    blackLineBottom();
    borderRight();
}
void kprint(const char *str)
{
    unsigned int i = 0;
    while (str[i] != '\0') {
        videoPtr[dWindow++] = str[i++];
        videoPtr[dWindow++] = 0x30;
    }
}




void kernalMain(){
    unsigned int x = 0;
    const char *str = "This is the Kernal Loading up..";
    clear();
    centerScreen();
    x = centerLine(str);
    dWindow += x;
    drawSlow(str);
    str = ".........";
    drawSlow(str);
    clear();
    //drawBox();
    str = "Welcome to OS Lite";
    centerScreen();
    dWindow += centerLine(str);
    drawSlow(str);
    str = "Home";
    newLine();
    dWindow += centerLine(str);
    draw(str);
    str = "Help";
    newLine();
    dWindow += centerLine(str);
    draw(str);
    str = "Created by Matt Chris and Anthony";
    newLine();
    newLine();
    newLine();
    dWindow += centerLine(str);
    draw(str);
    str = "";
    sleep();
    sleep();
    clear();
    drawBox();

    dWindow +=162;
    kprint(str);

    idt_init();
    kb_init();
    while(1);
    return;

}
