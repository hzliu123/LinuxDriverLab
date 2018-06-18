#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>

using namespace std;
const char key_dev_name[] = "/dev/input/buttons";
const char key_dev_name2[]= "/dev/buttons";
int key_fd;

char data[256] = {0};
char buttons[9] = {0};
int num_of_keys = 8;

void keypad_handler(int signum) 
{
        int i, read_length;
	read_length = read(key_fd, buttons, num_of_keys) ;
        if (read_length < 0) {
                        cout << "e! ";
                        return ;
        }
	cout << buttons << endl;
	return;
}



int main(int argc, char **argv) 
{
	int  retval;
	key_fd = open(key_dev_name , O_RDONLY | O_NONBLOCK);
	if(key_fd < 0)
	{	
		key_fd = open(key_dev_name2 , O_RDONLY | O_NONBLOCK);
		if(key_fd < 0) 
		{
			cout << key_dev_name<< " and " << key_dev_name2 << " file open failed" << endl;
			return -1;
		}
	}
/*
	int read_length = read(key_fd, buttons, num_of_keys) ;
	cout << buttons << endl ;
        if (read_length < 0) cout << "e! " << endl;
*/


	//register our own process to process SIGIO
        fcntl(key_fd, F_SETOWN, getpid());
        //activate FASYNC on fd
        retval = fcntl(key_fd, F_GETFL);
        fcntl(key_fd, F_SETFL, retval | FASYNC);
/*
	// In embedded systems, the compatibility of sigaction is not good.
	// Set up the structure to specify the new action. 
	struct sigaction new_action;
	new_action.sa_handler = keypad_handler;
	sigemptyset (&new_action.sa_mask);
	new_action.sa_flags = 0;
	sigaction (SIGIO, &new_action, NULL);
*/
	signal (SIGIO, keypad_handler);

	while(1) 
	{
		cout << "Press q and return to quit: " << endl ;
		cin >> data;
	        cout << "console input " << "received: " << data << endl;
		if (strcmp(data, "q")==0) break;
		//cout << "info: sleep 10 seconds ..." << endl << endl;
		//sleep(10);
	}
	close (key_fd);
	return 0;
}

