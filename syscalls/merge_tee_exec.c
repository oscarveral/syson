#define _POSIX_C_SOURCE 200809L

#include <wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

enum IndicatorNumbers
{
	ERR = -1,
	OK = 0
};

enum ArgvConstructionConstants
{
	MAX_STR_SIZE = 16,
	EXEC_LINES_ARGV_SIZE = 4,
	TEE_ARGV_SIZE = 3,
	MERGE_FILES_ARGV_MIN_SIZE = 4
};

enum SpecBufferSizes
{
	MIN_BUFSIZE = 1,
	DEFAULT_BUFSIZE = 1024,
	MAX_BUFSIZE = 134217728
};

enum SpecProcessNumbers
{
	MIN_NUMPROC = 1,
	DEFAULT_NUMPROC = 1,
	MAX_NUMPROC = 8
};

enum SpecInputFiles
{
	MAX_INPUT_FILES = 16,
	MIN_INPUT_FILES = 1
};

enum SpecProgramArgs
{
	HELP = 'h',
	NUMPROC = 'p',
	LOGFILE = 'l',
	BUFSIZE = 't'
};

static const char *MERGE_FILES = "./merge_files";
static const char *TEE = "tee";
static const char *EXEC_LINES = "./exec_lines";

static const char *ARGV_NUMPROC = "-p";
static const char *ARGV_BUFSIZE = "-t";

static const int TOTAL_PROGRAMS = 3;

static const char *OPT_PROGRAM_ARGS = "hl:t:p:";

static const char *WARN_NO_INPUT_FILES = "Error: No hay ficheros de entrada.\n";
static const char *WARN_TOO_MANY_INPUT_FILES = "Error: Demasiados ficheros de entrada. Máximo 16 ficheros.\n";
static const char *WARN_INCORRECT_BUFFER_SIZE = "Error: Tamaño de buffer incorrecto.\n";
static const char *WARN_NO_LOGFILE = "Error: No hay fichero de log.\n";
static const char *WARN_INVALID_NUMPROC = "Error: El número de procesos en ejecución tiene que estar entre 1 y 8.\n";

static const char *ERR_MALLOC = "Error. Ha fallado la llamada malloc() / calloc() para reservar memoria dinámica";
static const char *ERR_PIPE = "Error. Ha fallado la llamada pipe() para crear una tubería";
static const char *ERR_CLOSE = "Error. Ha fallado la llamada a close() para cerrar un extremo de una tubería";
static const char *ERR_WAIT = "Error. Ha fallado la llamada a wait() mientras se esperaba a un proceso hijo";
static const char *ERR_FORK = "Error. Ha fallado la llamda a fork() para crear un proceso hijo";
static const char *ERR_DUP2 = "Error. Ha fallado la llamada a dup2()";
static const char *ERR_EXEC = "Error. Ha fallado la llamada a exec() en un proceso hijo";

static const char *USE_GUIDE_STR = "Uso: %s -l LOGFILE [-t BUFSIZE] [-p NUMPROC] FILEIN1 [FILEIN2 ... FILEINn]\n";
static const char *MORE_INFO_STR = "No admite lectura de la entrada estandar.\n-t BUFSIZE\tTamaño de buffer donde 1 <= BUFSIZE <= 128MB\n-l LOGFILE\tNombre del archivo de log.\n-p NUMPROC\tNúmero de procesos en ejecución de forma simultánea (1 <= NUMPROC <= 8)\n";

typedef struct
{
	int bufsize;
	int numproc;
	int file_count;
	char *log_file;
	char **input_files;

} configuration;

