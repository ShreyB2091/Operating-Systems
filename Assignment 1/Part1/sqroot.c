#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>

int main(int argc, char *argv[]) {

	unsigned long long int input_number = atoll(argv[argc - 1]);
	double sq_root = sqrtl(input_number);
	unsigned long long square_root = (unsigned long long)sq_root;
	int last_digit = (unsigned long long)(sq_root * 10) % 10;
	square_root += last_digit >= 5;
	argc--;
	argv++;
	pid_t pid = fork();

	if (pid < 0) {
		perror("Unable to execute\n");
		exit(-1);
	}

	if (!pid) {
		if(argc > 1) {
			char *binary_file_name = argv[0];
			char next_file_path[strlen(binary_file_name) + 3];
			strcpy(next_file_path, "./");
			strcat(next_file_path, binary_file_name);
			argv[0] = next_file_path;
			char number_in_string[100];
			sprintf(number_in_string, "%llu", square_root);
			argv[argc - 1] = number_in_string;
			int res = execv(next_file_path, argv);
			if(res == -1) {
				perror("Unable to execute\n");
				exit(EXIT_FAILURE);
			}
		}
	} else {
		if(argc == 1) printf("%llu\n", square_root);
	}

	return 0;
}
