#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

typedef enum e_error_type
{
	ERR_CD_ARG,
	ERR_CD_FAIL,
	ERR_CMD_FAIL
} t_error_type;


static void put_err(char *s);
void handle_cd(char **arg, int i);
int main(int ac, char **av, char **env);
void execute(char **arg, int i, char **env);
void do_pipe(char **arg, int i, char **env);

void handle_error(t_error_type err_type, char *arg)
{
	if (err_type == ERR_CD_ARG)
	{
		put_err("error: cd: bad arguments\n");
	}
	else if (err_type == ERR_CD_FAIL)
	{
		put_err("error: cd: cannot change directory to ");
		put_err(arg);
		put_err("\n");
	}
	else if (err_type == ERR_CMD_FAIL)
	{
		put_err("error: cannot execute ");
		put_err(arg);
		put_err("\n");
	}
}

void execute(char **arg, int i, char **env)
{ 
	if (fork() == 0)
	{
		arg[i] = NULL;
		execve(arg[0], arg, env);
		handle_error(ERR_CMD_FAIL, arg[0]);
		exit(EXIT_FAILURE);
	}
	else
		waitpid(-1, NULL, 0);
}

void do_pipe(char **arg, int i, char **env)
{
	int fd[2];

	pipe(fd);
	if (fork() == 0)
	{
		arg[i] = NULL;
		close(fd[0]);
		dup2(fd[1], STDOUT_FILENO);
		close(fd[1]);
		execve(arg[0], arg, env);
		handle_error(ERR_CMD_FAIL, arg[0]);
		exit(EXIT_FAILURE);
	}

	close(fd[1]);
	dup2(fd[0], STDIN_FILENO);
	close(fd[0]);
	waitpid(-1, NULL, 0);
}

void handle_cd(char **arg, int i)
{
	if (i != 2)
		handle_error(ERR_CD_ARG, NULL);
	else if (chdir(arg[1]))
		handle_error(ERR_CD_FAIL, arg[1]);
}

int main(int ac, char **av, char **env)
{
	int i = 0;
	(void)ac;

	while (av[i] && av[i + 1])
	{
		av = &av[i + 1];
		i = 0;

		while (av[i] && strcmp(av[i], ";") && strcmp(av[i], "|"))
			i++;

		if (!strcmp(av[0], "cd"))
			handle_cd(av, i);

		else if (i > 0 && (!av[i] || !strcmp(av[i], ";")))
			execute(av, i, env);

		else if (i > 0 && !strcmp(av[i], "|"))
			do_pipe(av, i, env);
	}
}


static void put_err(char *s)
{
	while (s && *s)
		write(2, s++, 1);
}
