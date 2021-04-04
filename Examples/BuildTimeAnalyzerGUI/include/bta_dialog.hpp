//
// File:        bta_dialog.hpp
// Author:      Shareef Abdoul-Raheem
// Description:
//   Platform Agnostic API for openeing file dialogs.
//
//   This should maybe be replaced with a better library but I don't want to check licensing info rn.
//     - Shareef R
//
// Copyright (c) Nintendo Of America 2020
//
#ifndef BTA_DIALOG_HPP
#define BTA_DIALOG_HPP

#include <string> /* string */

namespace bta
{
  bool dialogInit();
  bool dialogOpenFolder(std::string& out);  // out is populated if this function returns true.
  bool dialogOpenFile(std::string& out);  // out is populated if this function returns true.
  void dialogQuit();
}

#endif /* BTA_DIALOG_HPP */
