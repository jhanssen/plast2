#include "Plast.h"

namespace plast {

Path defaultSocketFile()
{
    return Path::home() + ".plastd.sock";
}

} // namespace plast
