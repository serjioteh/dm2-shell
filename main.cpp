#include <iostream>
#include <vector>

#include "Entity.h"
#include <signal.h>


pid_t current_pid, shell_pid;
std::set<pid_t> child_pids;

// for pasrer func
std::vector<std::string> operands;

// This function allows to pass last test
bool is_operator(std::string& s) { return s == "||" || s == "&&" || s == "|" || s == "&"; }

// SIGINT signal handler
void handler(int signum)
{
	pid_t pid;
	int status;
	
    signal(signum, &handler);

    if (signum == SIGINT)
    {
        if (getpid() == current_pid)
        {
            for (auto p : child_pids)
                kill(p, SIGINT);
				pid = waitpid(pid, &status, WUNTRACED | WCONTINUED | WNOHANG);
				child_pids.erase(pid);
        }

        if (getpid() != shell_pid)
            exit(1);
    }
}

// run 
void run(std::vector<Entity *>& entities)
{
    int status;

    for (auto entity = entities.begin(); entity < entities.end(); entity++)
    {
        if ((*entity)->get_type() == "call")
        {
            Call* call = dynamic_cast<Call *>(*entity);
            call->run();
            status = call->get_exit_status();
        }
        else
        {
            op_type _operator = dynamic_cast<Operator *>(*entity)->get_name();

            if ((_operator == LOGICAL_OR  &&  (WIFEXITED(status) && !WEXITSTATUS(status))) ||
                (_operator == LOGICAL_AND && !(WIFEXITED(status) && !WEXITSTATUS(status) )))
            {
                for ( ; entity < entities.end(); entity++)
                    if ((*entity)->get_type() == "operator" && dynamic_cast<Operator *>(*entity)->get_name() != _operator)
                        break;
            }
        }
    }
}


//parser part 
static void push_string_to_buffer(std::string& s)
{
    if (s.size() > 0)
    {
        operands.push_back(s);
        s.clear();
    }
}

static std::vector<std::string>& parse(std::string& _call)
{
    operands.clear();
    std::string call = "", op = "", quote = "";

    for (char c : _call)
    {
        if (quote.size() > 0)
        {
            quote += c;
            if (quote[0] == c)
                push_string_to_buffer(quote);
        }
        else
        {
            if (isspace(c))
            {
                push_string_to_buffer(call);
                push_string_to_buffer(op);
            }
            else if (c == '<' || c == '>' || c == ';')
            {
                push_string_to_buffer(call);
                push_string_to_buffer(op);
                operands.push_back(std::string("") + c);
            }
            else if (c == '&' || c == '|')
            {
                push_string_to_buffer(call);
                op += c;
            }
            else if (c == '\'' || c == '\"')
            {
                push_string_to_buffer(call);
                push_string_to_buffer(op);
                quote += c;
            }
            else
            {
                push_string_to_buffer(op);
                call += c;
            }
        }
    }

    push_string_to_buffer(call);
    push_string_to_buffer(op);

    return operands;
}

// main
int main()
{
    signal(SIGINT, &handler);

    std::string command;
    current_pid = shell_pid = getpid();

    while (std::getline(std::cin, command))
    {
        while (waitpid(-1, NULL, WNOHANG) > 0);
        child_pids.clear();

		std::vector<std::string> cmd_arg_op_parsed = parse(command);
		
        if (cmd_arg_op_parsed.size() == 0)
            continue;

        std::vector<Entity *> entities;
        std::vector<std::string> call_to_process;

        for (auto i = cmd_arg_op_parsed.begin(); i < cmd_arg_op_parsed.end(); i++)
        {
            if (!is_operator(*i))
                call_to_process.push_back(*i);
            else
            {
                entities.push_back(new Call(call_to_process));
                call_to_process.clear();

                Entity *op = NULL;

                if (*i == "|")
                    op = new Operator(PIPE);
                else if (*i == "||")
                    op = new Operator(LOGICAL_OR);
                else if (*i == "&")
                    op = new Operator(BACKGROUND_MODE);
                else if (*i == "&&")
                    op = new Operator(LOGICAL_AND);
                else if (*i == ";")
                    op = new Operator(SEPARATOR);

                entities.push_back(op);
            }
        }

        if (call_to_process.size() > 0)
            entities.push_back(new Call(call_to_process));

        for (auto entity = entities.begin(); entity < entities.end(); entity++)
        {
            if ((*entity)->get_type() == "operator" && dynamic_cast<Operator *>(*entity)->get_name() == PIPE)
            {
                int fd[2];
                pipe(fd);

                dynamic_cast<Call *>(*(entity-1))->set_fd_output(fd[1]); // prev
                dynamic_cast<Call *>(*(entity+1))->set_fd_input(fd[0]);  // next
            }
        }

        Entity *back = entities.back();
        if (back->get_type() == "operator" && dynamic_cast<Operator *>(back)->get_name() == BACKGROUND_MODE)
        {
            if ((current_pid = fork()) == 0)
            {
                current_pid = getpid();
                run(entities);
                exit(0);
            }
            else
            {
                child_pids.insert(current_pid);
                std::cerr << "Spawned child process " << current_pid << std::endl;
            }
        }
        else
            run(entities);

        for (auto i : entities)
            delete i;
    }

    return 0;
}

