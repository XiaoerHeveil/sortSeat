#pragma once

#include <stdexcept>
#include <string>

// 函数不存在
class inexistentFunction : public std::runtime_error {
public:
    explicit inexistentFunction(const std::string &what)
        : std::runtime_error("inexistentFunction: " + what) {}
};

// 函数为空（无参数）
class NullFunction : public std::runtime_error {
public:
    explicit NullFunction(const std::string &what)
        : std::runtime_error("NullFunction: " + what) {}
};

// 函数参数无意义
class meaninglessFunction : public std::runtime_error {
public:
    explicit meaninglessFunction(const std::string &what)
        : std::runtime_error("meaninglessFunction: " + what) {}
};

// 函数缺少必填参数
class functionParameterLacking : public std::runtime_error {
public:
    explicit functionParameterLacking(const std::string &what)
        : std::runtime_error("functionParameterLacking: " + what) {}
};

// 函数额外/过多参数
class functionAdditionalParameters : public std::runtime_error {
public:
    explicit functionAdditionalParameters(const std::string &what)
        : std::runtime_error("functionAdditionalParameters: " + what) {}
};

// 工作表中标题不存在
class titleNotExistence : public std::runtime_error {
public:
    explicit titleNotExistence(const std::string &what)
        : std::runtime_error("titleNotExistence: " + what) {}
};

// 姓名不存在
class nameNotExistence : public std::runtime_error {
public:
    explicit nameNotExistence(const std::string &what)
        : std::runtime_error("nameNotExistence: " + what) {}
};
