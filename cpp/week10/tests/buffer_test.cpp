#include "buffer.hpp"
#include <iostream>
#include <cassert>
#include <gtest/gtest.h>

TEST(BufferTest,AppendAndPartialRetrieve) {
    Buffer buffer(5);
    buffer.append("abc",3);
    EXPECT_EQ(buffer.readable_bytes(),3);
    auto begin=buffer.peek();
    EXPECT_EQ(*begin,'a');
    EXPECT_EQ(*(++begin),'b');
    EXPECT_EQ(*(++begin),'c');
    buffer.retrieve(1);
    begin=buffer.peek();
    EXPECT_EQ(*begin,'b');
    EXPECT_EQ(*(++begin),'c');
}

TEST(BufferTest,RetrieveThenAppend) {
    Buffer buffer(5);
    buffer.append("abc",3);
    buffer.retrieve(1);
    buffer.append("defg",4);
    std::string tmp=buffer.retrieve_all_as_string();
    EXPECT_EQ(tmp,"bcdefg");
}

TEST(BufferTest,RetrieveWorks) {
    Buffer buffer(5);
    buffer.append("bcdefg",6);
    std::string tmp=buffer.retrieve_as_string(2);
    EXPECT_EQ(tmp,"bc");
    auto it=buffer.peek();
    char content[]="defg";
    for(std::size_t i=0;i<4;i++) {
        EXPECT_EQ(*it,content[i]);
        ++it;
    }
}

TEST(BufferTest,BinaryBytes) {
    Buffer buffer(5);
    buffer.append("A\0B",3);
    EXPECT_EQ(buffer.readable_bytes(),3);
    auto it=buffer.peek();
    char content[]="A\0B";
    for(std::size_t i=0;i<3;i++) {
        EXPECT_EQ(*it,content[i]);
        ++it;
    }
}

TEST(BufferTest,OutOfRange) {
    Buffer buffer(5);
    buffer.append("A\0B",3);
    EXPECT_THROW(buffer.retrieve(buffer.readable_bytes()+1),std::out_of_range);
    auto it=buffer.peek();
    char content[]="A\0B";
    for(std::size_t i=0;i<3;i++) {
        EXPECT_EQ(*it,content[i]);
        ++it;
    }
    EXPECT_EQ(buffer.readable_bytes(), 3);
}

TEST(BufferTest,EmptyBufferTest) {
    Buffer buffer(5);
    EXPECT_TRUE(buffer.empty());
    buffer.append("anc",0);
    EXPECT_TRUE(buffer.empty());
    buffer.retrieve(0);
    EXPECT_TRUE(buffer.empty());
}

TEST(BufferTest,BigConsumedPrefix) {
    Buffer buffer(5);
    for(std::size_t i=0;i<100000;i++) {
        buffer.append("a",1);
    }
    buffer.retrieve(50000);
    for(std::size_t i=0;i<100000;i++) {
        buffer.append("b",1);
    }
    auto data=buffer.peek();
    for(std::size_t i=0;i<150000;i++) {
        if(i<50000) {
            EXPECT_EQ(data[i],'a');
        } else {
            EXPECT_EQ(data[i],'b');
        }
    }
}