/*
 Name:		door_lock_system.ino
 Created:	10/12/2019 8:10:33 PM
 Author:	GSzwa
*/

// the setup function runs once when you press reset or power the board
#include <SD.h>
#include <require_cpp11.h>
#include <MFRC522Extended.h>
#include <MFRC522.h>
#include <deprecated.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <EEPROM.h>

#define MFRC_SS_PIN 8 
#define MFRC_RST_PIN 9
#define RESET_TIME 86400000
#define SD_SS_PIN 4
#define START_ADR 3

#define _PIN_A 14 
#define _PIN_B 15
#define _PIN_C 16

struct _log {
	uint8_t sec;
	uint8_t min;
	uint8_t hour;
	uint16_t day;
	byte uid[4];
}str;

void(*resetFunc) (void) = 0;

void set_led(uint8_t);
boolean check_user();
boolean check_admin();
void add_user();
void date_format();
void write_log(String);
inline void blink_led(uint8_t wej) { digitalWrite(wej, !digitalRead(wej)); }


const byte admin_master[4] = {0x00,0x00,0x00,0x00};
const byte admin_slave[4] = {};

int adr_save = START_ADR;

MFRC522 mfrc522(MFRC_SS_PIN, MFRC_RST_PIN);
//          //          //          //          //          //
void setup() {
	pinMode(_PIN_A, INPUT_PULLUP);
	pinMode(_PIN_B, INPUT_PULLUP);
	pinMode(_PIN_C, INPUT_PULLUP);
	pinMode(5, OUTPUT);
	pinMode(6, OUTPUT);
	pinMode(7, OUTPUT);
	pinMode(SD_SS_PIN, OUTPUT);
	pinMode(MFRC_SS_PIN, OUTPUT);

	

	if (EEPROM.read(0) > 1) adr_save = EEPROM.read(0);
	EEPROM.get(1,str.day);

	Serial.begin(9600);
	SPI.begin();
	
	mfrc522.PCD_Init();
	delay(20);
	if (!SD.begin(4)) {
		Serial.println("initialization failed!");
	}

	delay(5);
	/*digitalWrite(SD_SS_PIN, HIGH);
	digitalWrite(MFRC_SS_PIN, LOW);*/
}

//          //          //          //          //          //
void loop() 
{
	while (true)
	{
		if (millis() >= RESET_TIME) 
		{
			EEPROM.put(1,EEPROM.get(1, str.day)+1);
			resetFunc(); 
		}

		if (!digitalRead(_PIN_C)) { set_led(_PIN_C); }
		else if (!digitalRead(_PIN_B)) { set_led(_PIN_B); }
		else if (!digitalRead(_PIN_A)) { set_led(_PIN_A); }
		//else set_led(10);
		
		if (mfrc522.PICC_IsNewCardPresent()) {
			break;
		}
	}

	if (!mfrc522.PICC_ReadCardSerial()) {
		return;
	}

	if (!digitalRead(_PIN_A))
	{
		Serial.println(F("Scanned PICC's UID:"));
		for (uint8_t i = 0; i < 4; i++) Serial.print(mfrc522.uid.uidByte[i], HEX);
	}
	else
	{
		if (check_admin())
		{
			if (!digitalRead(_PIN_C))
			{
				mfrc522.PICC_HaltA();
				add_user();
			}
			else
			{
				write_log("ACCEPTED");
				//open door
				Serial.println("otwieram");
				delay(3000);
			}
		}
		else
		{
			if (!digitalRead(_PIN_B))
			{
				if (check_user()) 
				{ 
					Serial.println("otwieram");
					write_log("ACCEPTED");
				}
				else
				{
					write_log("DENIED");
				}
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
	case 16:
		digitalWrite(5, HIGH);
		digitalWrite(6, LOW);
		digitalWrite(7, LOW);
		break;
	case 15:
		digitalWrite(5, LOW);
		digitalWrite(6, HIGH);
		digitalWrite(7, LOW);
		break;
	case 14:
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

void date_format()
{
	unsigned long time = millis();

	str.sec = ((time / 1000) % 60);
	str.min = ((time / (60000)) % 60);
	str.hour = ((time / (3600000)) % 24);
}


boolean check_admin()
{
	for (uint8_t i = 0; i < 4; i++) {  //
		if (mfrc522.uid.uidByte[i] != admin_master[i] && mfrc522.uid.uidByte[i] != admin_slave[i]) return false;
	}
	return true;
}

void write_log(String acces)
{
	/*digitalWrite(MFRC_SS_PIN, HIGH);
	digitalWrite(SD_SS_PIN, LOW);
	delay(10);*/

	File ACCES_LOG;

	date_format();
	
	
	if (SD.exists("log.txt"))
	{
		ACCES_LOG = SD.open("log.txt", O_APPEND);
	}
	else
	{
		ACCES_LOG = SD.open("log.txt", O_WRITE);
	}

	ACCES_LOG.print(str.day);
	ACCES_LOG.print("   ");
	ACCES_LOG.print(str.hour);
	ACCES_LOG.print("   ");
	ACCES_LOG.print(str.min);
	ACCES_LOG.print("   ");
	ACCES_LOG.print(str.sec);
	ACCES_LOG.print("  --");
	ACCES_LOG.println(acces);


	ACCES_LOG.close();

	/*digitalWrite(MFRC_SS_PIN, HIGH);
	digitalWrite(SD_SS_PIN, LOW);
	delay(10);*/
}

boolean check_user()
{
	int adr_read = START_ADR;
	boolean flag;
	while (adr_read < adr_save)
	{
		flag = true;
		for (uint8_t i = 0; i < 4; i++) {
			if (mfrc522.uid.uidByte[i] != EEPROM.read(adr_read + i)) {
				flag = false;
				break;
			}
		}
		adr_read += 4;
		if (flag) { return true; }
	}
	
	return false;
}

void add_user()
{
	unsigned long now = millis(),now2 = millis();
	while (!mfrc522.PICC_IsNewCardPresent()) 
	{
		if ((millis() - now) > 10000) return;
		if ((millis() - now2) > 500) blink_led(14); now2 = millis();
	}
	mfrc522.PICC_ReadCardSerial();
	

	if (!check_admin() && !check_user() && adr_save < 202)
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