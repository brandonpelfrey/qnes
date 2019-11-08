#pragma once

#include <memory>
#include "core/console.h"

class ImFont;
class Window {
public:
  Window(std::shared_ptr<Console> console, ImFont *font)
    : m_console(console), m_font(font), m_is_visible(true)
  {
    return;
  }

  void draw(bool embed)
  {
    if (is_visible() || embed) {
      render(embed);
    }
  }

  void show()
  {
    m_is_visible = true;
  }

  void hide()
  {
    m_is_visible = false;
  }

  void toggle_visible()
  {
    m_is_visible = !m_is_visible;
  }

  bool is_visible() const
  {
    return m_is_visible;
  }

protected:
  std::shared_ptr<Console> m_console;
  ImFont *const m_font;
  bool m_is_visible;

  virtual void render(bool embed) = 0;
};

