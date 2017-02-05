// 1-channel LoRa Gateway for ESP8266
// Copyright (c) 2016, 2017 Maarten Westenberg version for ESP8266
// Verison 4.0.2
// Date: 2017-01-29
//
// 	based on work done by Thomas Telkamp for Raspberry PI 1ch gateway
//	and many others.
//
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the MIT License
// which accompanies this distribution, and is available at
// https://opensource.org/licenses/mit-license.php
//
// Author: Maarten Westenberg (mw12554@hotmail.com)
//
// This file contains the webserver code for the ESP Single Channel Gateway.

#if A_SERVER==1

// ================================================================================
// WEBSERVER DECLARATIONS 

// None at the moment

// ================================================================================
// WEBSERVER FUNCTIONS 

// ----------------------------------------------------------------------------
// Output the 4-byte IP address for easy printing
// ----------------------------------------------------------------------------
String printIP(IPAddress ipa, const char sep) {
	String response;
	response+=(IPAddress)ipa[0]; response+=sep;
	response+=(IPAddress)ipa[1]; response+=sep;
	response+=(IPAddress)ipa[2]; response+=sep;
	response+=(IPAddress)ipa[3];
	return (response);
}

// Print a HEXadecimal string
String printHEX(char * hexa, const char sep) {
	String response;
	char m;
	m = hexa[0]; if (m<016) response+='0'; response += String(m, HEX);  response+=sep;
	m = hexa[1]; if (m<016) response+='0'; response += String(m, HEX);  response+=sep;
	m = hexa[2]; if (m<016) response+='0'; response += String(m, HEX);  response+=sep;
	m = hexa[3]; if (m<016) response+='0'; response += String(m, HEX);  response+=sep;
	return (response);
}

// ----------------------------------------------------------------------------
// stringTime
// Only when RTC is present we print real time values
// t contains number of milli seconds since system started that the event happened.
// So a value of 100 wold mean that the event took place 1 minute and 40 seconds ago
// ----------------------------------------------------------------------------
String stringTime(unsigned long t) {
	String res;
	String Days[7]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};

	if (t==0) { res = "--"; return(res); }
	
	// now() gives seconds since 1970
	time_t eventTime = now() - ((millis()-t)/1000);
	byte _hour   = hour(eventTime);
	byte _minute = minute(eventTime);
	byte _second = second(eventTime);
	
	res += Days[weekday(eventTime)-1]; res += " ";
	res += day(eventTime); res += "-";
	res += month(eventTime); res += "-";
	res += year(eventTime); res += " ";
	if (_hour < 10) res += "0";
	res += _hour; res +=":";
	if (_minute < 10) res += "0";
	res += _minute; res +=":";
	if (_second < 10) res+= "0";
	res += _second;
	return (res);
}


