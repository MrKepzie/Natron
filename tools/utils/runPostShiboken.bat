:: ***** BEGIN LICENSE BLOCK *****
:: This file is part of Natron <http://www.natron.fr/>,
:: Copyright (C) 2015 INRIA and Alexandre Gauthier
::
:: Natron is free software: you can redistribute it and/or modify
:: it under the terms of the GNU General Public License as published by
:: the Free Software Foundation; either version 2 of the License, or
:: (at your option) any later version.
::
:: Natron is distributed in the hope that it will be useful,
:: but WITHOUT ANY WARRANTY; without even the implied warranty of
:: MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
:: GNU General Public License for more details.
::
:: You should have received a copy of the GNU General Public License
:: along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
:: ***** END LICENSE BLOCK *****

:: To be run after shiboken to fix errors

sed "/<destroylistener.h>/d" -i Engine/NatronEngine/*.cpp
sed "/<destroylistener.h>/d" -i Gui/NatronGui/*.cpp
sed -e "/SbkPySide_QtCoreTypes;/d" -i Gui/NatronGui/natrongui_module_wrapper.cpp
sed -e "/SbkPySide_QtCoreTypeConverters;/d" -i  Gui/NatronGui/natrongui_module_wrapper.cpp
sed -e "/SbkNatronEngineTypes;/d" -i Gui/NatronGui/natrongui_module_wrapper.cpp
sed -e "/SbkNatronEngineTypeConverters;/d" -i Gui/NatronGui/natrongui_module_wrapper.cpp
sed -e "s/cleanTypesAttributes/cleanGuiTypesAttributes/g" -i Gui/NatronGui/natrongui_module_wrapper.cpp


rm sed*
rm */sed*
