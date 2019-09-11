/*
 * ctlstatus.cpp
 *
 * This file is part of the sim-ctl distribution (https://github.com/OpenVetSimDevelopers/sim-ctl).
 * 
 * Copyright (c) 2019 VetSim, Cornell University College of Veterinary Medicine Ithaca, NY
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
	
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <vector>  
#include <string>  
#include <cstdlib>
#include <sstream>
#include <utility>
#include <algorithm>

#include "simCtlComm.h"
#include "simUtil.h"
#include "shmData.h"

using namespace std;

void makejson(ostream & output, string key, string content)
{
    output << "\"" << key << "\":\"" << content << "\"";
}

struct shmData *shmData;
void sendStatus(void );

int debug = 0;

int
main( int argc, const char* argv[] )
{
    char buffer[256];
	int sts;

	cout << "Content-Type: application/json\r\n\r\n";
	cout << "{\n";

	sts = initSHM(SHM_OPEN );

	if ( sts < 0 )
	{
		sprintf(buffer, "%d: %s", sts, "initSHM failed");
		makejson(cout, "error", buffer );
		cout << "\n}\n";
		return ( 0 );
	}


	sendStatus();
	
	return ( 0 );
}
void
sendStatus(void )
{
	
	cout << " \"auscultation\" : {\n";
	makejson(cout, "side", itoa(shmData->auscultation.side ) );
	cout << ",\n";
	makejson(cout, "row", itoa(shmData->auscultation.row ) );
	cout << ",\n";
	makejson(cout, "col", itoa(shmData->auscultation.col ) );
	cout << ",\n";
	makejson(cout, "heartStrength", itoa(shmData->auscultation.heartStrength ) );
	cout << ",\n";
	makejson(cout, "leftLungStrength", itoa(shmData->auscultation.leftLungStrength ) );
	cout << ",\n";
	makejson(cout, "rightLungStrength", itoa(shmData->auscultation.rightLungStrength ) );
	cout << ",\n";
	makejson(cout, "tag", shmData->auscultation.tag );
	cout << "\n},\n";

	cout << " \"pulse\" : {\n";
	makejson(cout, "right_dorsal", itoa(shmData->pulse.right_dorsal ) );
	cout << ",\n";
	makejson(cout, "RD_AIN", itoa(shmData->pulse.ain[1] ) );
	cout << ",\n";
	makejson(cout, "left_dorsal", itoa(shmData->pulse.left_dorsal ) );
	cout << ",\n";
	makejson(cout, "LD_AIN", itoa(shmData->pulse.ain[3] ) );
	cout << ",\n";
	makejson(cout, "right_femoral", itoa(shmData->pulse.right_femoral ) );
	cout << ",\n";
	makejson(cout, "RF_AIN", itoa(shmData->pulse.ain[2] ) );
	cout << ",\n";
	makejson(cout, "left_femoral", itoa(shmData->pulse.left_femoral ) );
	cout << ",\n";
	makejson(cout, "LF_AIN", itoa(shmData->pulse.ain[4] ) );
	cout << "\n},\n";

	cout << " \"respiration\" : {\n";
	makejson(cout, "ain", itoa(shmData->manual_breath_ain ) );
	cout << ",\n";
	makejson(cout, "active", itoa(shmData->respiration.active ) );
	cout << "\n},\n";
	
	cout << " \"cpr\" : {\n";
	makejson(cout, "last", itoa(shmData->cpr.last ) );
	cout << ",\n";
	makejson(cout, "x", itoa(shmData->cpr.x ) );
	cout << ",\n";
	makejson(cout, "y", itoa(shmData->cpr.y ) );
	cout << ",\n";
	makejson(cout, "z", itoa(shmData->cpr.z ) );
	cout << "\n},\n";
	
	cout << " \"general\" : {\n";
	makejson(cout, "simMgrIPAddr", shmData->simMgrIPAddr );
	cout << "\n}\n";
	
	cout << "\n}\n";
}



