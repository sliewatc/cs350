#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/wait.h>
#include <lib.h>
#include <syscall.h>
#include <current.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include <copyinout.h>
#include "opt-A2.h"
#include <mips/trapframe.h>
#include <spl.h>
#include <synch.h>
#include <kern/fcntl.h>
#include <vfs.h>
/* this implementation of sys__exit does not do anything with the exit code */
/* this needs to be fixed to get exit() and waitpid() working properly */


void sys__exit(int exitcode) {
    struct addrspace *as;
    struct proc *p = curproc;
    
#if OPT_A2
    struct proc_info* pInfo;
    pInfo = getProcDataByPID(p->pid);
    // if(pInfo.exited == true){kprintf("exit already\n");}
    // if(pInfo.exited == false){kprintf("not exited\n");}
    
    
    pInfo->exited = true;
    //kprintf("encode start\n");
    pInfo->exitcode = _MKWAIT_EXIT(exitcode);
    //kprintf("encode end\n");
    V(pInfo->exitSem);
    
    // if(pInfo.exited == true){kprintf("exit already\n");}
    // if(pInfo.exited == false){kprintf("not exited\n");}
    //kprintf("call v()\n");
    
#else
    /* for now, just include this to keep the compiler from complaining about
	 an unused variable */
    (void)exitcode;
#endif /* OPT_A2 */
    
    DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);
    //kprintf("Syscall: _exit(%d)\n",exitcode);
    
    KASSERT(curproc->p_addrspace != NULL);
    as_deactivate();
    /*
     * clear p_addrspace before calling as_destroy. Otherwise if
     * as_destroy sleeps (which is quite possible) when we
     * come back we'll be calling as_activate on a
     * half-destroyed address space. This tends to be
     * messily fatal.
     */
    as = curproc_setas(NULL);
    as_destroy(as);
    
    /* detach this thread from its process */
    /* note: curproc cannot be used after this call */
    proc_remthread(curthread);
    
    /* if this is the last user process in the system, proc_destroy()
	 will wake up the kernel menu thread */
    proc_destroy(p);
    
    thread_exit();
    /* thread_exit() does not return, so we should never get here */
    panic("return from thread_exit in sys_exit\n");
}


/* stub handler for getpid() system call                */
int
sys_getpid(pid_t *retval)
{
    /* for now, this is just a stub that always returns a PID of 1 */
    /* you need to fix this to make it work properly */
#if OPT_A2
    *retval = curproc->pid;
    return(0);
#else
    *retval = 1;
    return(0);
#endif /* OPT_A2 */
}

/* stub handler for waitpid() system call                */

int
sys_waitpid(pid_t pid,
            userptr_t status,
            int options,
            pid_t *retval)
{
    int exitstatus;
    int result;
    
    /* this is just a stub implementation that always reports an
	 exit status of 0, regardless of the actual exit status of
	 the specified process.
	 In fact, this will return 0 even if the specified process
	 is still running, and even if it never existed in the first place.
     
	 Fix this!
     */
    
    if (options != 0) {
        return(EINVAL);
    }
    
#if OPT_A2
    struct proc_info* waitProcInfo;
    if (status == NULL){
        return EFAULT;
    }
    
    if (pid < PID_MIN || pid > PID_MAX){
        return ESRCH;
    }
    
    waitProcInfo = getProcDataByPID(pid);
    if (waitProcInfo == NULL){
        return ESRCH;
    }
    if (waitProcInfo->pid < 2){
        return ESRCH;
    }
    if (waitProcInfo->pid == curproc->parentPid){
        //kprintf("wait on parent!\n");
        return ECHILD;
    }
    // if(waitProcInfo->process->parentPid != curproc->pid){
    //   //kprintf("not wait on child!\n");
    //   return ECHILD;
    // }
    
    //kprintf("parent = %d , child = %d \n",waitProcInfo->process->parentPid, curproc->pid);
    if(waitProcInfo->exited == 0){
        //kprintf("call p()\n");
        P(waitProcInfo->exitSem);
        
    }
    exitstatus = waitProcInfo->exitcode;
    //kprintf("exit status = %d\n",exitstatus);
    
#else
    /* for now, just pretend the exitstatus is 0 */
    //exitstatus = 0;
#endif /* OPT_A2 */
    
    result = copyout((void *)&exitstatus,status,sizeof(int));
    if (result) {
        return(result);
    }
    *retval = waitProcInfo->pid;
    //kprintf("retval = %d\n",*retval);
#if OPT_A2
    pid_free(pid);
#endif /* OPT_A2 */
    
    return(0);
}



#if OPT_A2
//fork_entry
void fork_entry(void *tf, unsigned long as){
    //kprintf("IN fork entry\n");
    (void)as;
    struct trapframe copy_tf, *ptr_tf;
    ptr_tf = (struct trapframe *)tf;
    copy_tf = *ptr_tf;
    
    kfree(tf);
    
    //Set Return value
    copy_tf.tf_a3 = 0;
    copy_tf.tf_v0 = 0;
    
    //Increment PC
    copy_tf.tf_epc += 4;
    
    //Activate address space
    //curproc->p_addrspace = (struct addrspace *)as;
    as_activate();
    
    //enter user mode
    mips_usermode(&copy_tf);
}


