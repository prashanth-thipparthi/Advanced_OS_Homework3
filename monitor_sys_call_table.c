#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/delay.h>
#include <asm/paravirt.h>
#include <linux/kallsyms.h>
#include <linux/module.h>
#include <asm/unistd.h>
#include <linux/unistd.h>
#include <asm/syscalls.h>
#include <asm/asm-offsets.h>
#include <linux/sys.h>
#include <linux/fs.h>

#define SYS_CALL_ENTRY 333
#define MAX_ENTRIES 548 //0 - 334

//   /home/kernel/linux-hwe-4.15.0/arch/x86/entry/syscalls - Can see what entries are blank. 

MODULE_LICENSE("GPL");

unsigned long original_cr0;
static unsigned long **p_sys_call_table;
unsigned long syscall_address[MAX_ENTRIES];

static void mykmod_work_handler(struct work_struct *w);

static struct workqueue_struct *wq = 0;
static DECLARE_DELAYED_WORK(mykmod_work, mykmod_work_handler);
static unsigned long onesec;
static int run_in_loop = 1;


static void
mykmod_work_handler(struct work_struct *w)
{
        pr_info("mykmod work %u jiffies\n", (unsigned)onesec);
        if(run_in_loop && wq) {
        	queue_delayed_work(wq, &mykmod_work, onesec);
	}
}

asmlinkage int (*original_syscall)(void);

asmlinkage int custom_call(void){
	printk(KERN_ALERT "In custom call: System call start address: %p\n", p_sys_call_table[0]);
	printk(KERN_ALERT "Max: %d", __NR_syscall_max);	
	int i;

	for(i = 0; i < MAX_ENTRIES; i++) {
		if(syscall_address[i] == (unsigned long) p_sys_call_table[i]) {
			printk(KERN_ALERT "Same --> Original: %p, modified: %p, syscall number: %d\n", (unsigned long) syscall_address[i], (unsigned long) p_sys_call_table[i], i); 
		}
		else {
			printk(KERN_ALERT "Modified spaces --> Original: %p, modified: %p, syscall number: %d\n", (void *) syscall_address[i], (void *)p_sys_call_table[i], i); 

		}
	}
	

	return 0;
}

int __init init_module(void) 
{
	p_sys_call_table = (void *) kallsyms_lookup_name("sys_call_table");
	printk(KERN_ALERT "system call start address: %p\n", p_sys_call_table);

	original_cr0 = read_cr0();
	
	write_cr0(original_cr0 & ~0x00010000);
	
	original_syscall = (void *)p_sys_call_table[SYS_CALL_ENTRY];
	//syscall(333);
	
	printk(KERN_ALERT "our system call loaded : %p\n", original_syscall);
	//ref_sys_read = (void *)p_sys_call_table[__NR_read];
	p_sys_call_table[SYS_CALL_ENTRY] = (void *)custom_call;
	write_cr0(original_cr0);
	
	
	int i;
	for(i = 0; i < MAX_ENTRIES; i++) {
		syscall_address[i] = p_sys_call_table[i];
	}

	onesec = msecs_to_jiffies(1000);
        pr_info("mykmod loaded %u jiffies\n", (unsigned)onesec);

        if (!wq)
                wq = create_singlethread_workqueue("mykmod");
        if (wq)
                queue_delayed_work(wq, &mykmod_work, onesec);
	return 0;
}

void cleanup_module(void) 
{
	if(!p_sys_call_table) {
		return;
	}
	write_cr0(original_cr0 & ~0x00010000);
	p_sys_call_table[SYS_CALL_ENTRY] = (unsigned long *)original_syscall;
	write_cr0(original_cr0);
	run_in_loop = 0;

	flush_workqueue(wq);
        if (wq)
            destroy_workqueue(wq);
        pr_info("mykmod exit\n");
}
