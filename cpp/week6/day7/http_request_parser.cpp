#include <charconv>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

struct Header {
    std::string name;
    std::string value;
};

struct HttpRequest {
    std::string method;
    std::string target;
    std::string version;
    std::vector<Header> headers;
    std::string body;
    std::size_t content_length;
    HttpRequest(): content_length(0) {
        
    }
};

int get_string_count(std::size_t begin,std::size_t end,const std::string& raw,char c) {
    int cnt=0;
    for(std::size_t i=begin;i<end;i++) {
        if(raw[i]==c) {
            ++cnt;
        }
    }
    return cnt;
}

bool parse_request_line(std::size_t begin,std::size_t end,const std::string& raw,HttpRequest& request,std::string& error_message) {
    if(get_string_count(begin,end,raw,' ')!=2) {
        error_message="invalid request line: not equal to 2 whitespaces";
        return false;
    }
    // method
    std::size_t method_begin=begin,method_end=begin;
    while(raw[method_end]!=' ') ++method_end;
    // [method_begin,method_end)
    for(std::size_t i=method_begin;i<method_end;i++) {
        request.method+=raw[i];
    }
    if(request.method.empty()) {
        error_message="invalid request line: empty request method";
        return false;
    }
    // target
    std::size_t target_begin=method_end+1,target_end=target_begin;
    if(raw[target_begin]==' ') {
        error_message="invalid request line: not single SP";
         return false;
    }
    while(raw[target_end]!=' ') ++target_end;
    for(std::size_t i=target_begin;i<target_end;i++) {
        request.target+=raw[i];
    }
    if(request.target.empty()) {
        error_message="invalid request line: empty request target";
        return false;
    }
    // version
    std::size_t version_begin=target_end+1,version_end=version_begin;
    if(raw[version_begin]==' ') {
        error_message="invalid request line: not single SP";
        return false;
    }
    while(raw[version_end]!='\r') ++version_end;
    for(std::size_t i=version_begin;i<version_end;i++) {
        request.version+=raw[i];
    }
    if(request.version.empty()) {
        error_message="invalid request line: empty request version";
        return false;
    }
    if(request.version!="HTTP/1.1") {
        error_message="HTTP version not equal to HTTP/1.1";
        return false;
    }
    return true;
}

bool parse_header(std::size_t begin,std::size_t end,const std::string& raw,HttpRequest& request,std::string& error_message) {
    std::size_t pos=std::string::npos;
    for(std::size_t i=begin;i<end;i++) {
        if(raw[i]==':') {
            pos=i;
            break ;
        }
    }
    if(pos==std::string::npos) {
        error_message="header line not found :";
        return false;
    }
    if(pos==begin) {
        error_message="invalid header field name: name is empty";
        return false;
    }
    if(raw[pos-1]==' '||raw[pos-1]=='\t') {
        error_message="invalid header field name: whitespace before ':'";
        return false;
    }
    // pos 处为 ':'
    // [begin,pos): name
    // 去除掉 OWS，' ' '\t' 后得到 value
    Header tmp;
    for(std::size_t i=begin;i<pos;i++) {
        tmp.name+=raw[i];
    }
    std::size_t first_not_ows=0,last_not_ows=0;
    for(std::size_t i=pos+1;i<end;i++) {
        if(raw[i]!=' '&&raw[i]!='\t') {
            if(!first_not_ows) first_not_ows=i;
            last_not_ows=i;
        }
    }
    if(first_not_ows!=0) {
        for(std::size_t i=first_not_ows;i<=last_not_ows;i++) {
            tmp.value+=raw[i];
        }
    }
    request.headers.push_back(tmp);
    return true;
}

bool case_insentitive_compare(const std::string& A,const std::string& B) {
    if(A.size()!=B.size()) {
        return false;
    }
    for(std::size_t i=0;i<A.size();i++) {
        if(std::tolower(A[i])!=std::tolower(B[i])) {
            return false;
        }
    }
    return true;
}

bool parse_http_request(const std::string& raw,HttpRequest& request,std::string& error_message) {
    std::size_t line_begin=0,line_end;
    line_end=raw.find("\r\n",line_begin);
    if(line_end==std::string::npos) {
        error_message="not found request line";
        return false;
    }
    if(!parse_request_line(line_begin,line_end,raw,request,error_message)) {
        return false;
    }
    // parse headers
    while(1) {
        line_begin=line_end+2;
        line_end=raw.find("\r\n",line_begin);
        if(line_end==std::string::npos) {
            error_message="invalid request";
            return false;
        }
        if(line_begin==line_end) {
            // 遇到空行
            break ;
        }
        if(!parse_header(line_begin,line_end,raw,request,error_message)) {
            return false;
        }
    }
    std::size_t body_start=line_begin+2; // 成为了 body 的开始
    // 处理 content_length
    int content_length_cnt=0;
    for(auto header:request.headers) {
        if(case_insentitive_compare(header.name,"transfer-encoding")) {
            error_message="transfer-encoding unsupported";
            return false;
        }
        if(case_insentitive_compare(header.name,"content-length")) {
            ++content_length_cnt;
            if(content_length_cnt>1) {
                error_message="repeating field: content-length";
                return false;
            }
            std::size_t value=0;
            const auto result=std::from_chars(header.value.data(),header.value.data()+header.value.size(),value);
            const bool ok =result.ec == std::errc{} && result.ptr == header.value.data()+header.value.size();
            if(!ok) {
                error_message="invalid content-length";
                return false;
            }
            request.content_length=value;
        }
    }
    if(request.content_length<raw.size()-body_start) {
        error_message="invalid body: body longer that Content-Length";
        return false;
    } else if(request.content_length>raw.size()-body_start) {
        error_message="invalid body: body shorter that Content-Length";
        return false;
    }
    std::size_t body_end=body_start+request.content_length;
    for(std::size_t i=body_start;i<body_end;i++) {
        request.body+=raw[i];
    }
    return true;
}

int main() {
    std::string input=" / HTTP/1.1\r\n\r\n";
    HttpRequest request;
    std::string error_message;
    if(!parse_http_request(input,request,error_message)) {
        std::cerr<<error_message<<'\n';
        return 1;
    } else {
        std::cout<<"success\n";
        std::cout<<"method: "<<request.method<<'\n';
        std::cout<<"target: "<<request.target<<'\n';
        std::cout<<"version: "<<request.version<<'\n';
        for(auto header:request.headers) {
            std::cout<<header.name<<" "<<header.value<<'\n';
        }
        std::cout<<"body size="<<request.content_length<<'\n';
    }
    return 0;
}