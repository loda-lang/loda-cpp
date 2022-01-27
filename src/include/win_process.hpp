#pragma once

#include <stdexcept>
#include <string>

// Header-only util functions for process managment on Windows.

#ifdef _WIN64

#include <windows.h>

HANDLE create_win_process(const std::string& command) {
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));
  LPSTR c = const_cast<LPSTR>(command.c_str());
  if (!CreateProcess(nullptr, c, nullptr, nullptr, false, 0, nullptr, nullptr,
                     &si, &pi)) {
    std::string msg = std::string("Error in CreateProcess: ") +
                      std::to_string(GetLastError());
    throw std::runtime_error(msg);
  }
  return pi.hProcess;
}

#endif
