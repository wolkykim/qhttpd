qHttpd Project
==============

## What's qHttpd?

The goal of qHttpd Project is building a highly customizable HTTP server which can be used in many projects not only as a HTTP contents delivery purpose but also an internal protocol purpose. 

Are you looking for customizable HTTP server for your own software development needs? Are you considering to develop a protocol like a HTTP protocol to use as a inter-communication protol for your software? Do you want to modify standard HTTP protocol and add your own methods to fit in your needs?
If your answer is YES to one of these questions, qHttpd is just for you. Take a look. It's simple, fast and compact! 

## qHttpd Copyright

All of the deliverable code in qHttpd has been dedicated to the public domain by the authors. Anyone is free to copy, modify, publish, use, compile, sell, or distribute the original qLibc code, either in source code form or as a compiled binary, for any purpose, commercial or non-commercial, and by any means. 

All of the deliverable code in qHttpd has been written from scratch. No code has been taken from other projects or from the open internet. Every line of code can be traced back to its original author. So the qHttpd code base is clean and is not contaminated with licensed code from other projects. 

## Features

  * Supports **HTTP/1.1**, HTTP/1.0, HTTP/0.9 
  * Supports completely working codebase for standard HTTP methods: OPTIONS, HEAD, GET, PUT(supports chunked transfer-encoding) 
  * Also supports **WebDAV extension**: PROPFIND, PROPPATCH, MKCOL, MOVE, DELETE, LOCK, UNLOCK 
  * Includes **C hooking**/customizing samples codes. 
  * Supports external **LUA script hooking**. 
  * Supports HTTP Basic Auth Module (refer http_auth.c) 
  * Supports Virtual Host (refer hook.c) 
  * You can **easily customize/add methods**. 
  * Supports server statistics page. 
  * Supports mime types. 
  * Supports rotating file log. 

## How easy adding a new method?

### Sample) Adding a new method in C

```
int hookRequestHandler(struct HttpRequest *pReq, struct HttpResponse *pRes) {
        int nResCode = 0;

        // method hooking
        if(!strcmp(pReq->pszRequestMethod, "MY_METHOD")) {
                nResCode = my_method(pReq, pRes);
        }
        return nResCode;
}

int my_method(struct HttpRequest *pReq, struct HttpResponse *pRes) {
        const char *txt = "Nice to meet you~"

        httpResponseSetCode(pRes, HTTP_CODE_OK, true);
        httpHeaderSetStr(pRes->pHeaders, "MY_HEADER", "Hi~");
        httpResponseSetContent(pRes, "httpd/plain", txt, strlen(txt));
}
```

### Sample) Modifying response using LUA script
```
function responseHandler()
      local code = response:getCode();

      out = assert(io.open("/tmp/test.log", "a+"));
      out:write("Response Code = ",code,"\n");
      out:write("\n");
      out:close();

      if (code == 200) then
              response:setHeader("Server", "milk/1.0.0");
      end
      return 0;
end
```

