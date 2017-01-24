#include "Entity.h"

extern std::set<pid_t> child_pids;

Call::Call(const std::vector<std::string>& call)
{
    is_input_modified = is_output_modified = false;

    args = (char **) malloc((call.size() + 1) * sizeof(char *));
    int j = 0;

    for (int i = 0; i < call.size(); i++)
    {
        if (call[i] == "<")
        {
            fd_input = open(call[++i].c_str(), O_RDONLY);
            is_input_modified = true;
        }
        else if (call[i] == ">")
        {
            fd_output = open(call[++i].c_str(), O_WRONLY | O_CREAT | O_APPEND | O_TRUNC, 0666);
            is_output_modified = true;
        }
        else
        {
            args[j] = (char *) calloc(call[i].size() + 1, sizeof(char));
            memcpy(args[j++], call[i].c_str(), call[i].size() + 1);
        }
    }

    args[j] = NULL;
    n_args = j + 1;
    args = (char **) realloc(args, n_args * sizeof(char *));
}

void Call::run()
{
    pid_t pid;
    if ((pid = fork()) == 0)
    {
        if (is_input_modified)
        {
            dup2(fd_input, STDIN_FILENO);
            close(fd_input);
        }

        if (is_output_modified)
        {
            dup2(fd_output, STDOUT_FILENO);
            close(fd_output);
        }

        execvp(args[0], args);
		exit(1); // we can be here on error only
    }
    else
    {
        child_pids.insert(pid);

        if (is_input_modified)
            close(fd_input);

        if (is_output_modified)
            close(fd_output);

        pid = waitpid(pid, &status, WUNTRACED | WCONTINUED);
        //pid = wait(&status);
		
        int code = WIFEXITED(status) ? WEXITSTATUS(status) : WIFEXITED(status);
        std::cerr << "Process " << pid << " exited: " << code << std::endl;

        child_pids.erase(pid);
    }
}

void Call::set_fd_input(int fd)
{
    fd_input = fd;
    is_input_modified = true;
}

void Call::set_fd_output(int fd)
{
    fd_output = fd;
    is_output_modified = true;
}

Call::~Call()
{
    for (int i = 0; i < n_args - 1; i++)
        free(args[i]);
    free(args);
}
