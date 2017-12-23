#ifndef USAGE_H
#define USAGE_H

#include <Qt>

namespace Usage
{
uint8_t keyboardUsage(const Qt::Key key);
const char *keyboardString(const uint8_t usage);
}

#endif // USAGE_H
