#include "unique_fd.hpp"
#include <unistd.h>
#include <iostream>
#include <cerrno>  // errno 及相关错误码；perror 会解释当前 errno
#include <cstddef> // std::size_t
#include <cstdio>  // fprintf、perror、stderr
#include <fcntl.h> // open、O_RDONLY

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

bool copy_file(int source_fd,int destination_fd) {
    // 每次从 source_fd 里 read
    // 再 write_all 到 destination_fd
    // 读一段，write 一段，知道读到 EOF，count=0
    char buffer[4096];
    while(1) {
        const ssize_t count=::read(source_fd,buffer,sizeof(buffer));
        if(count>0) {
            // 表示 buffer[0,count-1] 是有效的
            if(!write_all(destination_fd,buffer,count)) {
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
// ./copyfile source.txt destination.txt
// main 的返回值，0表示成功，非0表示失败。
int main(int argc,char* argv[]) {
    if(argc!=3) {
        ::fprintf(stderr,"usage: %s <source_file> <destination_file>\n",argv[0]);
        return 1;
    }
    UniqueFd source_fd=UniqueFd(::open(argv[1],O_RDONLY));
    if(!source_fd.valid()) {
        ::perror("open source ");
        return 1;
    }
    UniqueFd destination_fd=UniqueFd(::open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644));
    if(!destination_fd.valid()) {
        ::perror("open destination ");
        return 1;
    }
    if(!copy_file(source_fd.get(),destination_fd.get())) {
        return 1;
    }
    return 0;
} 