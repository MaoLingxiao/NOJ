#ifndef JUDGER_CHILD_H
#define JUDGER_CHILD_H

#include "runner.h"

//子进程错误结束，并日志记录
#define CHILD_ERROR_EXIT(error_code)\
    {\
        LOG_ERROR(error_code);  \
        close_file(input_file, output_file, error_file);  \
        raise(SIGUSR1);  \
        return -1; \
    }

//子进程
int child_process(FILE *log_fp, struct config *_config);

#endif //JUDGER_CHILD_H
