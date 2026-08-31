#pragma once

#include "Command.hpp"

#include <memory>
#include <vector>

class CommandRegistry {
public:
    void add(std::shared_ptr<Command> command) { m_commands.push_back(std::move(command)); }
    const std::vector<std::shared_ptr<Command>> &commands() const { return m_commands; }

private:
    std::vector<std::shared_ptr<Command>> m_commands;
};
