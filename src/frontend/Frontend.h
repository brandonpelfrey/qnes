#pragma once

#include <functional>
#include <memory>

class Console;
class Frontend
{
protected:
  std::function<void()> callback_onclose;

public:
  void SetCallback_OnClose(std::function<void()> callback)
  {
    this->callback_onclose = callback;
  }

  virtual void MainLoop() = 0;
};