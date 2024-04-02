#include <iostream>
#include <functional>
using Message = std::function<void(int)>;
using Messgae1 = std::function<void(int)>;
void hello(int a) {
    std::cout << "Hello, World!" << std::endl;
}

int main() {
    Message message = hello; // 将函数赋值给 Message 类型的变量
    
    message(1); // 调用 Message 类型的变量，执行函数

    return 0;
}
