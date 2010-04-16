#include "VolatileSettings.h"

#include <iostream>

void VolatileSettings::setValue(const QString & key, const QVariant & value)
{
	mValues[key] = value;
}

QVariant VolatileSettings::value(const QString & key, const QVariant & defaultValue) const
{
	return mValues[key];
}
