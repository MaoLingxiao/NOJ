#define _GNU_SOURCE
#define _POSIX_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <signal.h>
#include <pthread.h>
#include <wait.h>
#include <errno.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>

#include "runner.h"
#include "killer.h"
#include "child.h"
#include "logger.h"

//初始化结果集合
//参数：结果集指针
void init_result(struct result *_result) {
	  //结果集合 结果和异常设置空
    _result->result = _result->error = SUCCESS;
    //cpu时间、实际时间和退出代码初始化
    _result->cpu_time = _result->real_time = _result->signal = _result->exit_code = 0;
    //内存记录设为空
    _result->memory = 0;
}

//参数：设置集合指针 结果集合指针
void run(struct config *_config, struct result *_result) {
    // init log fp
    //log日志指针 指向设置集合中的log路径
    FILE *log_fp = log_open(_config->log_path);

    // init result
    //调用初始化集合函数
    init_result(_result);


    // check whether current user is root
    //检查用户是否root
    uid_t uid = getuid();
    if (uid != 0) {
    	  //结束：需求root权限
        ERROR_EXIT(ROOT_REQUIRED);
    }

    // check args
    //检查参数正确性
    //限定cpu时间
    if ((_config->max_cpu_time < 1 && _config->max_cpu_time != UNLIMITED) ||
    	  //限定真实时间
        (_config->max_real_time < 1 && _config->max_real_time != UNLIMITED) ||
        //限定内存大小
        (_config->max_memory < 1 && _config->max_memory != UNLIMITED) ||
        //限定进程数
        (_config->max_process_number < 1 && _config->max_process_number != UNLIMITED) ||
        //限定输出大小
        (_config->max_output_size < 1 && _config->max_output_size != UNLIMITED)) {
        //结束：无效的设置集合
        ERROR_EXIT(INVALID_CONFIG);
    }

    // record current time
    //记录现在时间 起始时间、结束时间
    //Linux下时间函数：struct timeval结构体
    struct timeval start, end;
    //获取当日时间
    gettimeofday(&start, NULL);
		//fork出子pid
    pid_t child_pid = fork();

    // pid < 0 shows clone failed
    //fork结果
    //失败
    if (child_pid < 0) {
    	  //结束：fork失败
        ERROR_EXIT(FORK_FAILED);
    }
    //子进程返回0
    else if (child_pid == 0) {
    	  //子进程传入日志和设置集合
    	  //子进程运行
        child_process(log_fp, _config);
    }
    //父进程返回子进程id
    else if (child_pid > 0){
        // create new thread to monitor process running time
        //创建新线程监控进程运行时间
        //线程id
        pthread_t tid = 0;
        //检查设置真实时间
        if (_config->max_real_time != UNLIMITED) {
        	  //定义超时杀除进程组
            struct timeout_killer_args killer_args;
						//组超时时间设为设置组大真实时间
            killer_args.timeout = _config->max_real_time;
            //组进程id设为fork出进程id
            killer_args.pid = child_pid;
            //unix线程创建
            //参数：tid线程标识指针 属性 运行函数起始地址指针 运行函数参数
            //创建线程，不设置属性，运行超时杀除函数，传入函数参数
            //返回值 0：成功 其他：出错编号
            if (pthread_create(&tid, NULL, timeout_killer, (void *) (&killer_args)) != 0) {
            	  //线程创建失败，杀除子进程
                kill_pid(child_pid);
                //结束：线程创建失败
                ERROR_EXIT(PTHREAD_FAILED);
            }
        }

        //状态
        int status;
        //统计资源使用量
        struct rusage resource_usage;

        // wait for child process to terminate
        //等待子进程终止
        // on success, returns the process ID of the child whose state has changed;
        //成功返回改变状态的pid子进程标识符和
        // On error, -1 is returned.
        //错误返回-1
        //wait for等待 子进程中止
        //记录状态 和 进程使用量
        //如果失败ERROR_EXIT
        if (wait4(child_pid, &status, 0, &resource_usage) == -1) {
        	  //杀死进程
            kill_pid(child_pid);
            //结束：等待结束失败
            ERROR_EXIT(WAIT_FAILED);
        }

        // process exited, we may need to cancel timeout killer thread
        //进程退出时，需要终止超时杀除线程
        if (_config->max_real_time != UNLIMITED) {
        	  //结束
            if (pthread_cancel(tid) != 0) {
                // todo logging
            };
        }
				
				//记录子进程退出码
				//WEXITSTATUS WEXITSTATUS(status)取得子进程exit()返回的结束代码，一般会先用WIFEXITED 来判断是否正常结束才能使用此宏。
        _result->exit_code = WEXITSTATUS(status);
        //记录cpu时间 
        _result->cpu_time = (int) (resource_usage.ru_utime.tv_sec * 1000 +
                                  resource_usage.ru_utime.tv_usec / 1000 +
                                  resource_usage.ru_stime.tv_sec * 1000 +
                                  resource_usage.ru_stime.tv_usec / 1000);
				//记录内存使用量
        _result->memory = resource_usage.ru_maxrss * 1024;
        // get end time
        //记录结束时间
        gettimeofday(&end, NULL);
        //真实时间为结束时间和开始时间
        _result->real_time = (int) (end.tv_sec * 1000 + end.tv_usec / 1000 - start.tv_sec * 1000 - start.tv_usec / 1000);
        
				//结果集退出代码不为0 即正常结束
        if (_result->exit_code != 0) {
        	  //记录超时
            _result->result = RUNTIME_ERROR;
        }
        // if signaled
        //WIFSIGNALED(status)为非0 表明进程异常终止。 
        if (WIFSIGNALED(status) != 0) {
        	 //log:记录
            LOG_DEBUG(log_fp, "signal: %d", WTERMSIG(status));
            //WTERMSIG(status) 取得子进程因信号而中止的信号
            //记录信号
            _result->signal = WTERMSIG(status);
            //SIGSEGV是当一个进程执行了一个无效的内存引用，或发生段错误时发送给它的信号。
            //无效引用
            if (_result->signal == SIGSEGV) {
            	//比较内存设置
                if (_config->max_memory != UNLIMITED && _result->memory > _config->max_memory) {
                	  //记录就超出限制内存
                    _result->result = MEMORY_LIMIT_EXCEEDED;
                }
                else {
                	  //记录运行时异常
                    _result->result = RUNTIME_ERROR;
                }
            }
            //SIGUSR1自定义信号
            //
            else if(_result->signal == SIGUSR1) {
            	  //记录系统异常
                _result->result = SYSTEM_ERROR;
            }
            else {
            	 //运行时异常
                _result->result = RUNTIME_ERROR;
            }
        }
        else {
        	 //比较内存设置
            if (_config->max_memory != UNLIMITED && _result->memory > _config->max_memory) {
            	  //记录超时异常
                _result->result = MEMORY_LIMIT_EXCEEDED;
            }
        }
        //比较真实时间设置
        if (_config->max_real_time != UNLIMITED && _result->real_time > _config->max_real_time) {
        	  //记录真实时间超时异常
            _result->result = REAL_TIME_LIMIT_EXCEEDED;
        }
        //比较CPU时间设置
        if (_config->max_cpu_time != UNLIMITED && _result->cpu_time > _config->max_cpu_time) {
        	  //记录CPU时间超时
            _result->result = CPU_TIME_LIMIT_EXCEEDED;
        }
				//关闭log日志
        log_close(log_fp);
    }
}
