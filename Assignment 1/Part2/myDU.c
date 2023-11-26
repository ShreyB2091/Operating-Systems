#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>



unsigned long long int calculate_subdirectory_size(char *full_path) {

	int fd[2];
	pid_t child_pid;

	unsigned long long int subdir_size = 0;

	if(pipe(fd) == -1) {
		perror("Unable to execute\n");
		exit(EXIT_FAILURE);
	}

	if((child_pid = fork()) == -1) {
		perror("Unable to execute\n");
		exit(EXIT_FAILURE);
	}

	if(!child_pid) {
		close(fd[0]);
		dup2(fd[1], STDOUT_FILENO);
		close(fd[1]);

		execl("./myDU", "myDU", full_path, (char *)NULL);

		perror("Unable to execute\n");
		exit(EXIT_FAILURE);
	} else {
		close(fd[1]);
		waitpid(child_pid, NULL, 0);

		char buffer[64];
		ssize_t numBytes = read(fd[0], buffer, sizeof(buffer) - 1);
		if(numBytes < 0) {
			perror("Unable to execute\n");
			exit(EXIT_FAILURE);
		}
		buffer[numBytes] = '\0';
		subdir_size += atoll(buffer);

		close(fd[0]);
	}

	return subdir_size;
}



unsigned long long int calculate_symbolic_link_size(char *initial_path) {

	unsigned long long int sym_link_size = 0;
	struct stat curr_file;

	if (stat(initial_path, &curr_file) == -1) {
		perror("Unable to execute\n");
		exit(EXIT_FAILURE);
	}
	if (S_ISDIR(curr_file.st_mode)) {
		sym_link_size += 4096;

		DIR *dir_name = opendir(initial_path);
		if (dir_name == NULL) {
			perror("Unable to execute\n");
			exit(EXIT_SUCCESS);
		}

		struct dirent *entry;

		while ((entry = readdir(dir_name)) != NULL) {
			if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
				char full_path[4096];
				snprintf(full_path, sizeof(full_path), "%s/%s", initial_path, entry->d_name);
				if(entry->d_type == DT_REG) {
					struct stat new_file;

					if (stat(full_path, &new_file) == -1) {
						perror("Unable to execute\n");
						exit(EXIT_FAILURE);
					}

					sym_link_size += (unsigned long long int) new_file.st_size;
				} else if (entry->d_type == DT_DIR) {
					sym_link_size += calculate_symbolic_link_size(full_path);
				} else if (entry->d_type == DT_LNK) {
					ssize_t len;
					char link_target[4096];
					len = readlink(full_path, link_target, sizeof(link_target) - 1);
					if (len != -1) {
						link_target[len] = '\0';
						char target[4096];
						snprintf(target, sizeof(target), "%s/%s", initial_path, link_target);
						sym_link_size += calculate_symbolic_link_size(target);
					} else {
						perror("Unable to execute\n");
						exit(EXIT_FAILURE);
					}
				}
			}
		}
	} else if(S_ISREG(curr_file.st_mode)) {
		sym_link_size += curr_file.st_size;
	} else if(S_ISLNK(curr_file.st_mode)) {
		size_t len;
		char link_target[4096];
		len = readlink(initial_path, link_target, sizeof(link_target) - 1);
		if (len != -1) {
			link_target[len] = '\0';
			char target[4096];
			snprintf(target, sizeof(target), "%s/%s", initial_path, link_target);
			sym_link_size += calculate_symbolic_link_size(target);
		}
	} else {
		perror("Unable to execute\n");
		exit(EXIT_FAILURE);
	}

	return sym_link_size;
}



int main(int argc, char *argv[]) {

	if(argc != 2) {
		exit(EXIT_SUCCESS);
	}

	unsigned long long int file_size = 0;
	struct stat curr_file;

	if (stat(argv[1], &curr_file) == -1) {
		perror("Unable to execute\n");
		exit(EXIT_FAILURE);
	}

	if (S_ISDIR(curr_file.st_mode)) {
		file_size += 4096;
		DIR *dir = opendir(argv[1]);
		if (dir == NULL) {
			perror("Unable to execute\n");
			exit(EXIT_SUCCESS);
		}

		struct dirent *entry;

		while ((entry = readdir(dir)) != NULL) {
			if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
				char full_path[4096];
				snprintf(full_path, sizeof(full_path), "%s/%s", argv[1], entry->d_name);
				if(entry->d_type == DT_REG) {
					struct stat new_file;

					if (stat(full_path, &new_file) == -1) {
						perror("Unable to execute\n");
						exit(EXIT_FAILURE);
					}

					file_size += (unsigned long long int) new_file.st_size;
				} else if (entry->d_type == DT_DIR) {
					file_size += calculate_subdirectory_size(full_path);
				} else if (entry->d_type == DT_LNK) {
					ssize_t len;
					char link_target[4096];
					len = readlink(full_path, link_target, sizeof(link_target) - 1);
					if (len != -1) {
						link_target[len] = '\0';
						char target[4096];
						snprintf(target, sizeof(target), "%s/%s", argv[1], link_target);
						file_size += calculate_symbolic_link_size(target);
					} else {
						perror("Unable to execute\n");
						exit(EXIT_FAILURE);
					}
				} else {
					perror("Unable to execute\n");
					exit(EXIT_FAILURE);
				}
			}
		}
	}

	printf("%llu", file_size);

	return 0;
}
