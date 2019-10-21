#pragma once

#include <functional>
#include <memory>

class Console;
class Frontend
{
protected:
  std::shared_ptr<Console> console;
  std::function<void()> callback_onclose;

public:
  void SetConsole(std::shared_ptr<Console> console)
  {
    this->console = console;
  }
  void SetCallback_OnClose(std::function<void()> callback)
  {
    this->callback_onclose = callback;
  }

  virtual void MainLoop() = 0;
};