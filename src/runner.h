 	#ifndef JUDGER_RUNNER_H
#define JUDGER_RUNNER_H

#include <sys/types.h>
#include <stdio.h>

// (ver >> 16) & 0xff, (ver >> 8) & 0xff, ver & 0xff  -> real version
#define VERSION 0x020001
//无限制
#define UNLIMITED -1
//日志记录
#define LOG_ERROR(error_code) LOG_FATAL(log_fp, "Error: "#error_code);
//异常退出
#define ERROR_EXIT(error_code)\
    {\
        LOG_ERROR(error_code);  \
        _result->error = error_code; \
        log_close(log_fp);  \
        return; \
    }

//执行状态
enum {
	  //成功
    SUCCESS = 0,
    //无效设置
    INVALID_CONFIG = -1,
    //fork失败
    FORK_FAILED = -2,
    //读取pth失败
    PTHREAD_FAILED = -3,
    //wait失败
    WAIT_FAILED = -4,
    //请求root权限
    ROOT_REQUIRED = -5,
    //加载Seccomp失败
    LOAD_SECCOMP_FAILED = -6,
    //限制设置失败
    SETRLIMIT_FAILED = -7,
    //转述错误
    DUP2_FAILED = -8,
    //UID设置失败
    SETUID_FAILED = -9,
    //执行失败
    EXECVE_FAILED = -10,
    //
    SPJ_ERROR = -11
};

//设置集合
struct config {
	  //最大cpu运行时间
    int max_cpu_time;
    //最大真实运行时间
    int max_real_time;
    //最大内存
    long max_memory;
    //最大进程数
    int max_process_number;
    //最大输出大小
    long max_output_size;
    //执行代码
    char *exe_path;
    //输入
    char *input_path;
    //输出
    char *output_path;
    //错误
    char *error_path;
    //参数
    char *args[256];
    //环境
    char *env[256];
    //日志
    char *log_path;
    //安全机制
    char *seccomp_rule_name;
    //linux uid 用户id
    uid_t uid;
    //linix gid 群体身份id
    gid_t gid;
};

//结果集
enum {
	  //错误代码
    WRONG_ANSWER = -1,
    //cpu运行超时
    CPU_TIME_LIMIT_EXCEEDED = 1,
    //真实时间超时
    REAL_TIME_LIMIT_EXCEEDED = 2,
    //内存溢出
    MEMORY_LIMIT_EXCEEDED = 3,
    //运行异常
    RUNTIME_ERROR = 4,
    //系统异常
    SYSTEM_ERROR = 5
};


struct result {
	  //cpu运行时间
    int cpu_time;
    //实际实际
    int real_time;
    //内存大小
    long memory;
    //信号
    int signal;
    //退出代码
    int exit_code;
    //错误代码
    int error;
    //结果
    int result;
};


void run(struct config *, struct result *);
#endif //JUDGER_RUNNER_H
