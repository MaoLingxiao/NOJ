#ifndef JUDGER_KILLER_H
#define JUDGER_KILLER_H

//��ʱɱ��������
struct timeout_killer_args {
	  //pid����id
    int pid;
    //��ʱʱ��
    int timeout;
};

//����ɱ��
//���� pid����id
int kill_pid(pid_t pid);

//��ʱɱ��
//���� ��ʱɱ��������
void *timeout_killer(void *timeout_killer_args);

#endif //JUDGER_KILLER_H
