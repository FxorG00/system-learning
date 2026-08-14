#include <cstddef>
#include <cstdio>
#include <iostream>
#include <memory>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <iomanip>
#include <sys/stat.h>
#include "/home/xgf/code/system-learning/cpp/week4/day2/unique_fd.hpp"

bool fd_read(int fd) {
     char buffer[4096];
    while(true) {
        ssize_t count=::read(fd,&buffer,sizeof(buffer));
        if(count>0) {
            // 当前 buffer[0,count) 有效
            for(std::size_t i=0;i<count;i++) {
                std::cout<<buffer[i];
            }
            if(!std::cout) {
                fprintf(stderr,"stdout cout failed\n");
                return false;
            }
        } else if(count==0) {
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
} 

int main() {
    UniqueFd fd{::open("cow_input.txt",O_RDWR)};
    if(!fd.valid()) {
        ::perror("open");
        return 1;
    }
    struct stat fd_info{};
    if(::fstat(fd.get(),&fd_info)==-1) {
        ::perror("fstat");
        return 1;
    }
    if(fd_info.st_size<=0) {
        fprintf(stderr,"length<=0\n");
        return 1;
    }
    const std::size_t length=static_cast<std::size_t>(fd_info.st_size);
    void* address=::mmap(nullptr,length,PROT_READ|PROT_WRITE,MAP_PRIVATE,fd.get(),0);
    if(address==MAP_FAILED) {
        ::perror("mmap");
        return 1;
    }
    auto* bytes=static_cast<char*>(address);
    std::cout<<"mmap before update\n";
    for(std::size_t i=0;i<length;i++) {
        std::cout<<bytes[i];
    }
    std::cout<<std::endl;
    if(!std::cout) {
        fprintf(stderr,"stdout cout failed\n");
        ::munmap(address,length);
        return 1;
    }
    bytes[0]='q';
    std::cout<<"mmap after update\n";
    for(std::size_t i=0;i<length;i++) {
        std::cout<<bytes[i];
    }
    std::cout<<std::endl;
    if(!std::cout) {
        fprintf(stderr,"stdout cout failed\n");
        ::munmap(address,length);
        return 1;
    }

    std::cout<<"fd read\n";
    //用 read 从 fd 读取源文件
    if(!fd_read(fd.get())) {
        fprintf(stderr,"fd_read failed\n");
        ::munmap(address,length);
        return 1;
    }
    ::munmap(address,length);
    return 0;
}