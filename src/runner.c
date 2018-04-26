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

//��ʼ���������
//�����������ָ��
void init_result(struct result *_result) {
	  //������� ������쳣���ÿ�
    _result->result = _result->error = SUCCESS;
    //cpuʱ�䡢ʵ��ʱ����˳������ʼ��
    _result->cpu_time = _result->real_time = _result->signal = _result->exit_code = 0;
    //�ڴ��¼��Ϊ��
    _result->memory = 0;
}

//���������ü���ָ�� �������ָ��
void run(struct config *_config, struct result *_result) {
    // init log fp
    //log��־ָ�� ָ�����ü����е�log·��
    FILE *log_fp = log_open(_config->log_path);

    // init result
    //���ó�ʼ�����Ϻ���
    init_result(_result);


    // check whether current user is root
    //����û��Ƿ�root
    uid_t uid = getuid();
    if (uid != 0) {
    	  //����������rootȨ��
        ERROR_EXIT(ROOT_REQUIRED);
    }

    // check args
    //��������ȷ��
    //�޶�cpuʱ��
    if ((_config->max_cpu_time < 1 && _config->max_cpu_time != UNLIMITED) ||
    	  //�޶���ʵʱ��
        (_config->max_real_time < 1 && _config->max_real_time != UNLIMITED) ||
        //�޶��ڴ��С
        (_config->max_memory < 1 && _config->max_memory != UNLIMITED) ||
        //�޶�������
        (_config->max_process_number < 1 && _config->max_process_number != UNLIMITED) ||
        //�޶������С
        (_config->max_output_size < 1 && _config->max_output_size != UNLIMITED)) {
        //��������Ч�����ü���
        ERROR_EXIT(INVALID_CONFIG);
    }

    // record current time
    //��¼����ʱ�� ��ʼʱ�䡢����ʱ��
    //Linux��ʱ�亯����struct timeval�ṹ��
    struct timeval start, end;
    //��ȡ����ʱ��
    gettimeofday(&start, NULL);
		//fork����pid
    pid_t child_pid = fork();

    // pid < 0 shows clone failed
    //fork���
    //ʧ��
    if (child_pid < 0) {
    	  //������forkʧ��
        ERROR_EXIT(FORK_FAILED);
    }
    //�ӽ��̷���0
    else if (child_pid == 0) {
    	  //�ӽ��̴�����־�����ü���
    	  //�ӽ�������
        child_process(log_fp, _config);
    }
    //�����̷����ӽ���id
    else if (child_pid > 0){
        // create new thread to monitor process running time
        //�������̼߳�ؽ�������ʱ��
        //�߳�id
        pthread_t tid = 0;
        //���������ʵʱ��
        if (_config->max_real_time != UNLIMITED) {
        	  //���峬ʱɱ��������
            struct timeout_killer_args killer_args;
						//�鳬ʱʱ����Ϊ���������ʵʱ��
            killer_args.timeout = _config->max_real_time;
            //�����id��Ϊfork������id
            killer_args.pid = child_pid;
            //unix�̴߳���
            //������tid�̱߳�ʶָ�� ���� ���к�����ʼ��ַָ�� ���к�������
            //�����̣߳����������ԣ����г�ʱɱ�����������뺯������
            //����ֵ 0���ɹ� ������������
            if (pthread_create(&tid, NULL, timeout_killer, (void *) (&killer_args)) != 0) {
            	  //�̴߳���ʧ�ܣ�ɱ���ӽ���
                kill_pid(child_pid);
                //�������̴߳���ʧ��
                ERROR_EXIT(PTHREAD_FAILED);
            }
        }

        //״̬
        int status;
        //ͳ����Դʹ����
        struct rusage resource_usage;

        // wait for child process to terminate
        //�ȴ��ӽ�����ֹ
        // on success, returns the process ID of the child whose state has changed;
        //�ɹ����ظı�״̬��pid�ӽ��̱�ʶ����
        // On error, -1 is returned.
        //���󷵻�-1
        //wait for�ȴ� �ӽ�����ֹ
        //��¼״̬ �� ����ʹ����
        //���ʧ��ERROR_EXIT
        if (wait4(child_pid, &status, 0, &resource_usage) == -1) {
        	  //ɱ������
            kill_pid(child_pid);
            //�������ȴ�����ʧ��
            ERROR_EXIT(WAIT_FAILED);
        }

        // process exited, we may need to cancel timeout killer thread
        //�����˳�ʱ����Ҫ��ֹ��ʱɱ���߳�
        if (_config->max_real_time != UNLIMITED) {
        	  //����
            if (pthread_cancel(tid) != 0) {
                // todo logging
            };
        }
				
				//��¼�ӽ����˳���
				//WEXITSTATUS WEXITSTATUS(status)ȡ���ӽ���exit()���صĽ������룬һ�������WIFEXITED ���ж��Ƿ�������������ʹ�ô˺ꡣ
        _result->exit_code = WEXITSTATUS(status);
        //��¼cpuʱ�� 
        _result->cpu_time = (int) (resource_usage.ru_utime.tv_sec * 1000 +
                                  resource_usage.ru_utime.tv_usec / 1000 +
                                  resource_usage.ru_stime.tv_sec * 1000 +
                                  resource_usage.ru_stime.tv_usec / 1000);
				//��¼�ڴ�ʹ����
        _result->memory = resource_usage.ru_maxrss * 1024;
        // get end time
        //��¼����ʱ��
        gettimeofday(&end, NULL);
        //��ʵʱ��Ϊ����ʱ��Ϳ�ʼʱ��
        _result->real_time = (int) (end.tv_sec * 1000 + end.tv_usec / 1000 - start.tv_sec * 1000 - start.tv_usec / 1000);
        
				//������˳����벻Ϊ0 ����������
        if (_result->exit_code != 0) {
        	  //��¼��ʱ
            _result->result = RUNTIME_ERROR;
        }
        // if signaled
        //WIFSIGNALED(status)Ϊ��0 ���������쳣��ֹ�� 
        if (WIFSIGNALED(status) != 0) {
        	 //log:��¼
            LOG_DEBUG(log_fp, "signal: %d", WTERMSIG(status));
            //WTERMSIG(status) ȡ���ӽ������źŶ���ֹ���ź�
            //��¼�ź�
            _result->signal = WTERMSIG(status);
            //SIGSEGV�ǵ�һ������ִ����һ����Ч���ڴ����ã������δ���ʱ���͸������źš�
            //��Ч����
            if (_result->signal == SIGSEGV) {
            	//�Ƚ��ڴ�����
                if (_config->max_memory != UNLIMITED && _result->memory > _config->max_memory) {
                	  //��¼�ͳ��������ڴ�
                    _result->result = MEMORY_LIMIT_EXCEEDED;
                }
                else {
                	  //��¼����ʱ�쳣
                    _result->result = RUNTIME_ERROR;
                }
            }
            //SIGUSR1�Զ����ź�
            //
            else if(_result->signal == SIGUSR1) {
            	  //��¼ϵͳ�쳣
                _result->result = SYSTEM_ERROR;
            }
            else {
            	 //����ʱ�쳣
                _result->result = RUNTIME_ERROR;
            }
        }
        else {
        	 //�Ƚ��ڴ�����
            if (_config->max_memory != UNLIMITED && _result->memory > _config->max_memory) {
            	  //��¼��ʱ�쳣
                _result->result = MEMORY_LIMIT_EXCEEDED;
            }
        }
        //�Ƚ���ʵʱ������
        if (_config->max_real_time != UNLIMITED && _result->real_time > _config->max_real_time) {
        	  //��¼��ʵʱ�䳬ʱ�쳣
            _result->result = REAL_TIME_LIMIT_EXCEEDED;
        }
        //�Ƚ�CPUʱ������
        if (_config->max_cpu_time != UNLIMITED && _result->cpu_time > _config->max_cpu_time) {
        	  //��¼CPUʱ�䳬ʱ
            _result->result = CPU_TIME_LIMIT_EXCEEDED;
        }
				//�ر�log��־
        log_close(log_fp);
    }
}
