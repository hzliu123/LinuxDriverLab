#define _GNU_SOURCE  /* F_SETSIG */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <poll.h>

const char key_dev_name[] = "/dev/input/buttons";
const char key_dev_name2[]= "/dev/buttons";
const char *tty_dev_name = "/dev/tty";
int key_fd, tty_fd;
int quit = 0;


void keypad_handler(int signum, siginfo_t *info, void *uctxt)
{
        int read_length, fd;
	long band;
	char data[80];

	fd = info->si_fd;
	band = info->si_band;
	//printf("fd: %d, band: %d\n", fd, band);

	/* Just to make sure this is a read event */
	if (!(band & POLLIN))
		return;
	/* All data should be read!
	 * New data from the device is only notified once.
	 */
	do {
		memset(data, 0, sizeof(data));
		read_length = read(fd, data, sizeof(data));
		if (read_length <= 0) {
			//if (read_length < 0)
			//	perror(NULL);
			break;
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

	return;
}



int main(int argc, char **argv) 
{
        int retval;

	key_fd = open(key_dev_name , O_RDONLY | O_NONBLOCK);
	if(key_fd < 0)
	{	
		key_fd = open(key_dev_name2 , O_RDONLY | O_NONBLOCK);
		if(key_fd < 0) 
		{
			printf("%s and %s file open failed\n", key_dev_name, key_dev_name2);
			return -1;
		}
	}

	tty_fd = open(tty_dev_name, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if(tty_fd < 0)
	{
		printf("%s file open failed\n", tty_dev_name);
		return -1;
	}

	//Set the signal number to use; this is required for valid siginfo->si_fd
	fcntl(key_fd, F_SETSIG, SIGIO);
	fcntl(tty_fd, F_SETSIG, SIGIO);
	//register our own process to process SIGIO
        fcntl(key_fd, F_SETOWN, getpid());
        fcntl(tty_fd, F_SETOWN, getpid());
        //activate O_ASYNC on fd
        retval = fcntl(key_fd, F_GETFL);
        fcntl(key_fd, F_SETFL, retval | O_ASYNC);
        retval = fcntl(tty_fd, F_GETFL);
        fcntl(tty_fd, F_SETFL, retval | O_ASYNC);

	// Set up the structure to specify the new action. 
	struct sigaction new_action;
	new_action.sa_sigaction = keypad_handler;
	sigemptyset (&new_action.sa_mask);
	// we need SA_RESTART to prevent cin being interrupted
	// SA_SIGINFO to allow signal handler to receive file descriptor information
	new_action.sa_flags = SA_RESTART | SA_SIGINFO;
	sigaction (SIGIO, &new_action, NULL);

	printf("Press q and return to quit:\n");
	while(1) 
	{
		if (quit) break;
		sleep(1);
	}
	close(key_fd);
	close(tty_fd);
	return 0;
}

