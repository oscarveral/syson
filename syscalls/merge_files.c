#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

typedef enum IndicatorNumbers
{
	ERR = -1,
	OK = 0,
	READ_ENDING = 1,
	READ_ENDED = 2

} indicator;

enum NonGraphCharacters
{
	NEW_LINE = '\n'
};

enum SpecBufferSizes
{
	MIN_BUFSIZE = 1,
	DEFAULT_BUFSIZE = 1024,
	MAX_BUFSIZE = 134217728
};

enum SpecOutputFiles
{
	DEFAULT_OUTPUT = STDOUT_FILENO
};

enum SpecInputFiles
{
	MAX_INPUT_FILES = 16,
	MIN_INPUT_FILES = 1
};

enum SpecProgramArgs
{
	BUFSIZE_ARG = 't',
	OUTPUT_ARG = 'o',
	HELP = 'h'
};

static const char *OPT_PROGRAM_ARGS = "ht:o:";

static const char *WARN_NO_INPUT_FILES = "Error: No hay ficheros de entrada.\n";
static const char *WARN_INCORRECT_BUFFER_SIZE = "Error: Tamaño de buffer incorrecto.\n";
static const char *WARN_TOO_MANY_INPUT_FILES = "Error: Demasiados ficheros de entrada. Máximo 16 ficheros.\n";
static const char *WARN_OUTPUT_DENIED = "Error: El fichero de salida %s no ha podido ser accedido.\n";
static const char *WARN_INPUT_DENIED = "Aviso: No se puede abrir '%s': ";
static const char *WARN_ALL_INPUT_INVALID = "Error: No es posible acceder a ninguno de los ficheros de entrada especificados.\n";
static const char *WARN_CANT_OPEN = "Error. No es posible abrir el fichero %s. Abortando...\n";
static const char *WARN_CANT_CLOSE = "Error. No es posible cerrar el fichero %s. Abortando...\n";

static const char *ERR_MALLOC = "Error. Ha fallado la llamada malloc() / calloc() para reservar memoria dinámica";
static const char *ERR_READ = "Error. Ha fallado la llamada read()";
static const char *ERR_WRITE = "Error. Ha fallado la llamada write()";
static const char *ERR_ACCESS = "Ha fallado la llamada access()";
static const char *ERR_OPEN = "Ha fallado la llamada open()";
static const char *ERR_CLOSE = "Ha fallado la llamada close()";

static const char *STDOUT_STR = "stdout";

static const char *USE_GUIDE_STR = "Uso: %s [-t BUFSIZE] [-o FILEOUT] FILEIN1 [FILEIN2 ... FILEINn]\n";
static const char *MORE_INFO_STR = "No admite lectura de la entrada estandar.\n-t BUFSIZE\tTamaño de buffer donde 1 <= BUFSIZE <= 128MB\n-o FILEOUT\tUsa FILEOUT en lugar de la salida estandar\n";

typedef struct
{
	int bufsize;
	int file_count;
	char *output_file;
	char **input_files;
	bool use_default_output;

} configuration;

typedef struct
{
	int fd;
	int bufsize;
	int buffer_offset;
	int available_bytes;
	char *buffer;
	char *name;
	indicator status;

} file;

void _validate_configuration_files(configuration *config)
{
	if (!config->use_default_output)
	{
		if ((access(config->output_file, F_OK) == OK) && (!access(config->output_file, W_OK) == OK))
		{
			fprintf(stderr, WARN_OUTPUT_DENIED, config->output_file);
			perror(ERR_ACCESS);
			exit(EXIT_FAILURE);
		}
	}

	int valid_file_count = config->file_count;

	for (int i = 0; i < config->file_count; i++)
	{
		if (access(config->input_files[i], R_OK) != OK)
		{
			fprintf(stderr, WARN_INPUT_DENIED, config->input_files[i]);
			perror(NULL);
			valid_file_count--;
			config->input_files[i] = NULL;
		}
	}

	if (valid_file_count == 0)
	{
		fprintf(stderr, WARN_ALL_INPUT_INVALID);
		exit(EXIT_FAILURE);
	}

	if (valid_file_count < config->file_count)
	{
		char **valid_files = calloc(valid_file_count, sizeof(char *));
		if (valid_files == NULL)
		{
			perror(ERR_MALLOC);
			exit(EXIT_FAILURE);
		}

		int next_file_index = 0;
		for (int j = 0; j < config->file_count; j++)
		{
			if (config->input_files[j] != NULL)
			{
				valid_files[next_file_index] = config->input_files[j];
				next_file_index++;
			}
		}

		free(config->input_files);
		config->input_files = valid_files;
		config->file_count = valid_file_count;
	}
	return;
}

void _init_file(file *f, configuration *config)
{
	f->buffer = calloc(config->bufsize, sizeof(char));
	if (f->buffer == NULL)
	{
		perror(ERR_MALLOC);
		exit(EXIT_FAILURE);
	}
	f->bufsize = config->bufsize;
	f->status = OK;
	f->buffer_offset = 0;
	f->available_bytes = 0;

	return;
}

void _debug_show_file_info(file *f)
{
	fprintf(stderr, "name: %s\n", f->name);
	fprintf(stderr, "fd: %d\n", f->fd);
	fprintf(stderr, "bufsize: %d\n", f->bufsize);
	fprintf(stderr, "buffer_offset: %d\n", f->buffer_offset);
	fprintf(stderr, "available_bytes: %d\n", f->available_bytes);
	fprintf(stderr, "status: %d\n", f->status);
	fprintf(stderr, "buffer: [");
	for (int i = 0; i < f->bufsize; i++)
		fprintf(stderr, "%c", f->buffer[i]);
	fprintf(stderr, "]\n\n");

	return;
}

