#ifndef __SAFE_STRLCPY_H__
#define __SAFE_STRLCPY_H__

size_t safe_strlcpy(char *dest, const char *src, size_t siz);
bool is_ascii_str(const char *str, int max_len);
int strnlen(char *s, int maxlen);
#endif //__SAFE_STRLCPY_H__