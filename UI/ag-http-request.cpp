//
//  HttpRequest.cpp
//  HttpRequest
//
//  Created by CavanSu on 2019/7/3.
//  Copyright © 2019 CavanSu. All rights reserved.
//

#include "ag-http-request.hpp"
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <netdb.h>
#include <sstream>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

HttpGetPostMethod::HttpGetPostMethod() : return_status_code_(0), request_return_(""), main_text_("")
{
    
}

HttpGetPostMethod::~HttpGetPostMethod()
{
    
}

void HttpGetPostMethod::AnalyzeReturn(void)
{
    size_t position1 = request_return_.find_first_of(" ", 0);
    size_t position2 = request_return_.find(" ", position1+1);
    return_status_code_ = atoi(request_return_.substr(position1+1, position2).c_str());
    
    position1 = request_return_.find("\r\n\r\n", position2);
    main_text_ = request_return_.substr(position1+4);
}

int HttpGetPostMethod::HttpGet(std::string host, std::string port, std::string path, std::string get_content)
{
    stringstream request_str;
    request_str << "GET " << path << (path=="/" ? "" : "?") << get_content << " HTTP/1.1" << "\r\n";
    request_str << "Host: " << host << "\r\n";
    // request_str << "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) "
    //             << "Ubuntu Chromium/62.0.3202.75 Chrome/62.0.3202.75 Safari/537.36 " << "\r\n";
    request_str << "Content-Type: text/html\r\n";
    request_str << "Content-Length: 0\r\n";
    request_str << "Connection: close\r\n";
    request_str << "\r\n";

#if 0
    cout << endl;
    cout << "dump request string: " << endl;
    cout << request_str.str() << endl;
#endif
    
    request_return_ = HttpSocket(host, port, request_str.str());
    if (request_return_ == "") {
        cout << "Http Socket error!" << endl;
        return -1;
    }
    
    AnalyzeReturn();
    
    return 0;
}

int HttpGetPostMethod::HttpPost(std::string host, std::string path, std::string post_content)
{
    stringstream request_str;
    request_str << "POST " << path << " HTTP/1.1" << "\r\n";
    request_str << "Host: " << host << "\r\n";
    // request_str << "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) "
    //             << "Ubuntu Chromium/62.0.3202.75 Chrome/62.0.3202.75 Safari/537.36 " << "\r\n";
    request_str << "Content-Type: application/x-www-form-urlencoded" << "\r\n";
    request_str << "Content-Length: " << post_content.length() << "\r\n";
    request_str << "Connection:close" << "\r\n";
    request_str << "\r\n";
    request_str << post_content;
    
#if 0
    cout << endl;
    cout << "dump request string: " << endl;
    cout << request_str.str() << endl;
#endif
    
    request_return_ = HttpSocket(host, "", request_str.str());
    if(request_return_ == "") {
        cout << "Http Post error!" << endl;
        return -1;
    }

    AnalyzeReturn();

    return 0;
}

string HttpGetPostMethod::HttpSocket(std::string host, std::string port, std::string request_str)
{
    const unsigned int kBufferSize = 1024*1024;
    struct sockaddr_in server_addr;
    hostent *server_hostent = nullptr;
    int client_fd;
    char recv_buf[kBufferSize];
    char ip_str[32];
    bzero(recv_buf, kBufferSize);
    bzero(&server_addr, sizeof(sockaddr_in));
    bzero(ip_str, 32);
    size_t ret = 0;
    fd_set client_fd_set;
    stringstream result_string;
    int h = 0;

    server_hostent = gethostbyname(host.c_str());
    if (server_hostent == nullptr) {
        cout << "get host ip address error!" << endl;
        return "";
    }

    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        cout << "create socket fd error!" << endl;
        return "";
    }

    server_addr.sin_family = AF_INET;
    memcpy((char *)&server_addr.sin_addr.s_addr, (char *)*server_hostent->h_addr_list, server_hostent->h_length);

    if ((port.length() > 0)) {
        int int_port = atoi(port.data());
        server_addr.sin_port = htons(int_port);
    } else {
        server_addr.sin_port = htons(80);
    }

    ret = connect(client_fd, (struct sockaddr *) &server_addr, sizeof(sockaddr_in));
    if (ret < 0) {
        cout << "connect to server error!" << endl;
        return "";
    }

    ret = send(client_fd, request_str.c_str(), request_str.length(), 0);
    if (ret != request_str.length()) {
        cout << "Send request unfinish!" << endl;
        return "";
    }

    FD_ZERO(&client_fd_set);
    FD_SET(client_fd, &client_fd_set);

    while (1) {
        h = select(client_fd + 1, &client_fd_set, NULL, NULL, NULL);
        if(h == -1) {
            cout << "read select failed!" << endl;
            break;
        }
        if(FD_ISSET(client_fd, &client_fd_set)) {
            bzero(recv_buf, kBufferSize);
            ret = read(client_fd, recv_buf, kBufferSize);
            if(ret == 0) {
                cout << "server has closed this connect!" << endl;
                break;
            }
            result_string << recv_buf;
        }
    }
    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);
    
    return result_string.str();
}

string HttpGetPostMethod::get_request_return()
{
    return request_return_;
}

string HttpGetPostMethod::get_main_text()
{
    return main_text_;
}

int HttpGetPostMethod::get_return_status_code()
{
    return return_status_code_;
}
