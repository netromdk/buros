#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <kernel/kbd.h>
#include <arch/i386/idt.h>
#include <arch/i386/pic.h>

namespace {
  void fillDesc(uint32_t offset, uint16_t selector, uint8_t type, IdtDesc *desc) {
    desc->offset_low  =  offset & 0x0000FFFF;
    desc->offset_high = (offset & 0xFFFF0000) >> 16;
    
    desc->selector = selector;
    desc->zero = 0;
    desc->type = type;
  }
}

IdtDesc idt[IDT_SIZE];
IdtReg idtr;

// Wrappers are defined in isr.s and will call methods here.
extern "C" {
  void asmIntDummy();
  void intDummy() {
    Pic::sendEoi();
  }

  void asmExcDivZero();
  void excDivZero() {
    printf("Divide by zero!\n");
    abort();
  }

  void asmExcInvOp();
  void excInvOp() {
    printf("Invalid opcode!\n");
    abort();
  }

  void asmExcSegNp();
  void excSegNp() {
    printf("Segment not present!\n");
    abort();
  }

  void asmExcSf();
  void excSf() {
    printf("Stack fault!\n");
    abort();
  }

  void asmExcGp();
  void excGp() {
    printf("General protection exception!\n");
    abort();
  }

  void asmExcPf();
  void excPf() {
    printf("Page fault!\n");
    abort();
  }

  void asmIntTick();
  void intTick() {
    /*
    static uint32_t ticks = 0;
    ticks++;
    printf("ticks: %u\n", ticks);
    */
    Pic::sendEoi();
  }

  void asmIntKbd();
  void intKbd() {
    Kbd::readScanCode();
    Pic::sendEoi();
  }
}

void Idt::init() {
  // Interrupt vectors:
  //   0x00 -> 0x1F        are for CPU exceptions.
  //   0x20 -> 0x27 (32->) are for master interrupts.
  //   0x28 -> 0x2F (40->) are for slave interrupts.
  // (Defined pic.cpp)

  // Initialize IRQ (interrupt requests) with dummy routines because
  // the entries must be defined.
  for (size_t i = 0; i < IDT_SIZE; i++) {
    fillDesc((uint32_t) asmIntDummy, IRQ_TIMER, INTR_GATE, &idt[i]);
  }

  // Exceptions.
  fillDesc((uint32_t) asmExcDivZero, IRQ_TIMER, INTR_GATE, &idt[0]);
  fillDesc((uint32_t) asmExcInvOp, IRQ_TIMER, INTR_GATE, &idt[6]);
  fillDesc((uint32_t) asmExcSegNp, IRQ_TIMER, INTR_GATE, &idt[11]);
  fillDesc((uint32_t) asmExcSf, IRQ_TIMER, INTR_GATE, &idt[12]);
  fillDesc((uint32_t) asmExcGp, IRQ_TIMER, INTR_GATE, &idt[13]);
  fillDesc((uint32_t) asmExcPf, IRQ_TIMER, INTR_GATE, &idt[14]);

  // Master interrupts.
  fillDesc((uint32_t) asmIntTick, IRQ_TIMER, INTR_GATE, &idt[32]); // 0: Hardware timer
  fillDesc((uint32_t) asmIntKbd, IRQ_TIMER, INTR_GATE, &idt[33]); // 1: Keyboard

  // Create idt register and put it at the base memory address.
  idtr.limit = IDT_SIZE * sizeof(IdtDesc);
  idtr.base = IDT_BASE;
  memcpy((void*) idtr.base, (void*) idt, idtr.limit);

  // Load idt.
  __asm__("lidtl (idtr)");
}
