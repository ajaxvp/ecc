#ifndef _LIBECC_LSYS_H
#define _LIBECC_LSYS_H

void* __ecc_lsys_mmap(void* addr, unsigned long length, int prot, int flags, int fd, long offset);
int __ecc_lsys_open(const char *pathname, int flags, unsigned mode);
int __ecc_lsys_close(int fd);
long __ecc_lsys_read(int fd, char* buf, unsigned long count);
long __ecc_lsys_write(int fd, const char* buf, unsigned long count);

#endif
