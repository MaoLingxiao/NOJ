#ifndef JUDGER_LOGGER_H
#define JUDGER_LOGGER_H

//log等级
//致命
#define LOG_LEVEL_FATAL 0
//警告
#define LOG_LEVEL_WARNING 1
//信息
#define LOG_LEVEL_INFO 2
//debug
#define LOG_LEVEL_DEBUG 3

//打开日志操作
FILE *log_open(const char *);

//关闭日志操作
void log_close(FILE *);

//写日日志操作
//参数：日志等级 文件名 行号 日志文件 信息
void log_write(int level, const char *source_filename, const int line_number, const FILE *log_fp, const char *, ...);

//定义基本操作
#define LOG_DEBUG(log_fp, x...)   log_write(LOG_LEVEL_DEBUG, __FILE__, __LINE__, log_fp, ##x)
#define LOG_INFO(log_fp, x...)  log_write(LOG_LEVEL_INFO, __FILE__ __LINE__, log_fp, ##x)
#define LOG_WARNING(log_fp, x...) log_write(LOG_LEVEL_WARNING, __FILE__, __LINE__, log_fp, ##x)
#define LOG_FATAL(log_fp, x...)   log_write(LOG_LEVEL_FATAL, __FILE__, __LINE__, log_fp, ##x)

#endif //JUDGER_LOGGER_H
