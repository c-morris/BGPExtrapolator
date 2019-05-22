#ifndef ROVAS_H
#define ROVAS_H

#include "AS.h"


struct ROVAS: public AS {
  bool pass_rov(Announcement &ann);
};

#endif
