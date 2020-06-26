#ifndef __LOG_H_
#define __LOG_H_

#define log_register(__X, __Y, __Z, __A, __B) {}
struct log {}; // Empty

#define LOG_INFO(__X, __Y, fmt, ...)                                     \
    pr_info("uwbcore: %s():%d: "fmt, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_WARN(__X, __Y, fmt, ...) LOG_INFO(__X, __Y, fmt, ##__VA_ARGS__)
#define LOG_ERR(__X, __Y, fmt, ...) LOG_INFO(__X, __Y, fmt, ##__VA_ARGS__)

#endif