void init_configuration(configuration *config, int argc, char **argv)
{
	config->bufsize = DEFAULT_BUFSIZE;
	config->numproc = DEFAULT_NUMPROC;
	config->log_file = NULL;

	int arg = 0;
	optind = 0;

	while ((arg = getopt(argc, argv, OPT_PROGRAM_ARGS)) != ERR)
	{
		switch (arg)
		{
		case BUFSIZE:
			config->bufsize = atoi(optarg);
			if (config->bufsize < MIN_BUFSIZE || config->bufsize > MAX_BUFSIZE)
			{
				fprintf(stderr, WARN_INCORRECT_BUFFER_SIZE);
				fprintf(stderr, USE_GUIDE_STR, argv[0]);
				fprintf(stderr, MORE_INFO_STR);
				exit(EXIT_FAILURE);
			}

			break;
		case LOGFILE:
			config->log_file = optarg;
			break;
		case NUMPROC:
			config->numproc = atoi(optarg);
			if (config->numproc < MIN_NUMPROC || config->numproc > MAX_NUMPROC)
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

	if (config->log_file == NULL)
	{
		fprintf(stderr, WARN_NO_LOGFILE);
		fprintf(stderr, USE_GUIDE_STR, argv[0]);
		fprintf(stderr, MORE_INFO_STR);
		exit(EXIT_FAILURE);
	}

	config->file_count = argc - optind;

	if (config->file_count < MIN_INPUT_FILES)
	{
		fprintf(stderr, WARN_NO_INPUT_FILES);
		fprintf(stderr, USE_GUIDE_STR, argv[0]);
		fprintf(stderr, MORE_INFO_STR);
		exit(EXIT_FAILURE);
	}

	if (config->file_count > MAX_INPUT_FILES)
	{
		fprintf(stderr, WARN_TOO_MANY_INPUT_FILES);
		fprintf(stderr, USE_GUIDE_STR, argv[0]);
		fprintf(stderr, MORE_INFO_STR);
		exit(EXIT_FAILURE);
	}

	config->input_files = calloc(config->file_count, sizeof(char *));

	if (config->input_files == NULL)
	{
		perror(ERR_MALLOC);
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < config->file_count; i++)
	{
		config->input_files[i] = argv[optind + i];
	}
	return;
}

char *to_string(int n)
{
	char *str = calloc(MAX_STR_SIZE, sizeof(char));
	if (str == NULL)
	{
		perror(ERR_MALLOC);
		exit(EXIT_FAILURE);
	}
	sprintf(str, "%d", n);
	return str;
}

char ** merge_files_argv(configuration *config)
{
	char **argv = calloc(MERGE_FILES_ARGV_MIN_SIZE + config->file_count, sizeof(char *));
	if (argv == NULL)
	{
		perror(ERR_MALLOC);
		exit(EXIT_FAILURE);
	}

	argv[0] = (char *) MERGE_FILES;
	argv[1] = (char *) ARGV_BUFSIZE;
	argv[2] = to_string(config->bufsize);

	for (int i = 0; i < config->file_count; i++)
	{
		argv[i + 3] = config->input_files[i];
	}

	return argv;
}

char **tee_argv(configuration * config)
{
	char **argv = calloc(TEE_ARGV_SIZE, sizeof(char *));
	if (argv == NULL)
	{
		perror(ERR_MALLOC);
		exit(EXIT_FAILURE);
	}

	argv[0] = (char *) TEE;
	argv[1] = config->log_file;

	return argv;
}

char **exec_lines_argv(configuration *config)
{
	char **argv = calloc(EXEC_LINES_ARGV_SIZE, sizeof(char *));
	if (argv == NULL)
	{
		perror(ERR_MALLOC);
		exit(EXIT_FAILURE);
	}
	argv[0] = (char *) EXEC_LINES;
	argv[1] = (char *) ARGV_NUMPROC;
	argv[2] = to_string(config->numproc);

	return argv;
}

void merge_files(configuration *config, int output_pipe[2])
{
	switch (fork())
	{
	case ERR:
		perror(ERR_FORK);
		exit(EXIT_FAILURE);
		break;

	case 0:
		if (close(output_pipe[0]) == ERR)
		{
			perror(ERR_CLOSE);
			exit(EXIT_FAILURE);
		}
		if (dup2(output_pipe[1], STDOUT_FILENO) == ERR)
		{
			perror(ERR_DUP2);
			exit(EXIT_FAILURE);
		}
		if (close(output_pipe[1]) == ERR)
		{
			perror(ERR_CLOSE);
			exit(EXIT_FAILURE);
		}

		char ** argv = merge_files_argv(config);
		execvp(MERGE_FILES, argv);
		perror(ERR_EXEC);
		exit(EXIT_FAILURE);
		break;

	default:
		break;
	}
	return;
}

void tee(configuration *config, int input_pipe[2], int output_pipe[2])
{
	int pid;
	switch (fork())
	{
	case ERR:
		perror(ERR_FORK);
		exit(EXIT_FAILURE);
		break;
	case 0:
		if (close(output_pipe[0]) == ERR || close(input_pipe[1]) == ERR)
		{
			perror(ERR_CLOSE);
			exit(EXIT_FAILURE);
		}
		if (dup2(input_pipe[0], STDIN_FILENO) == ERR || dup2(output_pipe[1], STDOUT_FILENO) == ERR)
		{
			perror(ERR_DUP2);
			exit(EXIT_FAILURE);
		}
		if (close(input_pipe[0]) == ERR || close (output_pipe[1]) == ERR)
		{
			perror(ERR_CLOSE);
			exit(EXIT_FAILURE);
		}

		char ** argv = tee_argv(config);
		execvp(TEE, argv);
		perror(ERR_EXEC);
		exit(EXIT_FAILURE);
		break;

	default:
		break;
	}
	return;
}

void exec_lines(configuration *config, int inputfd[2])
{
	switch (fork())
	{
	case ERR:
		perror(ERR_FORK);
		exit(EXIT_FAILURE);
		break;

	case 0:
		if (close(inputfd[1]) == ERR)
		{
			perror(ERR_CLOSE);
			exit(EXIT_FAILURE);
		}
		if (dup2(inputfd[0], STDIN_FILENO) == ERR)
		{
			perror(ERR_DUP2);
			exit(EXIT_FAILURE);
		}
		if (close(inputfd[0]) == ERR)
		{
			perror(ERR_CLOSE);
			exit(EXIT_FAILURE);
		}
		char ** argv = exec_lines_argv(config);
		execvp(EXEC_LINES, argv);
		perror(ERR_EXEC);
		exit(EXIT_FAILURE);
		break;

	default:
		break;
	}
	return;
}

void merge_tee_exec(configuration *config)
{
	int merge_tee_pipe[2];
	int tee_exec_pipe[2];

	if (pipe(merge_tee_pipe) == ERR)
	{
		perror(ERR_PIPE);
		exit(EXIT_FAILURE);
	}

	merge_files(config, merge_tee_pipe);

	if (pipe(tee_exec_pipe) == ERR)
	{
		perror(ERR_PIPE);
		exit(EXIT_FAILURE);
	}

	tee(config, merge_tee_pipe, tee_exec_pipe);

	if (close(merge_tee_pipe[0]) == ERR  || close(merge_tee_pipe[1]) == ERR)
	{
		perror(ERR_CLOSE);
		exit(EXIT_FAILURE);
	}

	exec_lines(config, tee_exec_pipe);

	if (close(tee_exec_pipe[0]) == ERR || close(tee_exec_pipe[1]) == ERR)
	{
		perror(ERR_CLOSE);
		exit(EXIT_FAILURE);
	}

	for (int i = 1; i <= TOTAL_PROGRAMS; i++)
	{
		if(wait(NULL) == ERR)
		{
			perror(ERR_WAIT);
			exit(EXIT_FAILURE);
		}
	}

	return;
}

int main(int argc, char **argv)
{
	configuration config;
	init_configuration(&config, argc, argv);

	merge_tee_exec(&config);

	exit(EXIT_SUCCESS);
}