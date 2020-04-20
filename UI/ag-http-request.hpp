//
//  HttpRequest.hpp
//  HttpRequest
//
//  Created by CavanSu on 2019/7/3.
//  Copyright © 2019 CavanSu. All rights reserved.
//

#include <stdio.h>
#include <iostream>

using namespace std;

// TODO replace this with libcurl

class HttpGetPostMethod
{
public:
    HttpGetPostMethod();
    virtual ~HttpGetPostMethod();
    // GET
    int HttpGet(string host, string port, string path, string get_content);
    // POST
    int HttpPost(string host, string path, string post_content);
    string get_request_return();
    string get_main_text();
    int get_return_status_code();
    
protected:
    
private:
    int return_status_code_;
    string request_return_;
    string main_text_;
    string HttpSocket(string host, string port, string request_str);
    void AnalyzeReturn(void);
};
