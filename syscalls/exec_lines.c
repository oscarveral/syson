#define _POSIX_C_SOURCE 200809L

#include <wait.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

enum IndicatorNumbers
{
	ERR = -1
};

enum NonGraphCharacters
{
	NULL_CHAR = '\0',
	NEW_LINE = '\n',
	SPACE = ' '
};

enum SpecInputFiles
{
	DEFAULT_INPUT = STDIN_FILENO
};

enum SpecBufferSizes
{
	LINE_SIZE = 128,
	READ_SIZE = 16
};

enum SpecProgramArgs
{
	HELP = 'h',
	NUMPROC = 'p'
};

enum SpecProcessNumbers
{
	MIN_NUMPROC = 1,
	DEFAULT_NUMPROC = 1,
	MAX_NUMPROC = 8
};

static const char *OPT_PROGRAM_ARGS = "hp:";

static const char *WARN_INVALID_NUMPROC = "Error: El número de procesos en ejecución tiene que estar entre 1 y 8.\n";
static const char *WARN_MAX_LINE_SIZE_EXCEEDED = "Error: Tamaño de línea mayor que 128.\n";

static const char *ERR_MALLOC = "Error. Ha fallado la llamada malloc() / calloc() para reservar memoria dinámica";
static const char *ERR_EXEC = "Error. Ha fallado la llamada exec() de un proceso hijo";
static const char *ERR_FORK = "Error. Ha fallado la llamada fork() para crear un proceso hijo";
static const char *ERR_READ = "Error. Ha fallado la llamada read() sobre la entrada estandar";

static const char *USE_GUIDE_STR = "Uso: %s [-p NUMPROC]\n";
static const char *MORE_INFO_STR = "Lee de la entrada estándar una secuencia de líneas conteniendo órdenes\npara ser ejecutadas y lanza cada una de dichas órdenes en un proceso diferente.\n-p NUMPROC\tNúmero de procesos en ejecución de forma simultánea (1 <= NUMPROC <= 8)\n";

int parse_args(int argc, char **argv)
{
	int numproc = DEFAULT_NUMPROC;

	int arg = 0;
	optind = 1;

	while ((arg = getopt(argc, argv, OPT_PROGRAM_ARGS)) != ERR)
	{
		switch (arg)
		{
		case NUMPROC:
			numproc = atoi(optarg);
			if (numproc < MIN_NUMPROC || numproc > MAX_NUMPROC)
			{
				fprintf(stderr, WARN_INVALID_NUMPROC);
				fprintf(stderr, USE_GUIDE_STR, argv[0]);
				fprintf(stderr, MORE_INFO_STR);
				exit(EXIT_FAILURE);
			}
			break;
		case HELP:
			fprintf(stdout, USE_GUIDE_STR, argv[0]);
			fprintf(stdout, MORE_INFO_STR);
			exit(EXIT_SUCCESS);
			break;
		default:
			fprintf(stderr, USE_GUIDE_STR, argv[0]);
			fprintf(stderr, MORE_INFO_STR);
			exit(EXIT_FAILURE);
			break;
		}
	}
	return numproc;
}

ssize_t read_all(int fd, void *buf, size_t nbytes)
{
	ssize_t num_read = 0;
	ssize_t total_read = 0;

	size_t read_count = nbytes;

	int offset = 0;

	while ((read_count > 0) && ((num_read = read(fd, buf + offset, read_count)) > 0))
	{
		read_count -= num_read;
		offset += num_read;
		total_read += num_read;
	}

	if (num_read == ERR)
	{
		perror(ERR_READ);
		exit(EXIT_FAILURE);
	}

	return total_read;
}

char read_character(void)
{
	static char read_buffer[READ_SIZE];

	static int available_bytes = 0;
	static int char_offset = 0;

	static bool at_eof = false;

	char next_character = 0;

	if (!at_eof && available_bytes <= 0)
	{
		char_offset = 0;
		available_bytes = read_all(DEFAULT_INPUT, read_buffer, READ_SIZE);
		if (available_bytes < READ_SIZE)
			at_eof = true;
	}

	if (at_eof && available_bytes <= 0)
		return EOF;

	next_character = read_buffer[char_offset];
	available_bytes--;
	char_offset++;

	return next_character;
}

size_t read_line(char *buf, size_t bufsize, int *token_count)
{
	static bool at_eof = false;

	char next_character = 0;

	size_t line_offset = 0;
	bool at_token = false;

	*token_count = 0;

	if (at_eof)
		return line_offset;

	do
	{
		next_character = read_character();
		buf[line_offset] = next_character;

		if (isgraph(next_character))
		{
			if (!at_token)
			{
				*token_count += 1;
				at_token = true;
			}
		}
		else
		{
			at_token = false;
			if (next_character == EOF)
			{
				buf[line_offset] = NEW_LINE;
				at_eof = true;
			}
		}

		line_offset++;

	} while ((next_character != EOF && next_character != NEW_LINE) && (line_offset < bufsize));

	if (line_offset >= bufsize && (next_character != EOF && next_character != NEW_LINE))
	{
		fprintf(stderr, WARN_MAX_LINE_SIZE_EXCEEDED);
		exit(EXIT_FAILURE);
	}

	return line_offset;
}

char **build_argv(char *buf, size_t line_length, int token_count)
{
	char **argv = calloc(token_count + 1, sizeof(char *));

	if (argv == NULL)
	{
		perror(ERR_MALLOC);
		exit(EXIT_FAILURE);
	}

	bool at_token = false;

	int token_index = 0;

	for (int offset = 0; offset < line_length; offset++)
	{
		if (isgraph(buf[offset]) && !at_token)
		{
			argv[token_index] = buf + offset;
			token_index++;
			at_token = true;
		}
		else if (buf[offset] == SPACE || buf[offset] == NEW_LINE)
		{
			buf[offset] = NULL_CHAR;
			at_token = false;
		}
	}

	return argv;
}

void execute_lines(int num_process)
{
	int pid = 0;
	int token_count = 0;
	int line_length = 0;
	int process_count = 0;

	char line[LINE_SIZE] = {0};

	char **line_argv = NULL;

	while ((line_length = read_line(line, LINE_SIZE, &token_count)) > 0)
	{
		if (process_count >= num_process)
		{
			wait(NULL);
			process_count--;
		}

		if (token_count > 0)
		{
			switch (pid = fork())
			{
			case -1:
				perror(ERR_FORK);
				exit(EXIT_FAILURE);
				break;
			case 0:
				line_argv = build_argv(line, line_length, token_count);
				execvp(line_argv[0], line_argv);
				perror(ERR_EXEC);
				exit(EXIT_FAILURE);
				break;
			default:
				process_count++;
				break;
			}
		}
	}

	while (process_count > 0)
	{
		wait(NULL);
		process_count--;
	}
	
}

int main(int argc, char **argv)
{
	int numproc = parse_args(argc, argv);

	execute_lines(numproc);

	return EXIT_SUCCESS;
}