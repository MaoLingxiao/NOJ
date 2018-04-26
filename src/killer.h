#ifndef JUDGER_KILLER_H
#define JUDGER_KILLER_H

//超时杀除进程组
struct timeout_killer_args {
	  //pid进程id
    int pid;
    //超时时间
    int timeout;
};

//进程杀除
//参数 pid进程id
int kill_pid(pid_t pid);

//超时杀除
//参数 超时杀除进程组
void *timeout_killer(void *timeout_killer_args);

#endif //JUDGER_KILLER_H
