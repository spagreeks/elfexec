#define _GNU_SOURCE
#include <features.h>

#ifndef FD_BUFFER_SIZE
#define FD_BUFFER_SIZE (8 * 1024)
#endif

#include <sys/mman.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>

extern char **environ;

// Pure Linux ELF magic bytes
const char MAGIC[4] = {0x7f, 'E', 'L', 'F'};

int eexit(const char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

int createfd(const char *name)
{
	// Natively supported on Linux 3.17+
	return memfd_create(name, MFD_CLOEXEC);
}

ssize_t writen(int fd, const char buf[], int n)
{
	ssize_t i = 0;
	for (ssize_t nw = 0; i < n;)
	{
		nw = write(fd, buf + i, n - i);
		if (nw < 0)
			return i;
		i += nw;
	}
	return i;
}

int findMagic(int srcfd, const char buf[])
{
	for (int i = 0, p = i % 4;; i++, p = i % 4)
	{
		ssize_t nr = read(srcfd, buf + p, 1);
		if (nr < 0)
			return eexit("read magic");
		if (nr == 0)
			return eexit("no magic found");

		int j = 0;
		for (; i >= 3 && j < 4; j++)
			if (buf[(i + j - 3) % 4] != MAGIC[j])
				break;
		if (j == 4)
			return i - 3;
	}
	return -1;
}

int closefd(int fd)
{
	if (fd == STDIN_FILENO)
		return -1;
	return close(fd);
}

bool isPipeMode()
{
	struct stat st;
	if (fstat(STDIN_FILENO, &st) == -1)
		return false;
	return st.st_mode & (S_IFIFO | S_IFREG);
}

int main(int argc, char *argv[])
{
	const char *dstfname;
	int srcfd, dstfd, magicoffset = 0;
	const char buf[FD_BUFFER_SIZE];
	const int ispipe = isPipeMode();

	if (ispipe)
		srcfd = STDIN_FILENO;
	else if (argc == 1)
		return eexit("no input provide");
	else
	{
		++argv; // shift out this app name
		srcfd = open(argv[0], O_RDONLY | O_CLOEXEC);
	}

	magicoffset += findMagic(srcfd, buf);

	dstfname = argv[0];
	if (magicoffset == 0 && !ispipe)
	{
		closefd(srcfd);
		if (execve(dstfname, argv, environ) == -1)
			return eexit("execve");
		return EXIT_SUCCESS;
	}

	dstfd = createfd(dstfname);

	if (dstfd == -1)
	{
		closefd(srcfd);
		return eexit("createfd");
	}
	if (writen(dstfd, MAGIC, 4) < 4)
		perror("writen MAGIC");

	for (ssize_t nr = 0;;)
	{
		nr = read(srcfd, buf, FD_BUFFER_SIZE);
		if (nr < 0)
			return eexit("read");
		if (nr == 0)
			break;
		if (writen(dstfd, buf, nr) < nr)
			perror("writen");
	}
	closefd(srcfd);

	if (fsync(dstfd) == -1)
		perror("fsync");

	// Beautiful, native in-memory execution via fexecve
	if (fexecve(dstfd, argv, environ) == -1)
	{
		closefd(dstfd);
		return eexit("fexecve");
	}

	return EXIT_SUCCESS;
}