/*
  comedi/comedistreaming.cc
  Makes comedi streaming devices a relacs plugin
  
  RELACS - Relaxed ELectrophysiological data Acquisition, Control, and Stimulation
  Copyright (C) 2002-2015 Jan Benda <jan.benda@uni-tuebingen.de>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.
  
  RELACS is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <relacs/relacsplugin.h>
#include <relacs/comedi/comedianaloginput.h>
#include <relacs/comedi/comedianalogoutput.h>
#include <relacs/comedi/comedidigitalio.h>

#ifdef HAVE_COMEDI_SET_ROUTING
#include <relacs/comedi/comedirouting.h>
#include <relacs/comedi/comedinipfi.h>
#endif

namespace comedi {

  addAnalogInput( ComediAnalogInput, comedi );
  addAnalogOutput( ComediAnalogOutput, comedi );
  addDigitalIO( ComediDigitalIO, comedi );
#ifdef HAVE_COMEDI_SET_ROUTING
  addDevice( ComediRouting, comedi );
  addDevice( ComediNIPFI, comedi );
#endif

};
