#include "util.h"

usize round_usize(usize n, usize multiple) 
{
    return ((n + multiple - 1) / multiple) * multiple;
}
