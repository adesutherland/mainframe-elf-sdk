/* Default pathname policy for ordinary MVS/TSO C programs. cREXX supplies
   a separate adapter for its logical NAME.TYPE/location-list convention. */
#include "sdk-path.h"
const char *lab_tso_path(const char *input,char *buffer,size_t capacity)
{
    (void)buffer;(void)capacity;
    return input;
}