//fork
int sys_fork(struct trapframe *tf, pid_t *retval){
    // (void)tf;
    struct trapframe *new_tf;
    struct addrspace *new_addrspace;
    struct proc *child_proc;
    int err, a;
    
    child_proc = proc_create_runprogram("child_proc");
    
    
    new_tf = kmalloc(sizeof(struct trapframe));
    if(new_tf == NULL)return ENOMEM;
    
    a = splhigh();
    
    *new_tf = *tf;
    
    err = as_copy(curproc->p_addrspace, &child_proc->p_addrspace);
    splx(a);
    if(err){
        proc_destroy(child_proc);
        return err;
    }
    
    err = thread_fork("child_thread", child_proc, fork_entry, new_tf, (unsigned long)new_addrspace);
    child_proc->parentPid = curproc->pid;
    if(err){
        as_destroy(child_proc->p_addrspace);
        proc_destroy(child_proc);
        return err;
    }
    *retval = child_proc->pid;
    return 0;
}

//execv
int sys_execv(struct trapframe *tf){
    char* progname;
    char** argv;
    char* kargv[20];
    char *ptrsarray[20];
    int argc = 0;
    
    
    struct addrspace *as;
    struct vnode *v;
    vaddr_t entrypoint, stackptr;
    int result, length;
    argv = (char**)tf->tf_a1;
    progname = (char*)tf->tf_a0;
    
    while(true){
        if(argv[argc] == '\0'){
            //kprintf("end of execv arg!!\n");
            break;
        }
        //kprintf("copying execv arg!!\n");
        length = strlen(argv[argc]) + 1;
        //kprintf("length1 = %d\n",length);

        kargv[argc] = kmalloc(sizeof(char) * length);
        for(int i = 0; i < length ; i++){
            kargv[argc][i] = '\0';
        }
        //kprintf("argsss = %s\n",argv[argc]);

        result = copyin((const_userptr_t)argv[argc], (void *)kargv[argc], length);
        if(result){
            return result;
        }
        
        argc++;
        if(argc >= 20){
            kprintf("damn...20 is not enough.!!\n");
        }
    }
    


    /* Open the file. */
    result = vfs_open(progname, O_RDONLY, 0, &v);
    if (result) {
        return result;
    }
    
    /* We should be a new process. */
    //KASSERT(curproc_getas() == NULL);
    
    /* Create a new address space. */
    as = curproc_getas();
    as_destroy(as);


    as = as_create();
    if (as == NULL) {
        vfs_close(v);
        return ENOMEM;
    }
    
    /* Switch to it and activate it. */
    curproc_setas(as);
    as_activate();
    
    /* Load the executable. */
    result = load_elf(v, &entrypoint);
    if (result) {
        /* p_addrspace will go away when curproc is destroyed */
        vfs_close(v);
        return result;
    }
    
    /* Done with the file now. */
    vfs_close(v);
    
    /* Define the user stack in the address space */
    result = as_define_stack(as, &stackptr);
    if (result) {
        /* p_addrspace will go away when curproc is destroyed */
        return result;
    }
    
    /* Warp to user mode. */
    
    
    
    for(int i = 0 ; i < argc ; i++){
        int length = strlen(kargv[i]) + 1;
        //kprintf("length 1 = %d\n",length);
        if(length % 4 != 0){length += (4 - (length % 4));}
        //kprintf("length 2 = %d\n",length);

        stackptr -= length;
        result = copyout((const void *)kargv[i],(userptr_t)stackptr,length);
    
        if(result){
            //err!!!
            return result;
        }
        ptrsarray[i] = (char *)stackptr;
        // kprintf("arg ptr = %s\n",ptrsarray[i]);
        // kprintf("arg = %s\n",kargv[i]);
        kfree(kargv[i]);
    }
    
        stackptr -= sizeof(char*)*(argc+1);
    
    for(int j = 0 ; j < argc ; j++){
        // kprintf("arg = %s\n",ptrsarray[j]);
        result = copyout(
                        (const void*)(&ptrsarray[j]), 
                        (userptr_t)(stackptr+sizeof(char*)*j), 
                        sizeof(char*)
                        );
        if(result){
            //err
            return result;
        }
    }
    //kprintf("so far so good!!\n");
    enter_new_process(argc, (userptr_t)stackptr, stackptr, entrypoint);
   

    //enter_new_process(0 /*argc*/, NULL /*userspace addr of argv*/,stackptr, entrypoint);
    /* enter_new_process does not return. */
    panic("enter_new_process returned\n");
    return EINVAL;
}

#endif /* OPT_A2 */

