#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/tty.h>

static void mykmod_work_handler(struct work_struct *w);

static struct workqueue_struct *wq = 0;
static DECLARE_DELAYED_WORK(mykmod_work, mykmod_work_handler);
static unsigned long onesec;
static int run_in_loop = 1;

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

        return 0;
}

static void __exit mykmod_exit(void)
{
	run_in_loop = 0;
 	cancel_delayed_work(&mykmod_work);
        flush_workqueue(wq);
        if (wq)
                destroy_workqueue(wq);
        pr_info("mykmod exit\n");
}

module_init(mykmod_init);
module_exit(mykmod_exit);

MODULE_DESCRIPTION("mykmod");
MODULE_LICENSE("GPL");