// ----------------------------------------------------------------------------
// WIFI SERVER
//
// This funtion implements the WiFI Webserver (very simple one). The purpose
// of this server is to receive simple admin commands, and execute these
// results are sent back to the web client.
// Commands: DEBUG, ADDRESS, IP, CONFIG, GETTIME, SETTIME
// The webpage is completely built response and then printed on screen.
// ----------------------------------------------------------------------------
String WifiServer(const char *cmd, const char *arg) {

	String response="";

	yield();	
	if (debug >=2) { Serial.println(F("WifiServer new client")); }

	// DEBUG settings; These can be used as a single argument
	if (strcmp(cmd, "DEBUG")==0) {									// Set debug level 0-2
		if (atoi(arg) == 1) {
			debug = (debug+1)%4;
		}	
		else if (atoi(arg) == -1) {
			debug = (debug-1)%4;
		}
		writeGwayCfg(CONFIGFILE);									// Save configuration to file
	}
	
	if (strcmp(cmd, "CAD")==0) {									// Set -cad on=1 or off=0
		_cad=(bool)atoi(arg);
		writeGwayCfg(CONFIGFILE);									// Save configuration to file
	}
	
	if (strcmp(cmd, "HOP")==0) {									// Set -hop on=1 or off=0
		_hop=(bool)atoi(arg);
		if (! _hop) { ifreq=0; freq=freqs[0]; rxLoraModem(); }
		writeGwayCfg(CONFIGFILE);									// Save configuration to file
	}
	
	if (strcmp(cmd, "DELAY")==0) {									// Set delay usecs
		txDelay+=atoi(arg)*1000;
	}
	
	// SF; Handle Spreading Factor Settings
	if (strcmp(cmd, "SF")==0) {
		uint8_t sfi = sf;
		if (atoi(arg) == 1) {
			if (sf==SF12) sf=SF7; else sf= (sf_t)((int)sf+1);
		}	
		else if (atoi(arg) == -1) {
			if (sf==SF7) sf=SF12; else sf= (sf_t)((int)sf-1);
		}
		rxLoraModem();												// Reset the radion with the new spreading factor
		writeGwayCfg(CONFIGFILE);									// Save configuration to file
	}
	
	// FREQ; Handle Frequency  Settings
	if (strcmp(cmd, "FREQ")==0) {
		uint8_t nf = sizeof(freqs)/sizeof(int);						// Number of elements in array
		
		// Compute frequency index
		if (atoi(arg) == 1) {
			if (ifreq==(nf-1)) ifreq=0; else ifreq++;
		}	
		else if (atoi(arg) == -1) {
			Serial.println("down");
			if (ifreq==0) ifreq=(nf-1); else ifreq--;
		}

		freq = freqs[ifreq];
		rxLoraModem();												// Reset the radion with the new frequency
		writeGwayCfg(CONFIGFILE);									// Save configuration to file
	}

	//if (strcmp(cmd, "GETTIME")==0) { response += "gettime tbd"; }	// Get the local time
	//if (strcmp(cmd, "SETTIME")==0) { response += "settime tbd"; }	// Set the local time
	if (strcmp(cmd, "HELP")==0)    { response += "Display Help Topics"; }
#if GATEWAYNODE == 1
	if (strcmp(cmd, "FCNT")==0)   { 
		frameCount=0; 
		rxLoraModem();												// Reset the radion with the new frequency
		writeGwayCfg(CONFIGFILE);
	}
#endif
	if (strcmp(cmd, "RESET")==0)   { 
		response += "Resetting Statistics"; 
		cp_nb_rx_rcv = 0;
		cp_nb_rx_ok = 0;
		cp_up_pkt_fwd = 0;
	}
	
#if WIFIMANAGER==1
	if (strcmp(cmd, "NEWSSID")==0) { 
		WiFiManager wifiManager;
		strcpy(wpa[0].login,""); 
		strcpy(wpa[0].passw,"");
		WiFi.disconnect();
		wifiManager.autoConnect(AP_NAME, AP_PASSWD );
	}
#endif

	delay(5);
	
	// ========================================================================
	// Do webserver work, fill the webpage
	response +="<!DOCTYPE HTML>";
	response +="<HTML><HEAD>";
	response +="<TITLE>ESP8266 1ch Gateway</TITLE>";
	response +="<style>";
	response +=".thead {background-color:green; color:white;}";
	response +=".cell {border: 1px solid black;}";
	response +=".config_table {max_width:100%; min-width:400px; width:90%; border:1px solid black; border-collapse:collapse;}";
	response +="</style>";
	response +="</HEAD>";
	response +="<BODY>";
		
	response +="<h1>ESP Gateway Config</h1>";
	response +="Version: "; response+=VERSION;
	response +="<br>ESP alive since "; response+=stringTime(1); 
	response +="<br>Current time    "; response+=stringTime(millis()); 
	response +="<br>";
		
	// ------------------------------------------------------------------------
	response +="<h2>Gateway Settings</h2>";
	
	response +="<table class=\"config_table\">";
	response +="<tr>";
	response +="<th class=\"thead\">Setting</th>";
	response +="<th style=\"background-color: green; color: white; width:120px;\">Value</th>";
	response +="<th colspan=\"2\" style=\"background-color: green; color: white; width:100px;\">Set</th>";
	response +="</tr>";
	
#if GATEWAYMGT==1
	String bg = " background-color: ";
	bg += ( (_cad == 1) ? "LightGreen" : "orange" );
	response +="<tr><td class=\"cell\">CAD</td>";
	response +="<td style=\"border: 1px solid black;"; response += bg; response += "\">";
	response += ( (_cad == 1) ? "ON" : "OFF" );
	response +="<td style=\"border: 1px solid black; width:40px;\"><a href=\"CAD=1\"><button>ON</button></a></td>";
	response +="<td style=\"border: 1px solid black; width:40px;\"><a href=\"CAD=0\"><button>OFF</button></a></td>";
	response +="</tr>";
	
	bg = " background-color: ";
	bg += ( (_hop == 1) ? "LightGreen" : "orange" );
	response +="<tr><td class=\"cell\">HOP</td>";
	response +="<td style=\"border: 1px solid black;"; response += bg; response += "\">";
	response += ( (_hop == 1) ? "ON" : "OFF" );
	response +="<td style=\"border: 1px solid black; width:40px;\"><a href=\"HOP=1\"><button>ON</button></a></td>";
	response +="<td style=\"border: 1px solid black; width:40px;\"><a href=\"HOP=0\"><button>OFF</button></a></td>";
	response +="</tr>";
	
	response +="<tr><td class=\"cell\">SF Setting</td><td class=\"cell\">";
	if (_cad == 1) {
		response += "AUTO</td>";
	}
	else {
		response += sf;
		response +="<td class=\"cell\"><a href=\"SF=1\"><button>+</button></a></td>";
		response +="<td class=\"cell\"><a href=\"SF=-1\"><button>-</button></a></td>";
	}
	response +="</tr>";
	
	response +="<tr><td class=\"cell\">Channel</td>";
	response +="<td class=\"cell\">"; 
	if (_hop == 1) {
		response += "AUTO</td>";
	}
	else {
		response+=ifreq; 
		response+="</td>";
		response +="<td class=\"cell\"><a href=\"FREQ=1\"><button>+</button></a></td>";
		response +="<td class=\"cell\"><a href=\"FREQ=-1\"><button>-</button></a></td>";
	}
	response +="</tr>";
	
	response +="<tr><td class=\"cell\">Timing Correction (uSec)</td><td class=\"cell\">"; response += txDelay; 
	response+="</td>";
	response +="<td class=\"cell\"><a href=\"DELAY=1\"><button>+</button></a></td>";
	response +="<td class=\"cell\"><a href=\"DELAY=-1\"><button>-</button></a></td>";
	response+="</tr>";	
#endif

	// Debugging options	
	response +="<tr><td class=\"cell\">";
	response +="Debug level</td><td class=\"cell\">"; 
	response +=debug; 
	response +="</td>";
	response +="<td class=\"cell\"><a href=\"DEBUG=1\"><button>+</button></a></td>";
	response +="<td class=\"cell\"><a href=\"DEBUG=-1\"><button>-</button></a></td>";
	response +="</tr>";
	
#if GATEWAYNODE==1
	response +="<tr><td class=\"cell\">Framecounter Internal Sensor</td>";
	response +="<td class=\"cell\">";
	response +=frameCount;
	response +="</td><td colspan=\"2\" style=\"border: 1px solid black;\">";
	response +="<button><a href=\"/FCNT\">RESET</a></button></td>";
	response +="</tr>";
#endif
	
#if WIFIMANAGER==1
	response +="<tr><td>";
	response +="Click <a href=\"/NEWSSID\">here</a> to reset accesspoint<br>";
	response +="</td><td></td></tr>";
#endif

	// Reset all statistics
	response +="<tr><td class=\"cell\">Reset Statistics</td>";
	response +="<td></td><td colspan=\"2\" class=\"cell\"><a href=\"/RESET\"><button>RESET</button></a></td></tr>";
	
	response +="</table>";
	
	delay(1);

	
	
	// ------------------------------------------------------------------------
	response +="<h2>Statistics</h2>";
	
	delay(1);
	response +="<table class=\"config_table\">";
	response +="<tr>";
	response +="<th class=\"thead\">Counter</th>";
	response +="<th class=\"thead\">Value</th>";
	response +="</tr>";
	response +="<tr><td class=\"cell\">Packages Uplink Total</td><td class=\"cell\">";
		response +=cp_nb_rx_rcv; response+="</tr>";
	response +="<tr><td class=\"cell\">Packages Uplink OK </td><td class=\"cell\">";
		response +=cp_nb_rx_ok; response+="</tr>";
	response +="<tr><td class=\"cell\">Packages Downlink</td><td class=\"cell\">"; response +=cp_up_pkt_fwd; response+="</tr>";


#if STATISTICS >= 2
	response +="<tr><td class=\"cell\">SF7 rcvd</td>"; response +="<td class=\"cell\">"; response +=statc.sf7; response +="</td></tr>";
	response +="<tr><td class=\"cell\">SF8 rcvd</td>"; response +="<td class=\"cell\">"; response +=statc.sf8; response +="</td></tr>";
	response +="<tr><td class=\"cell\">SF9 rcvd</td>"; response +="<td class=\"cell\">"; response +=statc.sf9; response +="</td></tr>";
	response +="<tr><td class=\"cell\">SF10 rcvd</td>"; response +="<td class=\"cell\">"; response +=statc.sf10; response +="</td></tr>";
	response +="<tr><td class=\"cell\">SF11 rcvd</td>"; response +="<td class=\"cell\">"; response +=statc.sf11; response +="</td></tr>";
	response +="<tr><td class=\"cell\">SF12 rcvd</td>"; response +="<td class=\"cell\">"; response +=statc.sf12; response +="</td></tr>";
#endif
	response +="</table>";


	
#if STATISTICS >= 1
	// ------------------------------------------------------------------------
	response +="<h2>Message History</h2>";
	
	response +="<table class=\"config_table\">";
	response +="<tr>";
	response +="<th class=\"thead\">Time</th>";
	response +="<th class=\"thead\">Node</th>";
	response +="<th class=\"thead\" colspan=\"2\">Channel</th>";
	response +="<th class=\"thead\" style=\"width: 50px;\">SF</th>";
	response +="<th class=\"thead\" style=\"width: 50px;\">RSSI</th>";
	response +="<th class=\"thead\" style=\"width: 50px;\">pRSSI</th>";
	response +="</tr>";
	for (int i=0; i<MAX_STAT; i++) {
		if (statr[i].sf == 0) break;
		
		response +="<tr>";
		response +="<td class=\"cell\">"; response+=stringTime(statr[i].tmst); response+="</td>";
		
		response +="<td class=\"cell\">"; 
		//response +=statr[i].node; 
		//unsigned char m;
		//m = (statr[i].node>>24)&0xFF; if (m<16) response += "0"; response +=String(m,HEX); response += " ";
		//m = (statr[i].node>>16)&0xFF; if (m<16) response += "0"; response += String(m,HEX); response += " ";
		//m = (statr[i].node>>8)&0xFF; if (m<16) response += "0"; response += String(m,HEX); response += " ";
		//m = (statr[i].node)&0xFF; if (m<16) response += "0"; response += String(m,HEX); response += " ";
		response += printHEX((char *)(& (statr[i].node)),' ');
		response +="</td>";
		
		response +="<td class=\"cell\">"; response +=statr[i].ch; response +="</td>";
		
		response +="<td class=\"cell\">"; response +=freqs[statr[i].ch]; response +="</td>";
		
		response +="<td class=\"cell\">"; response +=statr[i].sf; response +="</td>";
		
		response +="<td class=\"cell\">"; response+=statr[i].rssi; response+="</td>";
		
		response +="<td class=\"cell\">"; response+=statr[i].prssi; response+="</td>";
		response +="</tr>";
	}
	response +="</table>";
#endif

	
	// ------------------------------------------------------------------------
	response +="<h2>System Status</h2>";
	
	response +="<table class=\"config_table\">";
	response +="<tr>";
	response +="<th class=\"thead\">Parameter</th>";
	response +="<th class=\"thead\">Value</th>";
	response +="</tr>";
	response +="<tr><td style=\"border: 1px solid black; width:120px;\">Gateway ID</td>";
	response +="<td class=\"cell\">";	
	  if (MAC_array[0]< 0x10) response +='0'; response +=String(MAC_array[0],HEX);	// The MAC array is always returned in lowercase
	  if (MAC_array[1]< 0x10) response +='0'; response +=String(MAC_array[1],HEX);
	  if (MAC_array[2]< 0x10) response +='0'; response +=String(MAC_array[2],HEX);
	  response +="FFFF"; 
	  if (MAC_array[3]< 0x10) response +='0'; response +=String(MAC_array[3],HEX);
	  if (MAC_array[4]< 0x10) response +='0'; response +=String(MAC_array[4],HEX);
	  if (MAC_array[5]< 0x10) response +='0'; response +=String(MAC_array[5],HEX);
	response+="</tr>";
	response +="<tr><td class=\"cell\">Free heap</td><td class=\"cell\">"; response+=ESP.getFreeHeap(); response+="</tr>";
	response +="<tr><td class=\"cell\">ESP Chip ID</td><td class=\"cell\">"; response+=ESP.getChipId(); response+="</tr>";
	
	response +="</table>";

	// ------------------------------------------------------------------------
	response +="<h2>WiFi Config</h2>";

	response +="<table class=\"config_table\">";
	response +="<tr>";
	response +="<th class=\"thead\">Parameter</th>";
	response +="<th class=\"thead\">Value</th>";
	response +="</tr>";
	response +="<tr><td class=\"cell\">WiFi SSID</td><td class=\"cell\">"; response+=WiFi.SSID(); response+="</tr>";
	response +="<tr><td class=\"cell\">IP Address</td><td class=\"cell\">"; response+=printIP((IPAddress)WiFi.localIP(),'.'); response+="</tr>";
	response +="<tr><td class=\"cell\">IP Gateway</td><td class=\"cell\">"; response+=printIP((IPAddress)WiFi.gatewayIP(),'.'); response+="</tr>";
	response +="<tr><td class=\"cell\">NTP Server</td><td class=\"cell\">"; response+=NTP_TIMESERVER; response+="</tr>";
	response +="<tr><td class=\"cell\">LoRa Router</td><td class=\"cell\">"; response+=_TTNSERVER; response+="</tr>";
	response +="<tr><td class=\"cell\">LoRa Router IP</td><td class=\"cell\">"; response+=printIP((IPAddress)ttnServer,'.'); response+="</tr>";
#ifdef _THINGSERVER
	response +="<tr><td class=\"cell\">LoRa Router 2</td><td class=\"cell\">"; response+=_THINGSERVER; 
		response += ":"; response += String(_THINGPORT); response+="</tr>";
	response +="<tr><td class=\"cell\">LoRa Router 2 IP</td><td class=\"cell\">"; response+=printIP((IPAddress)thingServer,'.'); response+="</tr>";
#endif
	response +="</table>";

	// ------------------------------------------------------------------------
	if (debug >= 2) {
		
		response +="<h2>System State and Interrupt</h2>";
		
		response +="<table class=\"config_table\">";
		response +="<tr>";
		response +="<th class=\"thead\">Parameter</th>";
		response +="<th class=\"thead\">Value</th>";
		response +="</tr>";
		response +="<tr><td class=\"cell\">_state</td>";
		response +="<td class=\"cell\">"; 
		switch (_state) {							// See loraModem.h
			case 0: response +="INIT"; break;
			case 1: response +="SCAN"; break;
			case 2: response +="CAD"; break;
			case 3: response +="RX"; break;
			case 4: response +="RXDONE"; break;
			case 5: response +="TX"; break;
			default: response +="unknown"; break;
		}
		response +="</td></tr>";

		response +="<tr><td class=\"cell\">flags (8 bits)</td>";
		response +="<td class=\"cell\">0x";
		if (flags <16) response += "0";
		response+=String(flags,HEX); response+="</td></tr>";

		response +="<tr><td class=\"cell\">mask (8 bits)</td>";
		response +="<td class=\"cell\">0x"; 
		if (mask <16) response += "0";
		response+=String(mask,HEX); response+="</td></tr>";
		response +="</table>";
	}

	// ------------------------------------------------------------------------
	response +="<br><br />";
	response +="Click <a href=\"/HELP\">here</a> to explain Help and REST options<br>";
	response +="</BODY></HTML>";

	delay(3);

	return (response);
}

