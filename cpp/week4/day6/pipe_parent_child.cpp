#include "../day2/unique_fd.hpp"
#include <cerrno>
#include <cstdio>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int pipefd[2];
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

bool write_all(int fd,char* data,std::size_t size) {
    std::size_t offset=0;
    while(offset<size) {
        const ssize_t written=::write(fd,data+offset,size-offset);
        if(written>0) {
            // 成功写入 
            offset+=static_cast<std::size_t>(written);
        } else if(written==0) {
            // 没有进展
            // 认为失败
            ::fprintf(stderr,"write: return 0 bytes\n");
            return false;
        } else if(written==-1&&errno==EINTR) {
            // 当前被信号打断，重新调用
            continue ;
        } else {
            ::perror("write");
            return false;
        }
    }
    return true;
}

bool parent_work() {
    ::close(pipefd[0]);
    UniqueFd write_end{pipefd[1]};
    char data[]="hello through pipe\n";
    if(!write_all(write_end.get(),data,sizeof(data)-1)) {
        fprintf(stderr,"write error\n");
        return false;
    }
    return true;
}

bool child_work() {
    char buffer[4096];
    ::close(pipefd[1]);
    // 在 child 中立刻关闭 write end
    UniqueFd read_end{pipefd[0]};
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

int main() {
    const int result=::pipe(pipefd);
    if(result==-1) {
        ::perror("pipe");
        return 1;
    }
    // pipe 成功，来 fork

    const pid_t child_pid=::fork();
    // 先 fork 一个子进程出来
    if(child_pid==-1) {
        std::perror("fork");
        return 1;
    }
    if(child_pid==0) {
        // 当前在子进程
        if(!child_work()) {
            return 1;
        }
        return 0;
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
        return 0;
    }
}