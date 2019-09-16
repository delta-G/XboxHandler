/*

XboxHandler  --  library that helps to keep track of buttons and states on 
                 an xbox controller.  The data comes in either in raw format
                 or as ascii text. 
     Copyright (C) 2017  David C.

     This program is free software: you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation, either version 3 of the License, or
     (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program.  If not, see <http://www.gnu.org/licenses/>.

     */
#include "XboxHandler.h"

XboxHandler::XboxHandler(){
	memset(readUnion.rawBuffer, 0, 14);
	oldButtonState = 0;
	oldTriggerState=0;
	buttonClickState=0;
	memset((uint8_t*)oldHatState, 0, 8);
	L2Clicked= false;
	R2Clicked= false;
	newData = false;
}

boolean XboxHandler::newDataAvailable(){
	boolean retval = newData;
	newData = false;
	return retval;
}

//  This code didn't work well so now I use ascii.
void XboxHandler::handleIncoming(uint8_t *aPacket) {
	//check control codes
	if (aPacket[0] == 0x14 && aPacket[1] == 0x0D) {

		readUnion.values.checkBytes =
				(((uint16_t) aPacket[0] << 8) | aPacket[1]);
		readUnion.values.buttonState = (((uint16_t) aPacket[2] << 8)
				| aPacket[3]);
		readUnion.values.leftTrigger = aPacket[4];
		readUnion.values.rightTrigger = aPacket[5];
		for (int i = 0; i < 4; i++) {
			readUnion.values.hatValues[i] = (((uint16_t) aPacket[6 + (2*i)] << 8)
					| aPacket[7 + (2*i)]);
		}

		updateData();
	}

}

void XboxHandler::handleIncomingASCII(char* aPacket){

//	Serial.print("<aP,");
//	Serial.print(aPacket);
//	Serial.print(">");

	//save old hat state
	memcpy (oldHatState, readUnion.values.hatValues, 8);

	if(strncmp(aPacket, "140D", 4) == 0){

		uint8_t rawBuf[14];

		for ( uint8_t i = 0; i < 14; i++){
			char temp[3] = {aPacket[(2*i)], aPacket[1+(2*i)] , 0};
			rawBuf[i] = strtoul(temp, NULL, HEX);
		}

		memcpy(readUnion.rawBuffer, rawBuf, 14);

		updateData();

	}
}

void XboxHandler::updateData() {

	//  Use OR Equal to preserve clicks that haven been read yet
	buttonClickState |= readUnion.values.buttonState & ~oldButtonState;
	oldButtonState = readUnion.values.buttonState;

	if (((uint8_t) oldTriggerState == 0)
			&& readUnion.values.rightTrigger != 0) {
		R2Clicked = true;
	}
	if ((oldTriggerState >> 8 == 0) && readUnion.values.leftTrigger != 0) {
		L2Clicked = true;
	}

	oldTriggerState = (((uint16_t) readUnion.values.leftTrigger) << 8)
			| readUnion.values.rightTrigger;

	newData = true;

}

///  Breaks for R2 since L2 and R2 both equal 0.
boolean XboxHandler::isClicked(ButtonMaskEnum aButton) {

	uint16_t retval = (buttonClickState & aButton);

	// aButton is 0 for L2 and R2 since they're analog

	if (aButton == L2) {
		retval = L2Clicked;
		L2Clicked = false;
	} else if (aButton == R2) {
		retval = R2Clicked;
		R2Clicked = false;
	} else {
//		retval = (buttonClickState & aButton);

		buttonClickState &= ~retval;  // clear that click

	}

	return (boolean) retval;

}


///  This will break on R2 since L2 and R2 both equal 0.
boolean XboxHandler::isPressed(ButtonMaskEnum aButton) {

	if (!(aButton == L2 || aButton == R2)) {
		return (boolean) (readUnion.values.buttonState & aButton);
	}

	if(aButton == L2){
		return (boolean)readUnion.values.leftTrigger;
	}
	return (boolean)readUnion.values.rightTrigger;

}


int16_t XboxHandler::getHatValue(HatEnum aHat) {
	int16_t retval = readUnion.values.hatValues[aHat];
	if (abs(retval) < DEFAULT_DEADBAND) {
		retval = 0;
	}
	return retval;

}

uint8_t XboxHandler::getTriggerValue(ButtonMaskEnum aBut){

	if(aBut == L2) return readUnion.values.leftTrigger;
	if(aBut == R2) return readUnion.values.rightTrigger;

	return 0;
}



void XboxHandler::rebuildPacket(char* aBuf){
	sprintf(aBuf, "%0.4X%0.4X%0.2X%0.2X%0.4X%0.4X%0.4X%0.4X>", 0x140D, readUnion.values.buttonState, readUnion.values.leftTrigger, readUnion.values.rightTrigger, readUnion.values.hatValues[0], readUnion.values.hatValues[1], readUnion.values.hatValues[2], readUnion.values.hatValues[3]);

}
