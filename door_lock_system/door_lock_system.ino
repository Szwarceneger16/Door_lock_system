/*
 Name:		door_lock_system.ino
 Created:	10/12/2019 8:10:33 PM
 Author:	GSzwa
*/

// the setup function runs once when you press reset or power the board
#include <require_cpp11.h>
#include <MFRC522Extended.h>
#include <MFRC522.h>
#include <deprecated.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <EEPROM.h>

#define SS_PIN 8 
#define RST_PIN 9

void set_led(uint8_t);
boolean check_user();
boolean check_admin();
void add_user();
void open_door();

const byte admin_master[4] = {0x13,0x01,0x8D,0x73};
const byte admin_slave[4] = {};

int adr_save = 1;

MFRC522 mfrc522(SS_PIN, RST_PIN);
//          //          //          //          //          //
void setup() {
	pinMode(2, INPUT_PULLUP);
	pinMode(3, INPUT_PULLUP);
	pinMode(4, INPUT_PULLUP);
	pinMode(5, OUTPUT);
	pinMode(6, OUTPUT);
	pinMode(7, OUTPUT);
	pinMode(14, OUTPUT);

	if (EEPROM.read(0) != 255) adr_save = EEPROM.read(0);

	
	Serial.begin(9600);
	SPI.begin();
	mfrc522.PCD_Init();
}

//          //          //          //          //          //
void loop() 
{
	while (true)
	{
		if (!digitalRead(2)) { set_led(2); }
		else if (!digitalRead(3)) { set_led(3); }
		else if (!digitalRead(4)) { set_led(4); }
		//else set_led(10);

		if (mfrc522.PICC_IsNewCardPresent()) {
			break;
		}
	}

	if (!mfrc522.PICC_ReadCardSerial()) {
		return;
	}

	if (!digitalRead(2))
	{
		Serial.println(F("Scanned PICC's UID:"));
		for (uint8_t i = 0; i < 4; i++) Serial.print(mfrc522.uid.uidByte[i], HEX);
	}
	else
	{
		if (check_admin())
		{
			if (!digitalRead(4))
			{
				add_user();
			}
			else
			{
				//open door
				Serial.println("otwieram");
			}
		}
		else
		{
			if (!digitalRead(3))
			{
				if (check_user()) { Serial.println("otwieram"); }
			}
		}
	}

	mfrc522.PICC_HaltA();
}

////////////////////////////////////////////////////////////////////////////////////////
void set_led(uint8_t wej)
{
	switch (wej)
	{
	case 2:
		digitalWrite(5, HIGH);
		digitalWrite(6, LOW);
		digitalWrite(7, LOW);
		break;
	case 3:
		digitalWrite(5, LOW);
		digitalWrite(6, HIGH);
		digitalWrite(7, LOW);
		break;
	case 4:
		digitalWrite(5, LOW);
		digitalWrite(6, LOW);
		digitalWrite(7, HIGH);
		break;
	default:
		digitalWrite(5, HIGH);
		digitalWrite(6, HIGH);
		digitalWrite(7, HIGH);
		break;
	}
}

boolean check_admin()
{
	for (uint8_t i = 0; i < 4; i++) {  //
		if (mfrc522.uid.uidByte[i] != admin_master[i] && mfrc522.uid.uidByte[i] != admin_slave[i]) return false;
	}
	return true;
}

boolean check_user()
{
	int adr_read = 1;
	boolean flag;
	for (size_t i = 0; i < adr_save; i++)
	{
		flag = true;
		for (uint8_t i = 0; i < 4; i++) {
			if (mfrc522.uid.uidByte[i] != EEPROM.read(adr_read + i)) {
				flag = false;
				break;
			}
		}
		adr_read+=4;
		if (flag) { break; }
	}
	return flag;
}

void add_user()
{
	if (!check_admin)
	{
		for (uint8_t i = 0; i < 4; i++)
		{
			EEPROM.write(adr_save + i, mfrc522.uid.uidByte[i]);
		}
		adr_save += 4;
		EEPROM.write(0, adr_save);
		Serial.println("added");
	}

}