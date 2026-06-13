
#include <zephyr/kernel.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

size_t safe_strlcpy(char *dest, const char *src, size_t siz)
{
    const char *s = src;
    size_t left = siz;

    if (left)
    {
        /* Copy string up to the maximum size of the dest buffer */
        while (--left != 0)
        {
            if ((*dest++ = *s++) == '\0')
            {
                break;
            }
        }
    }

    if (left == 0)
    {
        /* Not enough room for the string; force NUL-termination */
        if (siz != 0)
        {
            *dest = '\0';
        }
        while (*s++)
        {
            ; /* determine total src string length */
        }
    }

    return s - src - 1;
}
int strnlen(char *s, int maxlen)
{
    int len = 0;
    while (len < maxlen && *s != '\0')
    {
        len++;
        s++;
    }
    return len;
}

bool is_ascii_str(const char *str, int max_len)
{
    int len = 0;
    while (*str != '\0')
    {
        if (*str < 0 || *str > 127)
        {
            return false;
        }
        str++;
        len++;
        if (len > max_len)
        {
            return false;
        }
    }
    return true;
}