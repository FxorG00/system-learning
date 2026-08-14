#include "../day2/unique_fd.hpp"
#include <cerrno>
#include <cstdio>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#define read_fd pipefd[0]
#define write_fd pipefd[1]
int pipefd[2];
char* file_name;

// 等待 child_pid 这个子进程结束
pid_t waitpid_retry(pid_t child_pid, int* status) {
    while (true) {
        const pid_t result = ::waitpid(child_pid, status, 0);
        if (result == -1 && errno == EINTR) {
            continue;
        }
        return result;
    }
}

bool parent_work() {
    char buffer[4096];
    ::close(write_fd);
    UniqueFd read_end{read_fd};
    while(1) {
        const ssize_t count=::read(read_end.get(),buffer,sizeof(buffer));
        if(count>0) {
            // 表示 buffer[0,count-1] 是有效的
            for(int i=0;i<count;i++) {
                std::cout<<buffer[i];
            }
            std::cout<<std::flush;
            if(!std::cout) {
                return false;
            }
        } else if(count==0) {
            // EOF
            return true;
        } else {
            if(errno==EINTR) {
                continue ;
            } else {
                ::perror("read");
                return false;
            }
        }
    }
    return true;
}

void child_work() {
    ::close(read_fd);
    if(::dup2(write_fd,STDOUT_FILENO)==-1) {
        ::close(write_fd);
        _exit(127);
    }
    ::close(write_fd);
    if(::execlp(file_name,file_name,"hello from exec",static_cast<char*>(nullptr))==-1) {
        ::perror("execlp");
        _exit(127);
    }
}
// ./fork_exec_pipe echo
int main(int argc,char* argv[]) {
    if(argc!=2) {
        fprintf(stderr,"usgae ./fork_exec_pipe <file_name>\n");
        return 1;
    }
    file_name=argv[1];
    const int result=::pipe(pipefd);
    if(result==-1) {
        ::perror("pipe");
        return 1;
    }
    const pid_t child_pid=::fork();
    if(child_pid==-1) {
        ::perror("fork");
        return 1;
    }
    if(child_pid==0) {
        child_work();
    } else {
        if(!parent_work()) {
            return 1;
        }
        int status = 0;
        const pid_t waited_pid = waitpid_retry(child_pid, &status);
        if (waited_pid == -1) {
            std::perror("waitpid");
            return 1;
        }
        if (WIFEXITED(status)) {
            std::cout << "parent: child " << waited_pid
                    << " exited normally, exit status="
                    << WEXITSTATUS(status) << '\n';
        } else if (WIFSIGNALED(status)) {
            std::cout << "parent: child " << waited_pid
                    << " was terminated by signal "
                    << WTERMSIG(status) << '\n';
        } else {
            std::cout << "parent: child changed state in another way\n";
        }
    }
}