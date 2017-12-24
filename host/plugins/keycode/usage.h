#ifndef USAGE_H
#define USAGE_H

#include <Qt>

namespace Usage
{
uint16_t keyboardUsage(const Qt::KeyboardModifiers modifiers, const Qt::Key key);
const char *keyboardString(const uint16_t usage);
}

#endif // USAGE_H
