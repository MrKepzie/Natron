/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "ImageKey.h"

using namespace Natron;

ImageKey::ImageKey()
: KeyHelper<U64>()
, _nodeHashKey(0)
, _frameVaryingOrAnimated(false)
, _time(0)
//, _mipMapLevel(0)
, _view(0)
, _pixelAspect(1)
, _draftMode(false)
{
}

ImageKey::ImageKey(const CacheEntryHolder* holder,
                   U64 nodeHashKey,
                   bool frameVaryingOrAnimated,
                   double time,
                   //unsigned int mipMapLevel, //< Store different mipmapLevels under the same key
                   int view,
                   double pixelAspect,
                   bool draftMode)
: KeyHelper<U64>(holder)
, _nodeHashKey(nodeHashKey)
, _frameVaryingOrAnimated(frameVaryingOrAnimated)
, _time(time)
, _view(view)
, _pixelAspect(pixelAspect)
, _draftMode(draftMode)
{
}

void
ImageKey::fillHash(Hash64* hash) const
{
    hash->append(_nodeHashKey);
    if (_frameVaryingOrAnimated) {
        hash->append(_time);
    }
    hash->append(_view);
    hash->append(_pixelAspect);
    hash->append(_draftMode);
}

bool
ImageKey::operator==(const ImageKey & other) const
{
    if (_frameVaryingOrAnimated) {
        return _nodeHashKey == other._nodeHashKey &&
        _time == other._time &&
        _view == other._view &&
        _pixelAspect == other._pixelAspect &&
        _draftMode == other._draftMode;
    } else {
        return _nodeHashKey == other._nodeHashKey &&
        _view == other._view &&
        _pixelAspect == other._pixelAspect &&
        _draftMode == other._draftMode;
    }
    
}
