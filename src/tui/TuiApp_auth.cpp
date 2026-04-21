/*
  Authentication screen for the terminal client.
  This file builds the login/register form and keeps the auth-specific Enter
  behavior local to the two input fields.
*/
#include "TuiApp.h"

Component TuiApp::make_auth_view() {
  auto username_field = CatchEvent(m_auth_username_field, [this](Event event) {
    if (event == Event::Return) {
      m_auth_password_field->TakeFocus();
      return true;
    }
    return false;
  });

  auto password_field = CatchEvent(m_auth_password_field, [this](Event event) {
    if (event == Event::Return) {
      submit_auth(false);
      return true;
    }
    return false;
  });

  auto buttons = Container::Horizontal({
      Button("Login", [this] { submit_auth(false); }),
      Button("Register", [this] { submit_auth(true); }),
  });

  auto form = Container::Vertical({
      username_field,
      password_field,
      buttons,
  });

  return Renderer(form, [this, form] {
    Element status = m_auth_status.empty()
                         ? text("Use Login or Register to continue.") | dim
                         : text(m_auth_status);

    return vbox({
               text(" Authentication ") | bold | center,
               separator(),
               text("Nickname"),
               form->Render(),
               separator(),
               status,
           }) |
           border | size(WIDTH, EQUAL, 54) | size(HEIGHT, EQUAL, 14) | center;
  });
}
