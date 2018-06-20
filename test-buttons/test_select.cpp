#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <linux/input.h>
#include <iostream>

using namespace std;
static const char *key_dev_name = "/dev/input/buttons";
static const char *key_dev_name2 = "/dev/buttons";
static const char *tty_dev_name = "/dev/tty";

int readkeycode(int key_fd) 
{
	char buttons[9];
        int i, read_length;
	int num_of_keys= 8;

	/* All data should be read!
	 * New data from the device is only notified once.
	 */
	do {
		memset(buttons, 0, sizeof(buttons));
		read_length = read(key_fd, buttons, num_of_keys);
		if (read_length <= 0) {
			//if (read_length < 0)
			//	perror(NULL);
			return -1;
	        }
		cout << buttons;
	} while (read_length > 0);

	return 0;
}

int main(int argc, char *argv[])
{
	int key_fd, tty_fd, max_fd;
	int read_bytes, retval;
	char data[256] = {0};
	fd_set readset, master;

	key_fd = open(key_dev_name , O_RDONLY);
	if(key_fd < 0)
	{
		key_fd = open(key_dev_name2 , O_RDONLY);
		if(key_fd < 0)
		{
			cout << key_dev_name << "and" <<  key_dev_name2 << " file open failed" << endl;
			return -1;
		}
	}

	tty_fd = open(tty_dev_name, O_RDWR | O_NOCTTY);
	if(tty_fd < 0)
	{
		cout << tty_dev_name << " file open failed:" << endl;
		return -1;
	}

	retval = fcntl(key_fd, F_GETFL);
        fcntl(key_fd, F_SETFL, retval | O_NONBLOCK);
        retval = fcntl(tty_fd, F_GETFL);
        fcntl(key_fd, F_SETFL, retval | O_NONBLOCK);

	max_fd = 1 + ((key_fd < tty_fd) ? tty_fd : key_fd);
	FD_ZERO(&master);
	FD_SET(key_fd, &master);
	FD_SET(tty_fd, &master);
	while (1)
	{
		cout << endl << "Type something and enter (q to exit):" << endl;
		//readset = master;
		memcpy(&readset, &master, sizeof master);
		if (select(max_fd, &readset, NULL, NULL, NULL) < 0) {
			cout << "Error: select call" << endl;
			return -1;
		}
		if (FD_ISSET(tty_fd, &readset))
		{
			cin >> data;
			if (strcmp(data, "q")==0) break;
			cout << "info: got data \"" << data << "\" from standard input" << endl;
		}
		if (FD_ISSET(key_fd, &readset)) 
		{
			readkeycode(key_fd);
		}
	}
	close(key_fd);
	close(tty_fd);
	cout << "Program exit successfully" << endl;
	return 0;
}

