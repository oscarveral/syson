struct stat;
struct rtcdate;

// system calls
extern int fork(void);

/* --- Boletín 1. Ejercicio 3. --- */
extern int exit(int) __attribute__((noreturn));
extern int wait(int *);

extern int pipe(int*);
extern int write(int, const void*, int);
extern int read(int, void*, int);
extern int close(int);
extern int kill(int);
extern int exec(char*, char**);
extern int open(const char*, int);
extern int mknod(const char*, short, short);
extern int unlink(const char*);
extern int fstat(int fd, struct stat*);
extern int link(const char*, const char*);
extern int mkdir(const char*);
extern int chdir(const char*);
extern int dup(int);
extern int getpid(void);
extern char* sbrk(int);
extern int sleep(int);
extern int uptime(void);

/* --- Boletín 1. Ejercicio 1. --- */
extern int date(struct rtcdate*);

/* --- Boletín 1. Ejercicio 2. --- */
extern int dup2(int, int);

/* --- Boletín 3. Ejercicio 2. --- */
extern enum proc_prio getprio(int);
extern int setprio(int, enum proc_prio);

// ulib.c
extern int stat(const char*, struct stat*);
extern char* strcpy(char*, const char*);
extern void *memmove(void*, const void*, int);
extern char* strchr(const char*, char c);
extern int strcmp(const char*, const char*);
extern void printf(int, const char*, ...);
extern char* gets(char*, int max);
extern uint strlen(const char*);
extern void* memset(void*, int, uint);
extern void* malloc(uint);
extern void free(void*);
extern int atoi(const char*);

#define NULL 0

/* --- Boletín 1. Ejercicio 3. --- */
/* Las llamadas a exit(...) en los programas de usuario utilizarán estas constantes para indicar el estado de finalización. */
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

/* Macros de procesado del estado de finalización de un proceso. */
#define WIFEXITED(status)   (((status) & 0x7f) == 0)
#define WEXITSTATUS(status) (((status) & 0xff00) >> 8)
#define WIFSIGNALED(status) (((status) & 0x7f) != 0)
/* Para obtener la trap se resta 1 para averiguar su valor original. */
#define WEXITTRAP(status)   (((status) & 0x7f) - 1)    