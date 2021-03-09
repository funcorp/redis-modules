#ifndef MODULE_COMMON_H
#define MODULE_COMMON_H

#define EXPECTED(condition)   __builtin_expect(!!(condition), 1)
#define UNEXPECTED(condition) __builtin_expect(!!(condition), 0)

#endif //MODULE_COMMON_H
