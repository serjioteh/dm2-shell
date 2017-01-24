#ifndef ENTITY
#define ENTITY

#include <iostream>

#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#include <cstring>

#include <string>
#include <vector>
#include <set>

#include <fcntl.h>


class Entity
{
public:
    virtual const std::string get_type() const = 0;
};


class Call : public Entity
{
    char **args;

    int status, n_args;

    int fd_input, fd_output;
    bool is_input_modified, is_output_modified;

public:
    Call(const std::vector<std::string>& call);

    void run();

    void set_fd_input(int fd);
    void set_fd_output(int fd);

    const std::string get_type() const { return "call"; }
    int get_exit_status() const { return status; }

    ~Call();
};


typedef enum
{
    LOGICAL_AND,
    LOGICAL_OR,
    PIPE,
    BACKGROUND_MODE,
    SEPARATOR
} op_type;


class Operator : public Entity
{
public:
    Operator(op_type o) : op_name(o) {}

    const std::string get_type() const { return "operator"; }
    const op_type get_name() { return  op_name; }

private:
    op_type op_name;
};

#endif //ENTITY
