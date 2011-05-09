#include <cstdio>
#include <cstdlib>
#include <err.h>
#include <fcntl.h>
#include <string>
#include <unistd.h>

#include "Parser.hh"
#include "JIT.hh"
#include "xmalloc.h"

#define INITIAL_BUF 64

using namespace std;

static char *readfile(const char *filename)
{
    int fd = open(filename, O_RDONLY);
    if (fd < 0)
        err(1, "Failed to open file '%s'", filename);

    size_t size = INITIAL_BUF;
    size_t count = 0;
    char *buf = static_cast<char *>(xmalloc(size));
    int error;
    while ((error = read(fd, buf + count, size - 1 - count)) > 0) {
        count += error;
        if (count == size - 1) {
            size *= 2;
            buf = static_cast<char *>(xrealloc(buf, size));
        }
    }
    if (error < 0)
        err(1, "Failed to read file");

    buf[count] = '\0';
    return buf;
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        printf("Usage: tjit <in> <func> [params]\n");
        return 1;
    }

    // Parse file
    string data(readfile(argv[1]));
    map<string, Function *> *funcs(Parser(data).parse());
    if (!funcs) {
        errx(1, "Parse error");
    }

    // Get requested function and check parameter count
    map<string, Function *>::iterator funcIter = funcs->find(string(argv[2]));
    if (funcIter == funcs->end())
        errx(1, "No such function '%s'", argv[2]);

    Function *func = funcIter->second;
    if (func->getArity() != argc - 3) 
        errx(1, "Expected %d arguments, got %d", func->getArity(), argc - 3);

    unsigned int params[func->getArity()];
    for (int i = 0; i < func->getArity(); i++)
        params[i] = strtoul(argv[i + 3], NULL, 0); 

    int result = JIT(func, params).run();
    printf("----------------------------------------------------------\n");
    printf("Result: %d\n", result);
    return 0;
}
