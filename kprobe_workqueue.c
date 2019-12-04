#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/kprobes.h>
#include <linux/ptrace.h>

static void mykmod_work_handler(struct work_struct *w);

static struct workqueue_struct *wq = 0;
static DECLARE_DELAYED_WORK(mykmod_work, mykmod_work_handler);
static unsigned long onesec;
static int run_in_loop = 1;

/*kprobes*/
static unsigned long probe_addr = mykmod_work_handler;
//module_param_named(probe_addr, probe_addr, ulong, S_IRUGO | S_IWUSR);
/*For each probe you need to allocate a kprobe structure*/
static struct kprobe kp;


static void dump_state(struct pt_regs *regs) {
    printk(KERN_INFO "EIP is at %s\n", regs->ip);
    printk(KERN_INFO "eax: %08lx ebx: %08lx ecx: %08lx edx: %08lx \n", regs->ax, regs->bx, regs->cx, regs->dx);
    printk(KERN_INFO "esi: %08lx edi: %08lx ebp: %08lx esp: %08lx\n", regs->si, regs->di, regs->bp, regs->sp);
    //printk(KERN_INFO "ds: %04x es: %04x\n", regs->ds & 0xffff, regs->es & 0xffff);
    printk(KERN_INFO "Process %s (pid: %d, threadinfo=%p task=%p)", current->comm, current->pid,current_thread_info(), current);
}

/*kprobe pre_handler: called just before the probed instruction is executed*/
int handler_pre(struct kprobe *p, struct pt_regs *regs) {
    printk("pre_handler: p->addr=0x%p\n", p->addr);
    dump_state(regs);
    dump_stack();
    return 0;
}

/*kprobe post_handler: called after the probed instruction is executed*/
void handler_post(struct kprobe *p, struct pt_regs *regs, unsigned long flags) {
    printk("post_handler: p->addr=0x%p\n", p->addr);
    dump_state(regs);
    return;
}

/* fault_handler: this is called if an exception is generated for any instruction 
within the pre- or post-handler, or when Kprobes single-steps the probed instruction.
*/
int handler_fault(struct kprobe *p, struct pt_regs *regs, int trapnr) {
    printk("fault_handler: p->addr=0x%p, trap #%dn",p->addr, trapnr);
    /* Return 0 because we don't handle the fault. */
    return 0;
}
static void printString(char *string) {

    struct tty_struct *tty;

    tty = get_current_tty();
    //tty = current->signal->tty;

    if(tty != NULL) { 

        (tty->driver->ops->write) (tty, string, strlen(string));
    }

    else
        printk("tty equals to zero");
}

static void mykmod_work_handler(struct work_struct *w)
{
        pr_info("mykmod work jiffies\n");
        if(run_in_loop && wq) {
        	queue_delayed_work(wq, &mykmod_work, onesec);
	}
	
}


static int __init mykmod_init(void)
{
        onesec = msecs_to_jiffies(1000);
        pr_info("mykmod loaded %u jiffies\n", (unsigned)onesec);

        if (!wq)
                wq = create_singlethread_workqueue("mykmod");
        if (wq)
                queue_delayed_work(wq, &mykmod_work, onesec);

	if (!probe_addr) {
		printk("trace-generic: provide probe_addr paramter\n"); return -1;
	}
	kp.pre_handler = handler_pre;
	kp.post_handler = handler_post;
	kp.fault_handler = handler_fault;
	kp.addr = (kprobe_opcode_t*) probe_addr;
	if (!kp.addr) {
		printk("Couldn't find the addr to plant kprobe\n");
		return -1;
	}
	int ret = 0;
	if ((ret = register_kprobe(&kp) < 0)) {
		printk("register_kprobe failed, returned %d\n", ret); return -1;
	}
        return 0;
}

static void __exit mykmod_exit(void)
{
	run_in_loop = 0;
        if (wq)
                destroy_workqueue(wq);
        pr_info("mykmod exit\n");
}

module_init(mykmod_init);
module_exit(mykmod_exit);

MODULE_DESCRIPTION("mykmod");
MODULE_LICENSE("GPL");
