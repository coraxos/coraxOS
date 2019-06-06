#include "string.h"
#include "malloc.h"

size_t strlen(const char *str)
{
	int i = 0;
	while (str[i] != '\0') {
		i++;
	}
	return i;
}
size_t strnlen(const char *s, size_t maxlen)
{
	size_t x;
	for (x = 0; x < maxlen; x++) {
		if (s[x] == '\0') {
			break;
		}
	}
	return x;
}
void reverse(const char *str)
{
	char *start = str;
	char *end = str + strlen(str) - 1;
	char temp;

	while (end > start) {
		temp = *start;
		*start = *end;
		*end = temp;

		start++;
		end--;
	}
}

void itoa(uint32_t number, char *str, uint32_t base)
{
	if (number == 0) {
		str[0] = '0';
		str[1] = '\0';
		return;
	}
	uint32_t i = 0;
	uint32_t lowBytes = number; //& 0xFFFFFFFF;
	while (lowBytes != 0) {
		uint32_t rem = lowBytes % base;
		if (rem < 10) {
			str[i] = rem + '0';
		} else {
			str[i] = rem + '0';
		}
		lowBytes /= base;
		i++;
	}

	str[i] = '\0';

	reverse(str);
}
char *strcpy(char *dst, char *src)
{
	uint32_t len = strlen(src);
	uint32_t x;
	for (x = 0; x < len; x++) {
		dst[x] = src[x];
	}
	dst[x] = '\0';

	return dst;
}
int strcmp(const char *str1, const char *str2)
{
	char *p = str1;
	char *q = str2;

	while (*p && *p == *q) {
		p++;
		q++;
	}
	return (int)(*p - *q);
}
char *strdup(const char *s)
{
	size_t len = strlen(s) + 1;

	char *new_str = malloc(len);
	memcpy(new_str, s, len);

	return new_str;
}
int atoi(const char *nptr)
{
	int returnVal = 0;
	for (size_t x = 0; x < strlen(nptr); x++) {
		returnVal += nptr[x] - '0';
		returnVal *= 10;
	}

	return returnVal;
}

char *strncpy(char *dest, const char *src, size_t n)
{
	size_t x;
	for (x = 0; x < n; x++) {
		dest[x] = src[x];
		if (src[x] == '\0') {
			break;
		}
	}
	for (; x < n; x++) {
		dest[x] = '\0';
	}

	return dest;
}
char *strcat(char *dest, const char *src)
{
	char *destAddr = (char *)dest;
	while (*destAddr != '\0') {
		destAddr++;
	}
	char *srcAddr = (char *)src;
	while (*srcAddr != '\0') {
		*destAddr = *srcAddr;
		destAddr++;
		srcAddr++;
	}
	*destAddr = '\0';

	return dest;
}
char *strncat(char *dest, const char *src, size_t n)
{
	char *destAddr = (char *)dest + strlen(dest);

	char *srcAddr = (char *)src;
	size_t count = 0;
	while (*srcAddr != '\0' && count < n) {
		*destAddr = *srcAddr;
		destAddr++;
		srcAddr++;
		count++;
	}
	*destAddr = '\0';

	return dest;
}
int strncmp(const char *s1, const char *s2, size_t max_len)
{
	int n = max_len;
	char *p = s1;
	char *q = s2;

	while (n > 0 && *p && *p == *q) {
		n--;
		p++;
		q++;
	}
	if (n == 0) {
		return 0;
	}
	return (int)(*p - *q);
}
char *strchr(const char *s, int c)
{
	size_t len = strlen(s) + 1; //include '\0' at end of the string
	for (size_t x = 0; x < len; x++) {
		if (s[x] == c) {
			return &s[x];
		}
	}
	return NULL;
}
char *strrchr(const char *s, int c)
{
	size_t len = strlen(s) + 1; //include '\0' at end of the string
	for (size_t x = len; x >= 0; x--) {
		if (s[x] == c) {
			return (char *)&s[x];
		}
	}
	return NULL;
}
bool contains(const char *input, char c)
{
	size_t len = strlen(input);
	for (size_t i = 0; i < len; i++) {
		if (input[i] == c) {
			return true;
		}
	}
	return false;
}
size_t strspn(const char *s, const char *accept)
{
	//TODO, there is a faster algo
	size_t len = strlen(s);

	size_t numOccurences = 0;
	for (size_t x = 0; x < len; x++) {
		if (!contains(accept, s[x])) {
			break;
		}
		numOccurences++;
	}
	return numOccurences;
}
size_t strcspn(const char *s, const char *reject)
{
	//TODO, there is a faster algo
	size_t len = strlen(s);

	size_t numNotContains = 0;
	for (size_t x = 0; x < len; x++) {
		if (contains(reject, s[x])) {
			break;
		}
		numNotContains++;
	}
	return numNotContains;
}

char *strtok(char *s, const char *delim)
{
	//TODO TAKEN FROM MUSL
	static char *strtok_ptr;
	if (s == '\0' && !(s = strtok_ptr))
		return NULL;

	s += strspn(s, delim);
	if (*s == '\0')
		return strtok_ptr = 0;
	strtok_ptr = s + strcspn(s, delim);
	if (*strtok_ptr)
		*strtok_ptr++ = 0;
	else
		strtok_ptr = 0;

	return s;
}