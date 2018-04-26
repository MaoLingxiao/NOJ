 	#ifndef JUDGER_RUNNER_H
#define JUDGER_RUNNER_H

#include <sys/types.h>
#include <stdio.h>

// (ver >> 16) & 0xff, (ver >> 8) & 0xff, ver & 0xff  -> real version
#define VERSION 0x020001
//������
#define UNLIMITED -1
//��־��¼
#define LOG_ERROR(error_code) LOG_FATAL(log_fp, "Error: "#error_code);
//�쳣�˳�
#define ERROR_EXIT(error_code)\
    {\
        LOG_ERROR(error_code);  \
        _result->error = error_code; \
        log_close(log_fp);  \
        return; \
    }

//ִ��״̬
enum {
	  //�ɹ�
    SUCCESS = 0,
    //��Ч����
    INVALID_CONFIG = -1,
    //forkʧ��
    FORK_FAILED = -2,
    //��ȡpthʧ��
    PTHREAD_FAILED = -3,
    //waitʧ��
    WAIT_FAILED = -4,
    //����rootȨ��
    ROOT_REQUIRED = -5,
    //����Seccompʧ��
    LOAD_SECCOMP_FAILED = -6,
    //��������ʧ��
    SETRLIMIT_FAILED = -7,
    //ת������
    DUP2_FAILED = -8,
    //UID����ʧ��
    SETUID_FAILED = -9,
    //ִ��ʧ��
    EXECVE_FAILED = -10,
    //
    SPJ_ERROR = -11
};

//���ü���
struct config {
	  //���cpu����ʱ��
    int max_cpu_time;
    //�����ʵ����ʱ��
    int max_real_time;
    //����ڴ�
    long max_memory;
    //��������
    int max_process_number;
    //��������С
    long max_output_size;
    //ִ�д���
    char *exe_path;
    //����
    char *input_path;
    //���
    char *output_path;
    //����
    char *error_path;
    //����
    char *args[256];
    //����
    char *env[256];
    //��־
    char *log_path;
    //��ȫ����
    char *seccomp_rule_name;
    //linux uid �û�id
    uid_t uid;
    //linix gid Ⱥ�����id
    gid_t gid;
};

//�����
enum {
	  //�������
    WRONG_ANSWER = -1,
    //cpu���г�ʱ
    CPU_TIME_LIMIT_EXCEEDED = 1,
    //��ʵʱ�䳬ʱ
    REAL_TIME_LIMIT_EXCEEDED = 2,
    //�ڴ����
    MEMORY_LIMIT_EXCEEDED = 3,
    //�����쳣
    RUNTIME_ERROR = 4,
    //ϵͳ�쳣
    SYSTEM_ERROR = 5
};


struct result {
	  //cpu����ʱ��
    int cpu_time;
    //ʵ��ʵ��
    int real_time;
    //�ڴ��С
    long memory;
    //�ź�
    int signal;
    //�˳�����
    int exit_code;
    //�������
    int error;
    //���
    int result;
};


void run(struct config *, struct result *);
#endif //JUDGER_RUNNER_H
