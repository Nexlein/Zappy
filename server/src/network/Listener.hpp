#pragma once

#include <exception>
#include <string>

class ListenerException : public std::exception {
    public:
    /**
     * @brief Construct exception with error message
     * @param message Description of the error
     */
    explicit ListenerException(const std::string& message) : _message(message) {}

    /**
     * @brief Get the error message
     * @return C-string containing the error description
     */
    const char* what() const noexcept override { return _message.c_str(); }

    private:
    std::string _message;
};

class Listener {
    public:
    explicit Listener(int port);
    ~Listener();

    int fd() const;
    int accept() const;

    Listener(const Listener&) = delete;
    Listener& operator=(const Listener&) = delete;

    private:
    int _fd;
};