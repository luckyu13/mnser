#include "uri.h"
#include <iostream>

int main(int argc, char** argv) {
    //MNSER::Uri::ptr uri = MNSER::Uri::Create("http://www.sylar.top/test/uri?id=100&name=MNSER#frg");
    MNSER::Uri::ptr uri = MNSER::Uri::Create("http://admin@www.sylar.top/test/中文/uri?id=100&name=MNSER&vv=中文#frg中文");
    //MNSER::Uri::ptr uri = MNSER::Uri::Create("http://admin@www.syalr.top");
    //MNSER::Uri::ptr uri = MNSER::Uri::Create("http://www.sylar.top/test/uri");
    std::cout << uri->toString() << std::endl;
    auto addr = uri->createAddress();
    std::cout << *addr << std::endl;
    return 0;
}
