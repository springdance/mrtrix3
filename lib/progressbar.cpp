/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "app.h"
#include "progressbar.h"

#ifdef MRTRIX_AS_R_LIBRARY
# include "wrap_r.h"
# define PROGRESS_PRINT REprintf (
#else
# define PROGRESS_PRINT fprintf (stderr, 
#endif

namespace MR
{

  namespace
  {

    const char* busy[] = {
      ".   ",
      " .  ",
      "  . ",
      "   .",
      "  . ",
      " .  "
    };



    void display_func_cmdline (ProgressInfo& p)
    {
      if (p.multiplier)
        PROGRESS_PRINT "\33[2K\r%s: [%3zu%%] %s", App::NAME.c_str(), size_t (p.value), p.text.c_str());
      else
        PROGRESS_PRINT "\33[2K\r%s: [%s] %s", App::NAME.c_str(), busy[p.value%6], p.text.c_str());
    }


    void done_func_cmdline (ProgressInfo& p)
    {
      if (p.multiplier)
        PROGRESS_PRINT "\33[2K\r%s: [%3u%%] %s\n", App::NAME.c_str(), 100, p.text.c_str());
      else
        PROGRESS_PRINT "\33[2K\r%s: [done] %s\n", App::NAME.c_str(), p.text.c_str());
    }
  }

  void (*ProgressInfo::display_func) (ProgressInfo& p) = display_func_cmdline;
  void (*ProgressInfo::done_func) (ProgressInfo& p) = done_func_cmdline;


}

