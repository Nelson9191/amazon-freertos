#ifndef _AT_COMMANDS_H_
#define _AT_COMMANDS_H_

//Verify registration status and access technology
#define CREG                "AT+CREG?"
#define CREG_OK             "+CREG: 1,1"

//Verify registration status
#define CGREG               "AT+CGREG?"
#define CGREG_OK            "+CREG: 0,1"

//Verify signal
#define CSQ                 "AT+CSQ"
#define CSQ_ERROR           "99"

//Activate GPRS
#define CGATT_ACTIVATE      "AT+CGATT=1"
#define CGATT               "AT+CGATT?"
#define CGATT_OK            "+CGATT: 1"

//Set PDP context
#define SET_CGDCONT         "AT+CGDCONT=1,\"IP\",\"lte.avantel.com.co\""
#define CGDCONT_LIST        "AT+CGDCONT?"
#define CGDCONT_OK          "lte.avantel.com.co"

//List of contexts
#define CGACT_LIST          "AT+CGACT?"
#define CGACT_LIST_OK       "lte.avantel.com.co"

//Verify that the device is connected
#define CGACT_LIST          "AT+CGACT?"
#define SET_CGACT           "AT+CGACT=1,1"
#define CGACT_OK            ",1"

//Verify IP address
#define CGPADDR             "AT+CGPADDR=1"
#define CGPADDR_FAIL        "0.0.0.0"


/*

AT+CREG AT command gives information about the registration status and access technology of the serving cell.

Possible values of registration status are,
0 not registered, MT is not currently searching a new operator to register to
1 registered, home network
2 not registered, but MT is currently searching a new operator to register to
3 registration denied
4 unknown (e.g. out of GERAN/UTRAN/E-UTRAN coverage)
5 registered, roaming
6 registered for "SMS only", home network (applicable only when indicates E-UTRAN)
7 registered for "SMS only", roaming (applicable only when indicates E-UTRAN)
8 attached for emergency bearer services only (see NOTE 2) (not applicable)
9 registered for "CSFB not preferred", home network (applicable only when indicates E-UTRAN)
10 registered for "CSFB not preferred", roaming (applicable only when indicates E-UTRAN)

Possible values for access technology are,
0 GSM
1 GSM Compact
2 UTRAN
3 GSM w/EGPRS
4 UTRAN w/HSDPA
5 UTRAN w/HSUPA
6 UTRAN w/HSDPA and HSUPA
7 E-UTRAN


AT+CGREG AT command returns the registration status of the device
//Get current registration status
//If there is any error, CME error code is returned

AT+CGREG?


//Configure to return unsolicited result code when network registration is disabled
AT+CGREG=0

//Configure to return unsolicited result code  with when network registration is enabled
AT+CGREG=1

//Configure to return unsolicited result code when there is change in
//network registration status or change of network cell
AT+CGREG=2


AT+CSQ AT command returns the signal strength of the device.


*/