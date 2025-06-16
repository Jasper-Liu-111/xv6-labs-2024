#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[], userret[];

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();

void
trapinit(void)
{
  initlock(&tickslock, "time");
}

// set up to take exceptions and traps while in the kernel.
void
trapinithart(void)
{
  w_stvec((uint64)kernelvec);
}

//
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
//
void
usertrap(void)
{
  int which_dev = 0;

  if((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  // send interrupts and exceptions to kerneltrap(),
  // since we're now in the kernel.
  w_stvec((uint64)kernelvec);

  struct proc *p = myproc();
  
  // save user program counter.
  p->trapframe->epc = r_sepc();
  
  if(r_scause() == 8){
    // system call

    if(killed(p))
      exit(-1);

    // sepc points to the ecall instruction,
    // but we want to return to the next instruction.
    p->trapframe->epc += 4;

    // an interrupt will change sepc, scause, and sstatus,
    // so enable only now that we're done with those registers.
    intr_on();

    syscall();
    if (p->sigreturn) {
      // If sigreturn has been called, restore the saved user state.
      p->trapframe->epc = p->sigstate.epc;
      p->trapframe->ra = p->sigstate.ra;
      p->trapframe->sp = p->sigstate.sp;
      p->trapframe->gp = p->sigstate.gp;
      p->trapframe->tp = p->sigstate.tp;
      p->trapframe->t0 = p->sigstate.t0;
      p->trapframe->t1 = p->sigstate.t1;
      p->trapframe->t2 = p->sigstate.t2;
      p->trapframe->s0 = p->sigstate.s0;
      p->trapframe->s1 = p->sigstate.s1;
      p->trapframe->a0 = p->sigstate.a0;
      p->trapframe->a1 = p->sigstate.a1;
      p->trapframe->a2 = p->sigstate.a2;
      p->trapframe->a3 = p->sigstate.a3;
      p->trapframe->a4 = p->sigstate.a4;
      p->trapframe->a5 = p->sigstate.a5;
      p->trapframe->a6 = p->sigstate.a6;
      p->trapframe->a7 = p->sigstate.a7;
      p->trapframe->s2 = p->sigstate.s2;
      p->trapframe->s3 = p->sigstate.s3;
      p->trapframe->s4 = p->sigstate.s4;
      p->trapframe->s5 = p->sigstate.s5;
      p->trapframe->s6 = p->sigstate.s6;
      p->trapframe->s7 = p->sigstate.s7;
      p->trapframe->s8 = p->sigstate.s8;
      p->trapframe->s9 = p->sigstate.s9;
      p->trapframe->s10 = p->sigstate.s10;
      p->trapframe->s11 = p->sigstate.s11;
      p->trapframe->t3 = p->sigstate.t3;
      p->trapframe->t4 = p->sigstate.t4;
      p->trapframe->t5 = p->sigstate.t5;
      p->trapframe->t6 = p->sigstate.t6;
      p->sigreturn = 0;
    }
  } else if((which_dev = devintr()) != 0){
    // ok
    if (which_dev == 2) {
      if (p->siginterval != 0 && p->sigbusy == 0) {
        p->sigticks++;
        if (p->sigticks >= p->siginterval) {
          p->sigticks = 0;
          p->sigbusy = 1;
          // Save the user state before handling the signal.
          p->sigstate.epc = p->trapframe->epc;
          p->sigstate.ra = p->trapframe->ra;
          p->sigstate.sp = p->trapframe->sp;
          p->sigstate.gp = p->trapframe->gp;
          p->sigstate.tp = p->trapframe->tp;
          p->sigstate.t0 = p->trapframe->t0;
          p->sigstate.t1 = p->trapframe->t1;
          p->sigstate.t2 = p->trapframe->t2;
          p->sigstate.s0 = p->trapframe->s0;
          p->sigstate.s1 = p->trapframe->s1;  
          p->sigstate.a0 = p->trapframe->a0;
          p->sigstate.a1 = p->trapframe->a1;
          p->sigstate.a2 = p->trapframe->a2;
          p->sigstate.a3 = p->trapframe->a3;
          p->sigstate.a4 = p->trapframe->a4;
          p->sigstate.a5 = p->trapframe->a5;
          p->sigstate.a6 = p->trapframe->a6;
          p->sigstate.a7 = p->trapframe->a7;
          p->sigstate.s2 = p->trapframe->s2;
          p->sigstate.s3 = p->trapframe->s3;
          p->sigstate.s4 = p->trapframe->s4;
          p->sigstate.s5 = p->trapframe->s5;
          p->sigstate.s6 = p->trapframe->s6;
          p->sigstate.s7 = p->trapframe->s7;
          p->sigstate.s8 = p->trapframe->s8;
          p->sigstate.s9 = p->trapframe->s9;
          p->sigstate.s10 = p->trapframe->s10;
          p->sigstate.s11 = p->trapframe->s11;
          p->sigstate.t3 = p->trapframe->t3;
          p->sigstate.t4 = p->trapframe->t4;
          p->sigstate.t5 = p->trapframe->t5;
          p->sigstate.t6 = p->trapframe->t6;
          p->trapframe->epc = p->sighandler;
        }
      }
    }
  } else {
    printf("usertrap(): unexpected scause 0x%lx pid=%d\n", r_scause(), p->pid);
    printf("            sepc=0x%lx stval=0x%lx\n", r_sepc(), r_stval());
    setkilled(p);
  }

  if(killed(p))
    exit(-1);

  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2)
    yield();

  usertrapret();
}

//
// return to user space
//
void
usertrapret(void)
{
  struct proc *p = myproc();

  // we're about to switch the destination of traps from
  // kerneltrap() to usertrap(), so turn off interrupts until
  // we're back in user space, where usertrap() is correct.
  intr_off();

  // send syscalls, interrupts, and exceptions to uservec in trampoline.S
  uint64 trampoline_uservec = TRAMPOLINE + (uservec - trampoline);
  w_stvec(trampoline_uservec);

  // set up trapframe values that uservec will need when
  // the process next traps into the kernel.
  p->trapframe->kernel_satp = r_satp();         // kernel page table
  p->trapframe->kernel_sp = p->kstack + PGSIZE; // process's kernel stack
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = r_tp();         // hartid for cpuid()

  // set up the registers that trampoline.S's sret will use
  // to get to user space.
  
  // set S Previous Privilege mode to User.
  unsigned long x = r_sstatus();
  x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE; // enable interrupts in user mode
  w_sstatus(x);

  // set S Exception Program Counter to the saved user pc.
  w_sepc(p->trapframe->epc);

  // tell trampoline.S the user page table to switch to.
  uint64 satp = MAKE_SATP(p->pagetable);

  // jump to userret in trampoline.S at the top of memory, which 
  // switches to the user page table, restores user registers,
  // and switches to user mode with sret.
  uint64 trampoline_userret = TRAMPOLINE + (userret - trampoline);
  ((void (*)(uint64))trampoline_userret)(satp);
}

// interrupts and exceptions from kernel code go here via kernelvec,
// on whatever the current kernel stack is.
void 
kerneltrap()
{
  int which_dev = 0;
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();
  
  if((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");
  if(intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  if((which_dev = devintr()) == 0){
    // interrupt or trap from an unknown source
    printf("scause=0x%lx sepc=0x%lx stval=0x%lx\n", scause, r_sepc(), r_stval());
    panic("kerneltrap");
  }

  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2 && myproc() != 0)
    yield();

  // the yield() may have caused some traps to occur,
  // so restore trap registers for use by kernelvec.S's sepc instruction.
  w_sepc(sepc);
  w_sstatus(sstatus);
}

void
clockintr()
{
  if(cpuid() == 0){
    acquire(&tickslock);
    ticks++;
    wakeup(&ticks);
    release(&tickslock);
  }

  // ask for the next timer interrupt. this also clears
  // the interrupt request. 1000000 is about a tenth
  // of a second.
  w_stimecmp(r_time() + 1000000);
}

// check if it's an external interrupt or software interrupt,
// and handle it.
// returns 2 if timer interrupt,
// 1 if other device,
// 0 if not recognized.
int
devintr()
{
  uint64 scause = r_scause();

  if(scause == 0x8000000000000009L){
    // this is a supervisor external interrupt, via PLIC.

    // irq indicates which device interrupted.
    int irq = plic_claim();

    if(irq == UART0_IRQ){
      uartintr();
    } else if(irq == VIRTIO0_IRQ){
      virtio_disk_intr();
    } else if(irq){
      printf("unexpected interrupt irq=%d\n", irq);
    }

    // the PLIC allows each device to raise at most one
    // interrupt at a time; tell the PLIC the device is
    // now allowed to interrupt again.
    if(irq)
      plic_complete(irq);

    return 1;
  } else if(scause == 0x8000000000000005L){
    // timer interrupt.
    clockintr();
    return 2;
  } else {
    return 0;
  }
}