// ----------------------------------------------------------------------------
// Call the webserver
//
// ----------------------------------------------------------------------------
void sendWebPage(String webPage) {
	server.send(200, "text/html", webPage);	
}

// ----------------------------------------------------------------------------
// SetupWWW function called by main setup() program to setup webserver
// It does actually not much more than installaing the callback handlers
// for messages sent to the webserver
//
// ----------------------------------------------------------------------------
void setupWWW() 
{
	server.begin();											// Start the webserver
		
	server.on("/", []() {
		sendWebPage(WifiServer("",""));
	});
	server.on("/HELP", []() {
		sendWebPage(WifiServer("HELP",""));					// Send the webPage string
	});
	server.on("/RESET", []() {
		sendWebPage(WifiServer("RESET",""));				// Send the webPage string
	});
	server.on("/NEWSSID", []() {
		sendWebPage(WifiServer("NEWSSID",""));				// Send the webPage string
	});
	server.on("/DEBUG=-1", []() {
		sendWebPage(WifiServer("DEBUG","-1"));				// Send the webPage string
	});
	server.on("/DEBUG=1", []() {
		sendWebPage(WifiServer("DEBUG","1"));				// Send the webPage string
	});
	server.on("/DELAY=1", []() {
		sendWebPage(WifiServer("DELAY","1"));				// Send the webPage string
	});
	server.on("/DELAY=-1", []() {
		sendWebPage(WifiServer("DELAY","-1"));				// Send the webPage string
	});
	server.on("/SF=1", []() {
		sendWebPage(WifiServer("SF","1"));					// Send the webPage string
	});
	server.on("/SF=-1", []() {
		sendWebPage(WifiServer("SF","-1"));					// Send the webPage string
	});
	server.on("/FREQ=1", []() {
		sendWebPage(WifiServer("FREQ","1"));				// Send the webPage string
	});
	server.on("/FREQ=-1", []() {
		sendWebPage(WifiServer("FREQ","-1"));				// Send the webPage string
	});
	server.on("/FCNT", []() {
		String webPage="";
		webPage = WifiServer("FCNT","");
		server.send(200, "text/html", webPage);				// Send the webPage string
	});
	server.on("/CAD=1", []() {
		sendWebPage(WifiServer("CAD","1"));					// Send the webPage string
	});
	server.on("/CAD=0", []() {
		sendWebPage(WifiServer("CAD","0"));					// Send the webPage string
	});
	server.on("/HOP=1", []() {
		sendWebPage(WifiServer("HOP","1"));					// Send the webPage string
	});
	server.on("/HOP=0", []() {
		sendWebPage(WifiServer("HOP","0"));					// Send the webPage string
	});
	Serial.print(F("Admin Server started on port "));
	Serial.println(SERVERPORT);
	return;
}

#endif

