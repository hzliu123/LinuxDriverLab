#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>

const char *key_dev_name = "/dev/input/buttons";
const char *key_dev_name2 = "/dev/buttons";
const char *tty_dev_name = "/dev/tty";
int key_fd, tty_fd, max_fd;
int quit = 0;

int read_data(int fd)
{
	char data[80];
        int read_length;

	//printf("fd: %d\n", fd);
	/* Read all data until read() returns 0 */
	do {
		memset(data, 0, sizeof(data));
		read_length = read(fd, data, sizeof(data));
		if (read_length <= 0) {
			//if (read_length < 0)
			//	perror(NULL);
			return -1;
	        }
		if (fd == tty_fd) {
			printf("console input received: %s\n", data);
			if (strcmp(data, "q\n")==0) {
				quit = 1;
				break;
			}
		} else
			printf(data);
	} while (read_length > 0);
	return 0;
}

int main(int argc, char *argv[])
{
	int i, retval;
	fd_set readset, master;

	key_fd = open(key_dev_name , O_RDONLY);
	if(key_fd < 0)
	{
		key_fd = open(key_dev_name2 , O_RDONLY);
		if(key_fd < 0)
		{
			printf("%s and %s file open failed\n", key_dev_name, key_dev_name2);
			return -1;
		}
	}

	tty_fd = open(tty_dev_name, O_RDWR | O_NOCTTY);
	if(tty_fd < 0)
	{
		printf("%s file open failed\n", tty_dev_name);
		return -1;
	}

	retval = fcntl(key_fd, F_GETFL);
        fcntl(key_fd, F_SETFL, retval | O_NONBLOCK);
        retval = fcntl(tty_fd, F_GETFL);
        fcntl(tty_fd, F_SETFL, retval | O_NONBLOCK);

	max_fd = 1 + ((key_fd < tty_fd) ? tty_fd : key_fd);
	FD_ZERO(&master);
	FD_SET(key_fd, &master);
	FD_SET(tty_fd, &master);
	while (1)
	{
		printf("\nType something and enter (q to exit):\n");
		//readset = master;
		memcpy(&readset, &master, sizeof master);
		if (select(max_fd, &readset, NULL, NULL, NULL) < 0) {
			printf("Error: select call\n");
			return -1;
		}
		for (i=0; i<max_fd; i++) {
			if (FD_ISSET(i, &readset))
				read_data(i);
		}
		if (quit) break;
	}
	close(key_fd);
	close(tty_fd);
	printf("Program exit successfully\n");
	return 0;
}

