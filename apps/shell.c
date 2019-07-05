#include "syscalls.h"
#include "signal.h"
#include "coraxstd.h"
#include "implementme.h"
#include "malloc.h"
#include "string.h"
#include "assert.h"
#include "keyboard_map.h"

extern char **environ;

char *read_line()
{
	int buf_size = 1024;
	char *buf = malloc(buf_size);
	int bufIndex = 0;
	int validCharacter;
	while (true) {
		validCharacter = false;
		uint8_t character = getc(stdin);
		if (character == KEY_PRESSED) {
			getc(stdin);
			continue;
		}

		if (character == KEY_RELEASED) {
			character = getc(stdin);
		}

		switch (character) {
		case KEY_RETURN:
			character = '\n';
			validCharacter = true;
			break;
		case KEY_SLASH:
			character = '/';
			validCharacter = true;
			break;
		case KEY_BACKSLASH:
			character = '\\';
			validCharacter = true;
			break;
		case KEY_DELETE:
			character = '\b';
			validCharacter = true;
			break;
		case KEY_SPACE:
			character = ' ';
			validCharacter = true;
			break;
		}
		if (character >= 'a' && character <= 'z' ||
		    character >= '1' && character <= '9') {
			validCharacter = true;
		}

		//Grow buffer if full
		if (bufIndex == buf_size) {
			buf_size *= 2;
			buf = realloc(buf, buf_size);
		}

		if (character == '\n') {
			buf[bufIndex] = '\0';
			printf("%s", "\n");
			return buf;
		} else if (character == '\b') {
			if (bufIndex > 0) {
				bufIndex--;
			} else {
				continue;
			}
		} else if (validCharacter) {
			buf[bufIndex] = character;
			bufIndex++;
		}

		if (validCharacter) {
			char charBuf[2];
			charBuf[0] = character;
			charBuf[1] = '\0';
			printf("%s", charBuf);
		}
	}
}
void run_process(char *command)
{
	char **args = tokenize(command, " ");
	if (access(args[0], 0) != 0) {
		printf("%s: command not found\n", args[0]);
		free_arr(args);
		return;
	}

	int pid = fork();

	/*int fds[2];
    pipe(fds);
    Ignore for now
    */

	if (pid == 0) { //Child
		execve(args[0], args, environ);

		exit(-1);
	}

	free_arr(args);
	assert(waitpid(pid, NULL, NULL) == 0);
}

void clear()
{
	//sys_clear?
}
int main(int argc, char *args[])
{
	int buf[1024];

	while (true) {
		printf("corax$");
		char *line = read_line();
		if (strncasecmp(line, "clear", sizeof("clear")) == 0) {
			clear();
		} else if (strncasecmp(line, "help", sizeof("help")) == 0) {
			puts("\n");
			puts("Welcome to coraxOS!\n");
			puts("help - display this message\n");
			puts("clear - clear the terminal\n");
			puts("reboot - restarts the computer\n");
			puts("filename [arguments] - run program with the specified arguments\n");
			puts("\n");
		} else if (strncasecmp(line, "reboot", sizeof("reboot")) == 0) {
			reboot();
		} else if (strlen(line) == 0) {
			//Do nothing
		} else {
			run_process(line);
		}

		free(line);
	}
}