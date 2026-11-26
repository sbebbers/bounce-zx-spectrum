#include <stdio.h>
#include <input.h>
#include <arch/zx.h>
#include <intrinsic.h>
/*
 * Global variables
 */
unsigned char sprite_shift;
unsigned char sprite_height;
unsigned char main_sprite[16];
unsigned short* sprite_data = main_sprite;
unsigned short screen_addr;
unsigned short pitch;
unsigned short duration;

/*
 * Forward declarations
 */
int main(void);
void setPoint(unsigned char x, unsigned char y, unsigned char h);
extern void drawSprite(void);
extern void beep(void);

/**
 * Main
 */
int main(void)
{
    intrinsic_ei();
    zx_cls(PAPER_BLACK | INK_WHITE);

    // main_sprite[ 0] = 0b00000000;
    // main_sprite[ 1] = 0b00000000;
    // main_sprite[ 2] = 0b00000000;
    // main_sprite[ 3] = 0b00000000;
    // main_sprite[ 4] = 0b00000000;
    // main_sprite[ 5] = 0b00000000;
    // main_sprite[ 6] = 0b00000000;
    // main_sprite[ 7] = 0b00000000;
    main_sprite[00] = 0b00000000;
    main_sprite[01] = 0b00011000;
    main_sprite[02] = 0b00111100;
    main_sprite[03] = 0b01111110;
    main_sprite[04] = 0b01111110;
    main_sprite[05] = 0b00111100;
    main_sprite[06] = 0b00011000;
    main_sprite[07] = 0b00000000;

    unsigned char _x = 1;
    unsigned char _y = 1;
    unsigned char _h = 8;

    signed char _xDirection = 1;
    signed char _yDirection = 1;

    // Example beeper code
    pitch = 0x00a0;
    duration = 0x0600;

    beep();

    while(1)
    {
        zx_border(1);
        _x = _x + _xDirection;
        _y = _y + _yDirection;
        if (_x < 1 || _x > 247) _xDirection = -_xDirection;
        if (_y < 1 || _y > 190) _yDirection = -_yDirection;

        for(int i = 0; i < 2; i++)
        {
            intrinsic_halt();
        }
        setPoint(_x, _y, _h);
    }
}

/**
 * Sets point for sprite, drawing from the bottom of a 16-byte sprite upwards
 * If height is zero, assumes height of 16 scan lines, else the sprite is the
 * height of h scan lines
 *
 * @param unsigned char x - the x position
 * @param unsigned char y - the y position
 * @param unsigned char h - the height of the char | zero means default height of 16
 * @return void
 * @author Shaun B
 */
void setPoint(unsigned char x, unsigned char y, unsigned char h)
{
    if (y > 191)
    {
        y = 191;
    }
    if (!h)
    {
        if (y >= 15)
        {
            sprite_height = 16;
        }
        else
        {
            sprite_height = h + 1;
        }
    }
    else
    {
        if (y >= h)
        {
            sprite_height = h;
        }
        else
        {
            sprite_height = h + 1;
        }
    }

    sprite_shift  = x & 7;

    screen_addr = 0x4000 + ((y & 0x07) << 8) + ((y & 0x38) << 2) + ((y & 0xC0) << 5) + (x >> 3);
    drawSprite();
}

/**
 * Draw sprite one byte across and h scan lines high (set by sprite_height global)
 *
 * @param void
 * @return void
 * @author Shaun B
 */
void drawSprite(void)
{
    __asm
        ld hl, (_screen_addr)     ; Screen address
        ld de, (_sprite_data)     ; Sprite data
        ld a, (_sprite_height)    ; Speite height (cropped near the top of screen)
        ld b, a                   ; Number of lines to draw

draw_loop:
        push bc                   ; Line counter pushed

        ld a, (_sprite_shift)
        ld c, a

        ld a, (de)
        inc de

        ld b, c                   ; Shift amount
        ld c, 0                   ; C will overflow the bits

        inc b
        dec b
        jr z, no_shift

shift_loop:
        srl a                     ; Shift sprite right
        rr  c                     ; Bit --> C
        djnz shift_loop

no_shift:
        ld (hl), a                ; Write byte
        inc hl
        ld a, c
        ld (hl), a                ; Write overflow bits
        dec hl                    ; restore HL

move_up_scanline:
        ld a, h
        dec h
        and 0x07
        jr nz, same_zone

        ld a, l
        sub 0x20
        ld l, a
        jr c, same_zone

        ld a, h
        add a, 0x08
        ld h, a

same_zone:
        pop bc                      ; Reset line counter
        djnz draw_loop
    __endasm;
}

/**
 * Beep routine; set global pitch and duration as follows:
 *   pitch -> lower value is higher pitch
 *   duration -> lower value is shorter beep
 *
 * @param void
 * @return void
 * @author Ped7g
 */
void beep(void)
{
    __asm
        di
        ld a, (0x5C48)              ; border colour in a
        rra
        rra
        rra
        and 0x07
        ld c,a                      ; c -> permanent border colour

        ld hl, (_duration)          ; duration
        ld de, (_pitch)             ; Pitch

beep_loop:
        ld a,c
        xor 0x10                    ; toggle speaker
        out (0xfe),a
        ld c,a                      ; keep the new speaker state + border

        push de
delay:
        dec de
        ld a,d
        or e
        jr nz,delay
        pop de

        dec hl
        ld a,h
        or l
        jr nz,beep_loop
    __endasm;
}