void init_configuration(configuration *config, int argc, char **argv)
{
	config->bufsize = DEFAULT_BUFSIZE;
	config->use_default_output = true;
	config->output_file = NULL;

	int arg = 0;
	optind = 0;

	while ((arg = getopt(argc, argv, OPT_PROGRAM_ARGS)) != ERR)
	{
		switch (arg)
		{
		case BUFSIZE_ARG:
			config->bufsize = atoi(optarg);
			if (config->bufsize < MIN_BUFSIZE || config->bufsize > MAX_BUFSIZE)
			{
				fprintf(stderr, WARN_INCORRECT_BUFFER_SIZE);
				fprintf(stderr, USE_GUIDE_STR, argv[0]);
				fprintf(stderr, MORE_INFO_STR);
				exit(EXIT_FAILURE);
			}

			break;
		case OUTPUT_ARG:
			config->use_default_output = false;
			config->output_file = optarg;
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

	_validate_configuration_files(config);

	return;
}

void free_configuration(configuration *config)
{
	free(config->input_files);

	return;
}

void init_output_file(file *f, configuration *config)
{
	if (!config->use_default_output)
	{
		f->fd = open(config->output_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
		if (f->fd == ERR)
		{
			fprintf(stderr, WARN_CANT_OPEN, config->output_file);
			perror(ERR_OPEN);
			exit(EXIT_FAILURE);
		}
		f->name = config->output_file;
	}
	else
	{
		f->fd = STDOUT_FILENO;
		f->name = (char *)STDOUT_STR;
	}

	_init_file(f, config);

	return;
}

file *allocate_input_files(configuration *config)
{
	file *file_array = malloc(config->file_count * sizeof(file));
	if (file_array == NULL)
	{
		perror(ERR_MALLOC);
		exit(EXIT_FAILURE);
	}
	return file_array;
}

void init_input_files(file *file_array, configuration *config)
{
	for (int i = 0; i < config->file_count; i++)
	{
		file_array[i].fd = open(config->input_files[i], O_RDONLY);
		if (file_array[i].fd == ERR)
		{
			fprintf(stderr, WARN_CANT_OPEN, config->input_files[i]);
			perror(ERR_OPEN);
			exit(EXIT_FAILURE);
		}
		file_array[i].name = config->input_files[i];
		_init_file(&file_array[i], config);
	}

	return;
}

void free_file(file *f)
{
	free(f->buffer);
	if (close(f->fd) == ERR)
	{
		fprintf(stderr, WARN_CANT_CLOSE, f->name);
		perror(ERR_CLOSE);
		exit(EXIT_FAILURE);
	}
	return;
}

void free_files(file *file_array, configuration *config)
{
	for (int i = 0; i < config->file_count; i++)
		free_file(&file_array[i]);
	free(file_array);
	return;
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

char read_char(file *f)
{
	if (f->status == READ_ENDED)
		return EOF;

	char next_character = 0;

	if (f->status == OK && f->available_bytes <= 0)
	{
		f->buffer_offset = 0;
		f->available_bytes = read_all(f->fd, f->buffer, f->bufsize);
		if (f->available_bytes < f->bufsize)
			f->status = READ_ENDING;
	}

	if (f->status == READ_ENDING && f->available_bytes <= 0)
	{
		f->status = READ_ENDED;
		return EOF;
	}

	next_character = f->buffer[f->buffer_offset];
	f->available_bytes--;
	f->buffer_offset++;

	return next_character;
}

ssize_t write_all(int fd, void *buf, size_t nbytes)
{
	ssize_t num_written = 0;
	ssize_t total_written = 0;

	ssize_t write_count = nbytes;

	int offset = 0;

	while ((write_count > 0) && (num_written = write(fd, buf + offset, write_count)) > 0)
	{
		write_count -= num_written;
		offset += num_written;
		total_written += num_written;
	}

	if (num_written == ERR)
	{
		perror(ERR_WRITE);
		exit(EXIT_FAILURE);
	}

	return total_written;
}

void flush_file(file *f)
{
	write_all(f->fd, f->buffer, f->available_bytes);
	f->available_bytes = 0;
	f->buffer_offset = 0;
	return;
}

void write_char(file *f, char c)
{
	if (f->available_bytes >= f->bufsize)
	{
		flush_file(f);
	}

	f->buffer[f->buffer_offset] = c;
	f->available_bytes++;
	f->buffer_offset++;

	return;
}

void copy_line(file *source, file *dest)
{
	char next_character = 0;
	do
	{
		next_character = read_char(source);
		if (source->status != READ_ENDED)
			write_char(dest, next_character);

	} while ((source->status != READ_ENDED && next_character != NEW_LINE));

	return;
}

void shuffle_lines(file *input_files_array, file *output_file, configuration *config)
{
	int remaining_files = config->file_count;
	int file_index = 0;

	while (remaining_files > 0)
	{
		file *next_file = input_files_array + file_index;
		if (next_file->status != READ_ENDED)
		{
			copy_line(next_file, output_file);
			if (next_file->status == READ_ENDED)
				remaining_files--;
		}

		file_index = (file_index + 1) % config->file_count;
	}

	flush_file(output_file);

	return;
}

int main(int argc, char **argv)
{
	configuration config;
	init_configuration(&config, argc, argv);

	file output_file;
	init_output_file(&output_file, &config);

	file *input_files = allocate_input_files(&config);
	init_input_files(input_files, &config);

	shuffle_lines(input_files, &output_file, &config);

	free_file(&output_file);
	free_files(input_files, &config);
	free_configuration(&config);

	exit(EXIT_SUCCESS);
}