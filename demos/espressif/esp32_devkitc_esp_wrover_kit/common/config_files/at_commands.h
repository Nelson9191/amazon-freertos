#ifndef _AT_COMMANDS_H_
#define _AT_COMMANDS_H_

#include "mqtt_config.h"

#define AT                  "AT"
#define AT_OK               "OK"

#define COPS                "AT+COPS=1,1,\"Avantel\""
#define COPS_LIST           "AT+COPS?"

//Verify registration status and access technology
#define CREG                "AT+CREG?"
#define CREG_OK             ",1"

//Verify registration status
#define CGREG               "AT+CGREG?"
#define CGREG_OK            ",1"

//Verify signal
#define CSQ                 "AT+CSQ"
#define CSQ_ERROR           "99,"

//Activate GPRS
#define CGATT_ACTIVATE      "AT+CGATT=1"
#define CGATT               "AT+CGATT?"
#define CGATT_OK            "+CGATT: 1"

//delecte PDP contexts
#define GC_DEL              "AT+CGDEL=1"

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


#define CLIENT_CERT         "clientcert.pem"
#define CLIENT_KEY          "clientkey.pem"
#define CA_CERT             "cacert.pem"

#define WRITE_CERT_TEMPLATE "AT+CCERTDOWN="
#define CERT_LIST           "AT+CCERTLIST"

//Config authentication
#define SSL_VERSION         "AT+CSSLCFG=\"sslversion\",0,4"
#define SSL_AUTH_MODE       "AT+CSSLCFG=\"authmode\",0,2"
#define SSL_CACERT          "AT+CSSLCFG=\"cacert\",0,\"" CA_CERT "\""
#define SSL_CLIENT_CERT     "AT+CSSLCFG=\"clientcert\",0,\"" CLIENT_CERT "\""
#define SSL_CLIENT_KEY      "AT+CSSLCFG=\"clientkey\",0,\"" CLIENT_KEY "\"" 


//MQTT
#define MQTT_START          "AT+CMQTTSTART"
#define MQTT_SET_CLIENT_ID  "AT+CMQTTACCQ=0,\"" MQTT_CLIENT_ID "\",1"
#define MQTT_SSL_CONFIG     "AT+CMQTTSSLCFG=0,0"


#define CFG_WILL_TOPIC      "AT+CMQTTWILLTOPIC=0,31"
#define WILL_TOPIC          "aws/things/simcom7600_device01/"
#define CFG_WILL_SMG        "AT+CMQTTWILLMSG=0,17,1"
#define WILL_MSG            "SIMCom Connected!"

#define _MQTT_ENDPOINT_START "AT+CMQTTCONNECT=0,\"tcp://\""
#define _MQTT_ENDPOINT_END   "\":8883\",60,1"
#define MQTT_GET_ENDPOINT()  (_MQTT_ENDPOINT  MQTT_BROKER_ENDPOINT  _MQTT_ENDPOINT_END)

#endif
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